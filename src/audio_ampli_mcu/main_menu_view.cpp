#include "main_menu_view.h"

#include "mute_img.h"
#include "small_speaker_img.h"

MainMenuView::MainMenuView(
  OptionController* option_ctrl_ptr,
  VolumeController* volume_ctrl_ptr,
  PersistentData* persistent_data_ptr,
  StateMachine* state_machine_ptr,
  const LvFontWrapper& small_font,
  const LvFontWrapper& digit_font)
  : option_ctrl_ptr_(option_ctrl_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , state_machine_ptr_(state_machine_ptr)
  , small_font_(small_font)
  , digit_font_(digit_font)
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
  option_ctrl_ptr_->increment_option(Option::audio_input, inversed_dir);
}

void MainMenuView::draw(Display& display, const bool has_state_changed)
{
  draw_volume(display, has_state_changed);
  draw_audio_inputs(display, has_state_changed);
  draw_left_right_bal_indicator(display, has_state_changed);
}

void MainMenuView::draw_left_right_bal_indicator(Display& display, const bool has_state_changed)
{
  if (!has_state_changed)
  {
    return;
  }
  // If there is a left/right balance, draw an indicator on top of the volume
  if (persistent_data_ptr_->left_right_balance_db == 0)
  {
    return;
  }
  const uint32_t middle_x = LCD_WIDTH / 2;
  const uint32_t bal_top_y = 8;
  const uint32_t txt_bal_top_y = bal_top_y + 4;

  draw_string_fast(
    display, "R", middle_x - 20, txt_bal_top_y, middle_x - 3, small_font_, true, false, TextAlign::right);

  draw_image_from_top_left(display, small_speaker, middle_x, bal_top_y);

  const uint32_t x_after_speaker = middle_x + small_speaker.w_px;
  char str_buffer[10];
  snprintf(str_buffer, 10, "%+ddB", persistent_data_ptr_->left_right_balance_db);
  draw_string_fast(
    display,
    str_buffer,
    x_after_speaker + 3,
    txt_bal_top_y,
    x_after_speaker + 40,
    small_font_,
    true,
    false,
    TextAlign::left);
}
void MainMenuView::draw_volume(Display& display, const bool has_state_changed)
{
  static bool prev_mute_state = volume_ctrl_ptr_->is_muted();
  static auto prev_volume_db = volume_ctrl_ptr_->get_volume_db();

  if (
    !has_state_changed && prev_mute_state == volume_ctrl_ptr_->is_muted() &&
    prev_volume_db == volume_ctrl_ptr_->get_volume_db())
  {
    return;
  }

  char buffer[5];
  sprintf(buffer, "%d", volume_ctrl_ptr_->get_volume_db());

  const uint32_t min_x = 94;
  const uint32_t max_x = LCD_WIDTH - 8;
  const uint32_t middle_y = LCD_HEIGHT / 2;
  const uint32_t start_y = middle_y - digit_font_.get_height_px() / 2;
  const uint32_t middle_x = (max_x - min_x) / 2 + min_x;

  if (prev_mute_state != volume_ctrl_ptr_->is_muted())
  {
    display.draw_rectangle(min_x, 0, max_x, LCD_HEIGHT, BLACK_COLOR);
  }

  if (volume_ctrl_ptr_->is_muted())
  {
    draw_image(display, mute_image, middle_x + 3, middle_y);
  }
  else
  {
    draw_string_fast(display, buffer, min_x, start_y, max_x, digit_font_);
  }
  prev_mute_state = volume_ctrl_ptr_->is_muted();
  prev_volume_db = volume_ctrl_ptr_->get_volume_db();
}

void MainMenuView::draw_audio_inputs(Display& display, const bool has_state_changed)
{
  static auto prev_audio_input = persistent_data_ptr_->selected_audio_input;
  const auto& selected_audio_input = persistent_data_ptr_->selected_audio_input;

  const auto max_enum_value = static_cast<uint8_t>(AudioInput::enum_length);
  const uint32_t ver_spacing =
    LCD_HEIGHT / (max_enum_value + 1);  // + 1 is to have equal spacing between the top input and the screen's border

  const uint32_t tab_width_px = 92;
  const uint32_t tab_height_px = small_font_.get_height_px() + 8;
  const uint32_t first_option_center_y = ver_spacing;

  if (has_state_changed)
  {
    display.draw_rectangle(0, 0, tab_width_px + 1, LCD_HEIGHT, BLACK_COLOR);
  }
  else if (prev_audio_input == selected_audio_input)
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
    const auto was_previously_selected = prev_audio_input == audio_input;
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
      const auto maybe_audio_input_str = option_ctrl_ptr_->get_input_rename_value(audio_input);
      if (maybe_audio_input_str)
      {
        draw_string_fast(
          display, *maybe_audio_input_str, 0, curr_option_text_top_y, tab_width_px, small_font_, !is_selected, false);
      }
    }
  }
  prev_audio_input = selected_audio_input;
}

// const char* MainMenuView::get_audio_input_renamed_str(const AudioInput& audio_input) const
// {
//   // Get the name alias
//   const auto& name_alias = persistent_data_ptr_->get_per_audio_input_data(audio_input).name_alias;
//   switch (name_alias)
//   {
//     case InputNameAliasOption::no_alias:
//       return audio_input_to_string(audio_input);
//       break;
//     case InputNameAliasOption::dac:
//       return "DAC";
//     case InputNameAliasOption::cd:
//       return "CD";
//     case InputNameAliasOption::phono:
//       return "PHONO";
//     case InputNameAliasOption::tuner:
//       return "TUNER";
//     case InputNameAliasOption::aux:
//       return "AUX";
//     case InputNameAliasOption::stream:
//       return "STREAM";
//     case InputNameAliasOption::enum_length:
//       return "ERR1";
//   }
//   return "ERR2";
// }