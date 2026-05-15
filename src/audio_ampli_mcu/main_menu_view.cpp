#include "main_menu_view.h"

#include "mute_img.h"
#include "small_speaker_img.h"

#include <cstdint>
#include <cstdlib>


MainMenuView::MainMenuView(
  OptionController& option_ctrl,
  VolumeController& volume_ctrl,
  PersistentData& persistent_data,
  StateMachine& state_machine,
  const LvFontWrapper& small_font,
  const LvFontWrapper& digit_font,
  const LvFontWrapper& regular_medium_font)
  : option_ctrl_(option_ctrl)
  , volume_ctrl_(volume_ctrl)
  , persistent_data_(persistent_data)
  , state_machine_(state_machine)
  , small_font_(small_font)
  , digit_font_(digit_font)
  , regular_medium_font_(regular_medium_font)
{
}

void MainMenuView::menu_up()
{
  menu_change(IncrementDir::increment);
}

void MainMenuView::menu_down()
{
  menu_change(IncrementDir::decrement);
}

void MainMenuView::menu_change(const IncrementDir& dir)
{
  // When going up in the menu, we need to decrement the index, because menu items are display from top to bottom.
  // This is a bit confusing.
  const auto inversed_dir = dir == IncrementDir::increment ? IncrementDir::decrement : IncrementDir::increment;
  option_ctrl_.increment_option(Option::audio_input, inversed_dir);
}

void MainMenuView::draw(Display& display, const bool has_state_changed)
{
  draw_volume(display, has_state_changed);
  draw_audio_inputs(display, has_state_changed);
  draw_left_right_bal_indicator(display, has_state_changed);
}

void MainMenuView::init()
{
}

void MainMenuView::draw_left_right_bal_indicator(Display& display, const bool has_state_changed)
{
  if (!has_state_changed)
  {
    return;
  }
  // If there is a left/right balance, draw an indicator on top of the volume
  if (persistent_data_.left_right_balance_db == 0)
  {
    return;
  }
  const uint32_t bal_top_y = 8;

  const auto [left_bias, right_bias] = volume_ctrl_.get_left_right_bias_compensation();
  const int32_t left_int = left_bias / 10;
  const int32_t right_int = right_bias / 10;
  const int32_t left_rem = abs(left_bias) % 10;
  const int32_t right_rem = abs(right_bias) % 10;
  constexpr size_t buffer_len = 40;
  char str_buffer[buffer_len];
  if (left_rem != 0 && right_rem != 0)
  {
    snprintf(str_buffer, buffer_len, "%+d.%d BAL %+d.%d", left_int, left_rem, right_int, right_rem);
  }
  else if (left_rem != 0)
  {
    snprintf(str_buffer, buffer_len, "%+d.%d BAL %+d", left_int, left_rem, right_int);
  }
  else if (right_rem != 0)
  {
    snprintf(str_buffer, buffer_len, "%+d BAL %+d.%d", left_int, right_int, right_rem);
  }
  else
  {
    snprintf(str_buffer, buffer_len, "%+d BAL %+d", left_int, right_int);
  }
  draw_string_fast(display, str_buffer, 0, bal_top_y, LCD_WIDTH, small_font_, true, false, TextAlign::center);
}

