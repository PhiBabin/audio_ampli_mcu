#include "draw_primitives.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <stdlib.h>

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void draw_character_fast(
  Display& display,
  const LvFontWrapper::LvGlyph* glyph,
  const uint32_t start_x,
  const uint32_t start_y,
  bool is_white_on_black,
  bool draw_spacing)
{
  uint32_t end_x = start_x + (draw_spacing ? glyph->width_with_spacing_px : glyph->width_px);
  uint32_t end_y = start_y + glyph->height_px;
  if (end_y > LCD_HEIGHT)
  {
    end_y = LCD_HEIGHT;
  }

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

  for (uint32_t y = 0; y < end_y - start_y; ++y)
  {
    uint32_t rgb444_color = 0;
    for (uint32_t x = 0; x < end_x - start_x; ++x)
    {
      auto color_4bit = glyph->get_color(x, y);
      if (!is_white_on_black)
      {
        color_4bit = 0xf - (color_4bit & 0xf);
      }
      // Convert 4bit grayscale to four 4bit RGB
      rgb444_color = (color_4bit << 8 | color_4bit << 4 | color_4bit);
      display.set_pixel_unsafe(x + start_x, y + start_y, rgb444_color);
    }
  }

  // LCD will auto increment the row when we reach columns == end_x
  // DEV_SPI_BEGIN_TRANS;
  // LCD_SetWindow(start_x, start_y, end_x, end_y + 1);
  // for (uint32_t y = 0; y < end_y - start_y; ++y)
  // {
  //   uint8_t px_count = 0;
  //   uint32_t color_2pixels = 0;
  //   for (uint32_t x = 0; x < end_x - start_x; ++x)
  //   {
  //     auto color_4bit = glyph->get_color(x, y);
  //     if (!is_white_on_black)
  //     {
  //       color_4bit = 0xf - (color_4bit & 0xf);
  //     }
  //     // Convert 4bit grayscale to four 4bit RGB
  //     color_2pixels = (color_2pixels << 12) | (color_4bit << 8 | color_4bit << 4 | color_4bit);
  //     ++px_count;
  //     if (px_count == 2)
  //     {
  //       LCD_write_2pixel_color(color_2pixels);
  //       px_count = 0;
  //       color_2pixels = 0;
  //     }
  //   }
  // }
  // DEV_SPI_END_TRANS;
}

void draw_multilines_string(
  Display& display,
  const char* str,
  const uint32_t start_x,
  const uint32_t start_y,
  const uint32_t end_x,
  const LvFontWrapper& font,
  bool is_white_on_black,
  bool clear_side,
  const TextAlign txt_align)
{
  const char* start = str;
  uint32_t top_y = start_y;
  const uint32_t line_spacing = font.get_height_px() + font.get_height_px() / 2;
  for (;; ++str)
  {
    const ptrdiff_t length = str - start;
    if ((*str == '\n' || *str == '\0') && length > 0)
    {
      char* line_buffer = static_cast<char*>(malloc(length + 2));
      strncpy(line_buffer, start, length);
      line_buffer[length] = '\0';
      draw_string_fast(display, line_buffer, start_x, top_y, end_x, font, is_white_on_black, clear_side, txt_align);
      free(line_buffer);

      start = str + 1;
      top_y += line_spacing;
    }
    if (*str == '\0')
    {
      break;
    }
  }
}

