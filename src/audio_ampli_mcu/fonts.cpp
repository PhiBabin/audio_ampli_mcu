#include "fonts.h"

#include <cstring>

#ifdef SIM
#include "sim/LCD_Driver.h"
#include "sim/arduino.h"
#else
#include "LCD_Driver.h"
#endif

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void draw_character_fast(
  const LvFontWrapper::LvGlyph* glyph,
  const uint32_t start_x,
  const uint32_t start_y,
  bool is_white_on_black,
  bool draw_spacing)
{
  uint32_t end_x = start_x + (draw_spacing ? glyph->width_with_spacing_px : glyph->width_px);
  const uint32_t end_y = start_y + glyph->height_px;

  // Make sure that we have an even number of columns, that way we don't have to worry about write call with only one
  // column
  if ((end_x - start_x) % 2 != 0)
  {
    ++end_x;
  }

  //   Serial.print("\tstart_x=");
  //   Serial.print(start_x);
  //   Serial.print("end_x=");
  //   Serial.print(end_x);
  //   Serial.println("");
  // LCD will auto increment the row when we reach columns == end_x
  LCD_SetWindow(start_x, start_y, end_x, end_y + 1);
  for (uint32_t y = 0; y < end_y - start_y; ++y)
  {
    uint8_t px_count = 0;
    uint32_t color_2pixels = 0;
    for (uint32_t x = 0; x < end_x - start_x; ++x)
    {
      auto color_4bit = glyph->get_color(x, y);
      if (!is_white_on_black)
      {
        color_4bit = 0xf - (color_4bit & 0xf);
      }
      // Convert 4bit grayscale to four 4bit RGB
      color_2pixels = (color_2pixels << 12) | (color_4bit << 8 | color_4bit << 4 | color_4bit);
      ++px_count;
      if (px_count == 2)
      {
        LCD_write_2pixel_color(color_2pixels);
        px_count = 0;
        color_2pixels = 0;
      }
    }
  }
}

void draw_string_fast(
  const char* str,
  const uint32_t start_x,
  const uint32_t start_y,
  const uint32_t end_x,
  const LvFontWrapper& font,
  bool is_white_on_black,
  bool clear_side)
{
  if (str == NULL || *str == '\0')
  {
    return;
  }
  if (start_x > end_x)
  {
    return;
  }
  uint32_t text_width_px = 0;
  const char* str_temp = str;
  while (*str_temp != '\0')
  {
    if (const auto maybe_glyph = font.get_glyph(*str_temp); maybe_glyph)
    {
      text_width_px += maybe_glyph.value()->width_px;
    }
    ++str_temp;
  }
  text_width_px += (strlen(str) - 1) * font.get_spacing_px();
  const auto end_y = start_y + font.get_height_px();
  const int32_t middle_x = static_cast<int32_t>((end_x - start_x) / 2 + start_x);
  const uint32_t start_text_x = static_cast<uint32_t>(MAX(0L, middle_x - static_cast<int32_t>(text_width_px / 2)));
  const auto end_text_x = middle_x + text_width_px / 2;

  //   Serial.print("start_x=");
  //   Serial.print(start_x);
  //   Serial.print("start_text_x=");
  //   Serial.print(start_text_x);
  //   Serial.print("end_text_x=");
  //   Serial.print(end_text_x);
  //   Serial.print("end_x=");
  //   Serial.print(end_x);
  //   Serial.println("");

  if (start_x < start_text_x && clear_side)
  {
    // whiteout
    LCD_ClearWindow_12bitRGB(start_x, start_y, start_text_x + 1, end_y, is_white_on_black ? BLACK_COLOR : WHITE_COLOR);
  }
  if (end_text_x < end_x && clear_side)
  {
    // whiteout
    LCD_ClearWindow_12bitRGB(end_text_x, start_y, end_x + 1, end_y, is_white_on_black ? BLACK_COLOR : WHITE_COLOR);
  }
  str_temp = str;
  auto current_text_x = start_text_x;
  while (*str_temp != '\0')
  {
    if (const auto maybe_glyph = font.get_glyph(*str_temp); maybe_glyph)
    {
      // If next character is null byte, we reach end of string
      const bool do_draw_spacing = str_temp[1] != '\0';
      draw_character_fast(*maybe_glyph, current_text_x, start_y, is_white_on_black, do_draw_spacing);
      current_text_x += maybe_glyph.value()->width_px + font.get_spacing_px();
    }
    ++str_temp;
  }
}

uint8_t LvFontWrapper::LvGlyph::get_color(const uint32_t bitmap_x_px, const uint32_t y_px) const
{
  if (bitmap_x_px >= width_px || y_px >= height_px)
  {
    return 0;
  }
  const auto bitmap_y_px = y_px + skip_top_px;
  const uint32_t bitmap_y_offset_px =
    width_px % 2 == 0 ? width_px * bitmap_y_px : (width_px + 1) * bitmap_y_px;  // Padding for odd width
  const uint32_t offset_px = bitmap_y_offset_px + bitmap_x_px;
  const auto two_pixels_byte = raw_bytes[offset_px / 2];
  if (offset_px % 2 == 0)
  {
    return two_pixels_byte >> 4;
  }
  return two_pixels_byte & 0x0F;
}

LvFontWrapper::LvFontWrapper(const lv_font_t* font)
  : font_(font), height_px_(font_->h_px - font_->h_top_skip_px - font_->h_bot_skip_px)
{
  auto add_character = [this](const uint32_t unicode, const uint32_t index) {
    LvGlyph glyph{
      .width_px = font_->glyph_dsc[index].w_px,
      .height_px = height_px_,
      .width_with_spacing_px = font_->glyph_dsc[index].w_px + font_->spacing_px,
      .skip_top_px = font_->h_top_skip_px,
      .raw_bytes = font_->glyph_bitmap + font_->glyph_dsc[index].glyph_index};
    unicode_to_char_.emplace(unicode, glyph);
  };

  // There are two way to encode the unicode, either as a continious array or as the range between unicode_first and
  // unicode_last
  if (font_->unicode_list != NULL)
  {
    uint32_t i = 0;
    while (font_->unicode_list[i] != 0)
    {
      add_character(font_->unicode_list[i], i);
      ++i;
    }
  }
  else
  {
    for (uint32_t unicode = font_->unicode_first; unicode <= font_->unicode_last; ++unicode)
    {
      add_character(unicode, unicode - font_->unicode_first);
    }
  }
}

std::optional<const LvFontWrapper::LvGlyph*> LvFontWrapper::get_glyph(const char c) const
{
  const auto iter = unicode_to_char_.find(static_cast<uint32_t>(c));
  if (iter == unicode_to_char_.end())
  {
    return {};
  }
  return std::optional<const LvGlyph*>(&iter->second);
};

uint32_t LvFontWrapper::get_height_px() const
{
  return height_px_;
}

uint32_t LvFontWrapper::get_spacing_px() const
{
  return font_->spacing_px;
}