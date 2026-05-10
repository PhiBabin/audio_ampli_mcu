#include "standby_view.h"

StandbyView::StandbyView(
  StateMachine* state_machine_ptr,
  Display* display_ptr,
  const LvFontWrapper* font_ptr,
  const lv_img_dsc_t* cat_image_ptr)
  : state_machine_ptr_(state_machine_ptr)
  , display_ptr_(display_ptr)
  , font_ptr_(font_ptr)
  , cat_image_ptr_(cat_image_ptr)
{
}

void StandbyView::draw(const bool has_state_changed)
{
  constexpr int wait_time_between_drawing_ZZZ = 2000;
  constexpr int number_of_ZZZ = 4;

  if (state_machine_ptr_->get_state() != State::standby)
  {
    return;
  }

  // We just switched to standby
  if (has_state_changed)
  {
    display_ptr_->clear_screen(BLACK_COLOR);
    draw_image_from_top_left(
      *display_ptr_,
      *cat_image_ptr_,
      LCD_WIDTH - cat_image_ptr_->w_px - 1,
      LCD_HEIGHT - cat_image_ptr_->h_px - 1);
    timer_ = millis();
    zzz_count_ = 1;
  }

  if (has_state_changed || millis() - timer_ > wait_time_between_drawing_ZZZ)
  {
    timer_ = millis();

    const auto& font = *font_ptr_;
    constexpr int16_t start_x = 220;
    constexpr int16_t top_y = 120;
    const auto maybe_z_glyph = font.get_glyph('Z');
    const int16_t spacing_x = maybe_z_glyph ? maybe_z_glyph.value()->width_px + font.get_spacing_px() + 2 : 20;

    if (zzz_count_ > number_of_ZZZ)
    {
      zzz_count_ = 1;
      display_ptr_->draw_rectangle(
        start_x,
        top_y - 3 * number_of_ZZZ,
        start_x + number_of_ZZZ * spacing_x,
        top_y + font.get_height_px(),
        BLACK_COLOR);
    }
    for (int i = 0; i < zzz_count_; ++i)
    {
      const auto height = top_y - 2 * i;
      draw_string_fast(*display_ptr_, "Z", start_x + i * spacing_x, height, start_x + (i + 1) * spacing_x, font);
    }
    ++zzz_count_;
  }
}