void MainMenuView::draw_volume(Display& display, const bool has_state_changed)
{
  if (
    !has_state_changed && prev_mute_state_ == volume_ctrl_.is_muted() &&
    prev_volume_db_ == volume_ctrl_.get_volume_db())
  {
    return;
  }

  const int32_t int_part = volume_ctrl_.get_volume_db_int();
  const uint8_t rem = volume_ctrl_.get_volume_tenth_db_rem();

  constexpr size_t int_buffer_len = 6;
  char int_buffer[int_buffer_len] = {0};
  snprintf(int_buffer, int_buffer_len, "%d", int_part);

  constexpr uint32_t suffix_width_guess = 20;
  const uint32_t min_x = 105;
  const uint32_t max_x = LCD_WIDTH - 4 - suffix_width_guess;
  const uint32_t middle_y = LCD_HEIGHT / 2;
  const uint32_t start_y = middle_y - digit_font_.get_height_px() / 2;
  const uint32_t end_y = middle_y + digit_font_.get_height_px() / 2;
  const uint32_t middle_x = (max_x - min_x) / 2 + min_x;

  // Clear the volume area
  if (prev_mute_state_ != volume_ctrl_.is_muted())
  {
    display.draw_rectangle(min_x, 0, max_x, LCD_HEIGHT, BLACK_COLOR);
  }

  if (volume_ctrl_.is_muted())
  {
    draw_image(display, mute_image, middle_x + 3, middle_y);
  }
  else
  {
    // For -0.X dB (tenth_db negative, int_part==0), show "-0"
    if (rem != 0 && int_part == 0 && volume_ctrl_.get_volume_db() < 0)
    {
      snprintf(int_buffer, int_buffer_len, "-0");
    }

    // Measure the pixel width of the integer part text
    uint32_t int_width = 0;
    for (const char* p = int_buffer; *p; ++p)
    {
      if (const auto g = digit_font_.get_glyph(*p))
      {
        int_width += (*g)->width_px + digit_font_.get_spacing_px();
      }
    }

    // Center the combined text (integer + suffix)
    // const uint32_t combined_width = int_width;// + suffix_width;
    const int32_t draw_start_x = static_cast<int32_t>(middle_x) - static_cast<int32_t>(int_width / 2);

    // Draw integer part
    draw_string_fast(display, int_buffer, min_x, start_y, max_x, digit_font_, true, true, TextAlign::center);
    // draw_string_fast(display, int_buffer, draw_start_x, start_y, max_x, digit_font_, true, false, TextAlign::left);

    // Draw suffix in small font directly adjacent to the last digit
    const int32_t suffix_x = draw_start_x + static_cast<int32_t>(int_width);
    const int32_t suffix_y = start_y + static_cast<int32_t>(digit_font_.get_height_px()) - static_cast<int32_t>(small_font_.get_height_px()) + 4;

    // The 0.5 is a few pixel under the large digit, so since we're using the clear side of the large digit to clear the screen, we need to
    // draw black rectangle under the digit to remove the 0.5
    display.draw_rectangle(middle_x, end_y, LCD_WIDTH, suffix_y + small_font_.get_height_px() + 4, BLACK_COLOR);
    if (rem != 0)
    {
      char suffix_buffer[4] = {0};
      snprintf(suffix_buffer, sizeof(suffix_buffer), ".%d", rem);
      draw_string_fast(display, suffix_buffer, suffix_x, suffix_y, LCD_WIDTH, small_font_, true, false, TextAlign::left);
    }
    else
    {
       display.draw_rectangle(suffix_x, suffix_y, LCD_WIDTH, suffix_y + small_font_.get_height_px() + 4, BLACK_COLOR);
    }
  }
  prev_mute_state_ = volume_ctrl_.is_muted();
  prev_volume_db_ = volume_ctrl_.get_volume_db();
}

void MainMenuView::draw_audio_inputs(Display& display, const bool has_state_changed)
{
  const auto& selected_audio_input = persistent_data_.selected_audio_input;

  const auto max_enum_value = static_cast<uint8_t>(AudioInput::enum_length);
  const uint32_t ver_spacing =
    LCD_HEIGHT / (max_enum_value + 1);  // + 1 is to have equal spacing between the top input and the screen's border

  const uint32_t tab_width_px = 100;
  const uint32_t tab_height_px = small_font_.get_height_px() + 8;
  const uint32_t first_option_center_y = ver_spacing;

  if (has_state_changed)
  {
    display.draw_rectangle(0, 0, tab_width_px + 1, LCD_HEIGHT, BLACK_COLOR);
  }
  else if (prev_audio_input_ == selected_audio_input)
  {
    return;
  }

  for (uint8_t enum_value = 0; enum_value < max_enum_value; ++enum_value)
  {
    const auto audio_input = static_cast<AudioInput>(enum_value);
    const auto curr_option_center_y = first_option_center_y + enum_value * ver_spacing;
    const auto curr_option_box_top_y = curr_option_center_y - tab_height_px / 2;
    const auto curr_option_text_top_y = curr_option_center_y - small_font_.get_height_px() / 2;

    const auto is_selected = selected_audio_input == audio_input;
    const auto was_previously_selected = prev_audio_input_ == audio_input;
    if (is_selected)
    {
      draw_rounded_rectangle(
        display,
        0,
        curr_option_box_top_y,
        tab_width_px,
        curr_option_box_top_y + tab_height_px + 1,
        /*is_white_on_black=*/true,
        /*rounded_left =*/false,
        /*rounded_right =*/true);
    }
    else if (was_previously_selected)
    {
      display.draw_rectangle(
        0, curr_option_box_top_y, tab_width_px, curr_option_box_top_y + tab_height_px + 1, BLACK_COLOR);
    }
    if (is_selected || was_previously_selected || has_state_changed)
    {
      const auto maybe_audio_input_str = option_ctrl_.get_input_rename_value(audio_input);
      if (maybe_audio_input_str)
      {
        draw_string_fast(
          display, *maybe_audio_input_str, 0, curr_option_text_top_y, tab_width_px, small_font_, !is_selected, false);
      }
    }
  }
  prev_audio_input_ = selected_audio_input;
}