void draw_string_fast(
  Display& display,
  const char* str,
  const uint32_t start_x,
  const uint32_t start_y,
  const uint32_t end_x,
  const LvFontWrapper& font,
  bool is_white_on_black,
  bool clear_side,
  const TextAlign txt_align)
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
  text_width_px +=
    (strlen(str) - 1) * font.get_spacing_px();  // This kind of assumes that every character in string has a glyph
  uint32_t end_y = start_y + font.get_height_px();
  if (end_y > LCD_HEIGHT)
  {
    end_y = LCD_HEIGHT;
  }

  uint32_t start_text_x = 0;
  uint32_t end_text_x = 0;
  switch (txt_align)
  {
    case TextAlign::center:
    {
      const int32_t middle_x = static_cast<int32_t>((end_x - start_x) / 2 + start_x);
      start_text_x = static_cast<uint32_t>(MAX(0L, middle_x - static_cast<int32_t>(text_width_px / 2)));
      end_text_x = middle_x + text_width_px / 2;
      break;
    }
    case TextAlign::right:
    {
      start_text_x = end_x > text_width_px ? end_x - text_width_px : 0;
      end_text_x = end_x;
      break;
    }
    default:
    case TextAlign::left:
    {
      start_text_x = start_x;
      end_text_x = start_x + text_width_px;
      break;
    }
  }

  // Serial.print("start_x=");
  // Serial.print(start_x);
  // Serial.print("start_text_x=");
  // Serial.print(start_text_x);
  // Serial.print("end_text_x=");
  // Serial.print(end_text_x);
  // Serial.print("end_x=");
  // Serial.print(end_x);
  // Serial.println("");

  if (start_x < start_text_x && clear_side)
  {
    // whiteout
    // LCD_ClearWindow_12bitRGB(
    display.draw_rectangle(start_x, start_y, start_text_x + 1, end_y, is_white_on_black ? BLACK_COLOR : WHITE_COLOR);
  }
  if (end_text_x < end_x && clear_side)
  {
    // whiteout
    // LCD_ClearWindow_12bitRGB(
    display.draw_rectangle(end_text_x, start_y, end_x + 1, end_y, is_white_on_black ? BLACK_COLOR : WHITE_COLOR);
  }
  str_temp = str;
  auto current_text_x = start_text_x;
  while (*str_temp != '\0')
  {
    if (const auto maybe_glyph = font.get_glyph(*str_temp); maybe_glyph)
    {
      // If next character is null byte, we reach end of string
      const bool do_draw_spacing = str_temp[1] != '\0';
      draw_character_fast(display, *maybe_glyph, current_text_x, start_y, is_white_on_black, do_draw_spacing);
      current_text_x += maybe_glyph.value()->width_px + font.get_spacing_px();
    }
    ++str_temp;
  }
}

uint8_t LvFontWrapper::LvGlyph::get_color(const uint32_t x_px, const uint32_t y_px) const
{
  // Monospace font offset the font
  const uint32_t left_offset_px = (width_px - bitmap_width_px) / 2;
  if (x_px < left_offset_px || x_px >= bitmap_width_px + left_offset_px || y_px >= height_px)
  {
    return 0;
  }
  const auto bitmap_x_px = x_px - left_offset_px;
  const auto bitmap_y_px = y_px + skip_top_px;

  // To save on memory, the bitmap doesn't include zone of empty space, e.g. the space character doesn't have any data
  // in the bitmap. So we need to check if the pixel x/y is within the bounding box before indexing into it.
  if (bitmap_x_px < ofs_x || bitmap_y_px < ofs_y || bitmap_x_px >= ofs_x + box_w || bitmap_y_px >= ofs_y + box_h)
  {
    return 0;
  }
  const auto bounding_box_x_px = bitmap_x_px - ofs_x;
  const auto bounding_box_y_px = bitmap_y_px - ofs_y;
  const uint32_t bitmap_y_offset_px = box_w * bounding_box_y_px;
  // const uint32_t bitmap_y_offset_px =
  //   box_w % 2 == 0 ? box_w * bounding_box_y_px : (box_w + 1) * bounding_box_y_px;  // Padding for odd width
  const uint32_t offset_px = bitmap_y_offset_px + bounding_box_x_px;
  const auto two_pixels_byte = raw_bytes[offset_px / 2];
  if (offset_px % 2 == 0)
  {
    return two_pixels_byte >> 4;
  }
  return two_pixels_byte & 0x0F;
}

