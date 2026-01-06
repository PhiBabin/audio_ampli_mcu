
#ifndef __DRAW_PRIMITIVES_H
#define __DRAW_PRIMITIVES_H

#include "LCD_Driver.h"

#include <optional>
#include <stdint.h>
#include <unordered_map>
#ifdef SIM
#include "sim/arduino.h"
#else
#endif

const uint32_t WHITE_COLOR = 0xfff;
const uint32_t BLACK_COLOR = 0x0;

// New format for glyph descriptor, it includes a bounding box offset, so the bitmap doesn't have to include
// transparent pixels.
struct lv_font_fmt_txt_glyph_dsc_t
{
  // Start index of the bitmap. A font can be max 4 GB.
  uint32_t bitmap_index;
  // Draw the next glyph after this width. 28.4 format (real_value * 16 is stored)
  uint32_t adv_w;
  // Width of the glyph's bounding box
  uint16_t box_w;
  // Height of the glyph's bounding box
  uint16_t box_h;
  // x offset of the bounding box. Measured from the left of the character.
  int16_t ofs_x;
  // y offset of the bounding box. Measured from the top of the typographic "base line", NOT from the top of the
  // character. Also, the sign of the offset is positive towards the top, which is the opposite from the screen's
  // coordinate system where positive y is towards the bottom.
  int16_t ofs_y;
};

struct lv_font_glyph_dsc_t
{
  // Pixel width
  uint32_t w_px;
  // Offset in glyph bitmap
  uint32_t glyph_index;
};

struct lv_font_t
{
  // How many pixel to cut off from the top (for some reason there is at least 3 pixels wasted)
  uint32_t h_top_skip_px;
  uint32_t h_bot_skip_px;
  uint32_t spacing_px;
  // First unicode code glyph
  uint32_t unicode_first;
  // Last unicode code glyph
  uint32_t unicode_last;
  // Height of the character in bitmap in pixel
  uint32_t h_px;
  // Baseline measured from the bottom of the line
  uint32_t base_line{0};
  // Pointer to the bitmap
  const uint8_t* glyph_bitmap;
  // Glyph description (width in px + offset in bitmap)
  const lv_font_glyph_dsc_t* glyph_dsc{NULL};
  // New glyph description (width in px + offset in bitmap + bounding box)
  const lv_font_fmt_txt_glyph_dsc_t* new_glyph_dsc{NULL};
  // Unicode code of each character in bitmap
  const uint32_t* unicode_list{NULL};
};

class LvFontWrapper
{
public:
  struct LvGlyph
  {
    uint8_t get_color(const uint32_t bitmap_x_px, const uint32_t y_px) const;

    // When monospace this is fixed
    uint32_t width_px;
    // Real width of the glyph
    uint32_t bitmap_width_px;
    uint32_t height_px;  // height_px = Real height - skip_top_px - bot_skip_px
    uint32_t width_with_spacing_px;
    uint32_t skip_top_px;
    const uint8_t* raw_bytes;
    // Width of the glyph's bitmap bounding box
    uint32_t box_w;
    // Height of the glyph's bitmap bounding box
    uint32_t box_h;
    // x offset of the bounding box. Measured from the left of the character.
    uint32_t ofs_x;
    // y offset of the bounding box. Measured from the top of the character.
    uint32_t ofs_y;
  };

  LvFontWrapper(const lv_font_t* font, const bool is_monospace = false);
  std::optional<const LvGlyph*> get_glyph(const char c) const;
  uint32_t get_height_px() const;
  uint32_t get_spacing_px() const;

private:
  // Unicode to the glyph
  std::unordered_map<uint32_t, LvGlyph> unicode_to_char_;
  // Actual font informations
  const lv_font_t* font_;
  uint32_t height_px_;
};

void draw_character_fast(
  Display& display,
  const LvFontWrapper::LvGlyph* glyph,
  const uint32_t start_x,
  const uint32_t start_y,
  bool is_white_on_black = true);

enum class TextAlign : uint8_t
{
  center = 0,
  left,
  right,
  enum_length
};

void draw_string_fast(
  Display& display,
  const char* str,
  const uint32_t start_x,
  const uint32_t start_y,
  const uint32_t end_x,
  const LvFontWrapper& font,
  bool is_white_on_black = true,
  bool clear_side = true,
  const TextAlign txt_align = TextAlign::center);

void draw_multilines_string(
  Display& display,
  const char* str,
  const uint32_t start_x,
  const uint32_t start_y,
  const uint32_t end_x,
  const LvFontWrapper& font,
  bool is_white_on_black = true,
  bool clear_side = true,
  const TextAlign txt_align = TextAlign::center);

struct lv_img_dsc_t
{
  uint32_t w_px;
  uint32_t h_px;
  const uint8_t* data;
  bool has_alpha;
};

void draw_image(Display& display, const lv_img_dsc_t& img, const uint32_t center_x, const uint32_t center_y);
void draw_image_from_top_left(
  Display& display,
  const lv_img_dsc_t& img,
  const uint32_t start_x,
  const uint32_t start_y,
  const bool vertical_mirror = false);

void draw_rounded_rectangle(
  Display& display,
  const int32_t start_x,
  const int32_t start_y,
  int32_t end_x,
  const int32_t end_y,
  const bool is_white_on_black,
  const bool rounded_left,
  const bool rounded_right,
  const int32_t corner_radius_px = 7);

#endif /* __DRAW_PRIMITIVES_H */