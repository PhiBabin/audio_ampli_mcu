#include "standby_view.h"

StandbyView::StandbyView(
  StateMachine& state_machine,
  Display& display,
  const LvFontWrapper& font,
  const lv_img_dsc_t& cat_image)
  : state_machine_(state_machine)
  , display_(display)
  , font_(font)
  , cat_image_(cat_image)
{
}

void StandbyView::draw(const bool has_state_changed)
{
  constexpr int wait_time_between_drawing_ZZZ = 2000;
  constexpr int number_of_ZZZ = 4;

  if (state_machine_.get_state() != State::standby)
  {
    return;
  }

  // We just switched to standby
  if (has_state_changed)
  {
    display_.clear_screen(BLACK_COLOR);
    draw_image_from_top_left(
      display_,
      cat_image_,
      LCD_WIDTH - cat_image_.w_px - 1,
      LCD_HEIGHT - cat_image_.h_px - 1);
    timer_ = millis();
    zzz_count_ = 1;
  }

  if (has_state_changed || millis() - timer_ > wait_time_between_drawing_ZZZ)
  {
    timer_ = millis();

    const auto& font = font_;
    constexpr int16_t start_x = 220;
    constexpr int16_t top_y = 120;
    const auto maybe_z_glyph = font.get_glyph('Z');
    const int16_t spacing_x = maybe_z_glyph ? maybe_z_glyph.value()->width_px + font.get_spacing_px() + 2 : 20;

    if (zzz_count_ > number_of_ZZZ)
    {
      zzz_count_ = 1;
      display_.draw_rectangle(
        start_x,
        top_y - 3 * number_of_ZZZ,
        start_x + number_of_ZZZ * spacing_x,
        top_y + font.get_height_px(),
        BLACK_COLOR);
    }
    for (int i = 0; i < zzz_count_; ++i)
    {
      const auto height = top_y - 2 * i;
      draw_string_fast(display_, "Z", start_x + i * spacing_x, height, start_x + (i + 1) * spacing_x, font);
    }
    ++zzz_count_;
  }
}