LvFontWrapper::LvFontWrapper(const lv_font_t* font, const bool is_monospace)
  : font_(font), height_px_(font_->h_px - font_->h_top_skip_px - font_->h_bot_skip_px)
{
  assert(font_->glyph_dsc != NULL || font_->new_glyph_dsc != NULL);
  auto add_character = [this](const uint32_t unicode, const uint32_t index) {
    if (font_->glyph_dsc != NULL)
    {
      LvGlyph glyph{
        .width_px = font_->glyph_dsc[index].w_px,  // Will be updated if monospace
        .bitmap_width_px = font_->glyph_dsc[index].w_px,
        .height_px = height_px_,
        .width_with_spacing_px = font_->glyph_dsc[index].w_px + font_->spacing_px,
        .skip_top_px = font_->h_top_skip_px,
        .raw_bytes = font_->glyph_bitmap + font_->glyph_dsc[index].glyph_index,
        .box_w =
          (font_->glyph_dsc[index].w_px % 2 == 0 ? font_->glyph_dsc[index].w_px
                                                 : font_->glyph_dsc[index].w_px + 1),  // Padding for odd width
        .box_h = font_->h_px,
        .ofs_x = 0,
        .ofs_y = 0};
      unicode_to_char_.emplace(unicode, glyph);
    }
    else if (font_->new_glyph_dsc != NULL)
    {
      // +1 is added here, because the first character is a null character
      const auto& descriptor = font_->new_glyph_dsc[index + 1];
      const uint32_t width = descriptor.adv_w / 16;
      // We don't support negative x offset, so we set negative value to zero
      const auto ofs_x = static_cast<uint32_t>(descriptor.ofs_x < 0 ? 0 : descriptor.ofs_x);
      // For some reason the y offset is convoluted and it's an offset relative to the "base line" of the character.
      // The offset is converted to an offset relative to the top left corner of the character.
      const auto ofs_y = height_px_ - (descriptor.box_h + descriptor.ofs_y + font_->base_line);
      LvGlyph glyph{
        .width_px = width,  // Will be updated if monospace
        .bitmap_width_px = width,
        .height_px = height_px_,
        .width_with_spacing_px = width + font_->spacing_px,
        .skip_top_px = font_->h_top_skip_px,
        .raw_bytes = font_->glyph_bitmap + descriptor.bitmap_index,
        .box_w = descriptor.box_w,
        .box_h = descriptor.box_h,
        .ofs_x = ofs_x,
        .ofs_y = ofs_y};
      unicode_to_char_.emplace(unicode, glyph);
    }
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

  // Update width_px to the largest glyph width
  if (is_monospace && !unicode_to_char_.empty())
  {
    const auto max_width =
      std::max_element(unicode_to_char_.begin(), unicode_to_char_.end(), [](const auto& a, const auto& b) {
        return a.second.width_px < b.second.width_px;
      })->second.width_px;
    for (auto& [unicode, glyph] : unicode_to_char_)
    {
      // 0-9
      if (48 <= unicode && unicode <= 57)
      {
        glyph.width_px = max_width;
        glyph.width_with_spacing_px = max_width + font_->spacing_px;
      }
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

void draw_image(Display& display, const lv_img_dsc_t& img, const uint32_t center_x, const uint32_t center_y)
{
  const uint32_t start_x = center_x - img.w_px / 2;
  const uint32_t start_y = center_y - img.h_px / 2;
  draw_image_from_top_left(display, img, start_x, start_y);
}

void draw_image_from_top_left(
  Display& display, const lv_img_dsc_t& img, const uint32_t start_x, const uint32_t start_y, const bool vertical_mirror)
{
  uint32_t end_x = start_x + img.w_px;
  const uint32_t end_y = start_y + img.h_px;

  //   Serial.print("\tstart_x=");
  //   Serial.print(start_x);
  //   Serial.print("end_x=");
  //   Serial.print(end_x);
  //   Serial.println("");

  const uint32_t span = img.has_alpha ? 4 : 3;
  for (uint32_t y = 0; y < end_y - start_y; ++y)
  {
    uint32_t rgb444_color = 0;
    for (uint32_t x = 0; x < end_x - start_x; ++x)
    {
      uint32_t r = 0;
      uint32_t g = 0;
      uint32_t b = 0;
      if (x < img.w_px && y < img.h_px)
      {
        // If vertically mirror change the x offset
        const auto x_mirror = vertical_mirror ? img.w_px - x - 1 : x;
        const auto offset = y * img.w_px * span + x_mirror * span;
        // Due to little endianness it's BGR, not RGB
        b = img.data[offset];
        g = img.data[offset + 1];
        r = img.data[offset + 2];
      }
      // 8 bit -> 4bit
      r >>= 4;
      g >>= 4;
      b >>= 4;
      // Convert 4bit grayscale to four 4bit RGB
      rgb444_color = ((r << 8) | (g << 4) | b);
      display.set_pixel_unsafe(x + start_x, y + start_y, rgb444_color);
      // color_2pixels = (color_2pixels << 12) | ((r << 8) | (g << 4) | b);
      // ++px_count;
      // if (px_count == 2)
      // {
      //   LCD_write_2pixel_color(color_2pixels);
      //   px_count = 0;
      //   color_2pixels = 0;
      // }
    }
  }
  // DEV_SPI_BEGIN_TRANS;
  // LCD_SetWindow(start_x, start_y, end_x, end_y + 1);
  // const uint32_t span = img.has_alpha ? 4 : 3;
  // for (uint32_t y = 0; y < end_y - start_y; ++y)
  // {
  //   uint8_t px_count = 0;
  //   uint32_t color_2pixels = 0;
  //   for (uint32_t x = 0; x < end_x - start_x; ++x)
  //   {
  //     uint32_t r = 0;
  //     uint32_t g = 0;
  //     uint32_t b = 0;
  //     if (x < img.w_px && y < img.h_px)
  //     {
  //       const auto offset = (img.w_px % 2 == 0) ? y * img.w_px * span + x * span : y * (img.w_px + 1) * span + x *
  //       span;
  //       // Due to little endianness it's BGR, not RGB
  //       b = img.data[offset];
  //       g = img.data[offset + 1];
  //       r = img.data[offset + 2];
  //     }
  //     // 8 bit -> 4bit
  //     r >>= 4;
  //     g >>= 4;
  //     b >>= 4;
  //     // Convert 4bit grayscale to four 4bit RGB
  //     color_2pixels = (color_2pixels << 12) | ((r << 8) | (g << 4) | b);
  //     ++px_count;
  //     if (px_count == 2)
  //     {
  //       LCD_write_2pixel_color(color_2pixels);
  //       px_count = 0;
  //       color_2pixels = 0;
  //     }
  //   }
  // }
  // DEV_SPI_END_TRANS;
}

/// Implementation of a fast anti-aliased rounded rectangle.
/// Based on Fast Anti-Aliased Circle Generation". In James Arvo (ed.). Graphics Gems II.
void draw_rounded_rectangle(
  Display& display,
  const int32_t start_x,
  const int32_t start_y,
  int32_t end_x,
  const int32_t end_y,
  const bool is_white_on_black,
  const bool rounded_left,
  const bool rounded_right,
  const int32_t corner_radius_px)
{
  assert(start_x >= 0);
  assert(start_y >= 0);
  assert(end_x <= LCD_WIDTH);
  assert(end_y <= LCD_HEIGHT);
  assert(start_x <= end_x);
  assert(start_y <= end_y);

  const int32_t left_corner_center_x = start_x + corner_radius_px - 1;
  const int32_t top_corner_center_y = start_y + corner_radius_px - 1;
  const int32_t right_corner_center_x = end_x - corner_radius_px;
  const int32_t bot_corner_center_y = end_y - corner_radius_px;

  auto draw_pixel_mirrored_8 = [&](int32_t x, int32_t y, uint32_t rgb444_color) {
    const auto& left_color = rounded_left ? rgb444_color : WHITE_COLOR;
    const auto& right_color = rounded_right ? rgb444_color : WHITE_COLOR;
    // Top left quadrant
    display.set_pixel_unsafe(left_corner_center_x - x, top_corner_center_y - y, left_color);
    display.set_pixel_unsafe(left_corner_center_x - y, top_corner_center_y - x, left_color);
    // Bottom left quadrant
    display.set_pixel_unsafe(left_corner_center_x - x, bot_corner_center_y + y, left_color);
    display.set_pixel_unsafe(left_corner_center_x - y, bot_corner_center_y + x, left_color);
    // Top right quadrant
    display.set_pixel_unsafe(right_corner_center_x + x, top_corner_center_y - y, right_color);
    display.set_pixel_unsafe(right_corner_center_x + y, top_corner_center_y - x, right_color);
    // Bottom right quadrant
    display.set_pixel_unsafe(right_corner_center_x + x, bot_corner_center_y + y, right_color);
    display.set_pixel_unsafe(right_corner_center_x + y, bot_corner_center_y + x, right_color);
  };

  int32_t x = corner_radius_px;
  int32_t y = 0;

  // The loop computes the antiliased pixels of 1/8 of a circle, because of symmetry, this 1/8 is a mirrored 8 times to
  // make the 4 rounded corners.
  while (x > y)
  {
    ++y;
    // TODO: Should precompute all value of x's value and save it in a table.
    constexpr int32_t precision = 0x10;
    const int32_t x_high_precision =
      sqrt((double)((corner_radius_px * corner_radius_px - y * y) * precision * precision));
    x = x_high_precision / precision;
    // Equivalent of  x - floor(x)
    const auto color_4bit = x_high_precision & 0xf;

    const auto rgb444_color = color_4bit << 8 | color_4bit << 4 | color_4bit;
    // Draw corner's antialiased border
    draw_pixel_mirrored_8(x, y, rgb444_color);
    // Fill corner's interior
    for (int32_t i = y; i < x; ++i)
    {
      draw_pixel_mirrored_8(i, y, WHITE_COLOR);
    }
    // Fill corner's exterior
    for (int32_t i = x + 1; i < corner_radius_px; ++i)
    {
      draw_pixel_mirrored_8(i, y, BLACK_COLOR);
    }
  }

  // Now that the corners are drawn, we can draw the not-corners, it's basically a cross-shaped area.

  // Horizontal lines
  for (int32_t j = top_corner_center_y; j <= bot_corner_center_y; ++j)
  {
    for (int32_t i = start_x; i < end_x; ++i)
    {
      display.set_pixel_unsafe(i, j, WHITE_COLOR);
    }
  }
  // Vertical lines
  for (int32_t j = 0; j < corner_radius_px; ++j)
  {
    for (int32_t i = left_corner_center_x; i <= right_corner_center_x; ++i)
    {
      // Mirror horizontally
      display.set_pixel_unsafe(i, start_y + j, WHITE_COLOR);
      display.set_pixel_unsafe(i, end_y - j - 1, WHITE_COLOR);
    }
  }
}