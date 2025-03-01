#include "options_view.h"

Menu::Menu(const OptionMenuScreen& _type, std::vector<MenuItem> _items) : type(_type), items(std::move(_items))
{
  if (!items.empty())
  {
    maybe_selected_index = items.size() - 1;
  }
}

OptionsView::OptionsView(
  OptionController* option_ctrl_ptr,
  VolumeController* volume_ctrl_ptr,
  PersistentData* persistent_data_ptr,
  StateMachine* state_machine_ptr,
  const LvFontWrapper& font)
  : option_ctrl_ptr_(option_ctrl_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , state_machine_ptr_(state_machine_ptr)
  , font_(font)
{
  menus_.emplace(
    OptionMenuScreen::main,
    Menu(
      OptionMenuScreen::main,
      {
        MenuItem{Option::gain, "GAIN", MenuItemType::increment_item},
        MenuItem{Option::output_mode, "OUTPUT MODE", MenuItemType::increment_item},
        MenuItem{Option::output_type, "OUTPUT TYPE", MenuItemType::increment_item},
        MenuItem{Option::subwoofer, "SUBWOOFER", MenuItemType::increment_item},
        MenuItem{Option::balance, "L/R BALANCE", MenuItemType::focus_item},
        MenuItem{Option::more_options, "MORE OPTION", MenuItemType::change_menu, OptionMenuScreen::advance},
        MenuItem{Option::back, "BACK", MenuItemType::change_menu, OptionMenuScreen::exit},
      }));

  menus_.emplace(
    OptionMenuScreen::advance,
    Menu(
      OptionMenuScreen::advance,
      {
        MenuItem{Option::bias, "BIAS", MenuItemType::focus_item},
        MenuItem{Option::rename_bal, "RENAME BAL ", MenuItemType::increment_item},
        MenuItem{Option::rename_rca1, "RENAME RCA1", MenuItemType::increment_item},
        MenuItem{Option::rename_rca2, "RENAME RCA2", MenuItemType::increment_item},
        MenuItem{Option::rename_rca3, "RENAME RCA3", MenuItemType::increment_item},
        MenuItem{
          Option::more_options, "FIRMWARE VERSION", MenuItemType::change_menu, OptionMenuScreen::firmware_version},
        MenuItem{Option::back, "BACK", MenuItemType::change_menu, OptionMenuScreen::main},
      }));
  menus_.emplace(
    OptionMenuScreen::firmware_version,
    Menu(
      OptionMenuScreen::firmware_version,
      {
        MenuItem{Option::text, "BAB AUDIO " VERSION_STRING, MenuItemType::text},
        MenuItem{Option::text, __DATE__, MenuItemType::text},
        MenuItem{Option::text, "BY ANDRE &", MenuItemType::text},
        MenuItem{Option::text, "PHILIPPE BABIN", MenuItemType::text},
        MenuItem{Option::back, "BACK", MenuItemType::change_menu, OptionMenuScreen::advance},
      }));
}

void Menu::change_selected_item(const IncrementDir& dir)
{
  if (!maybe_selected_index)
  {
    return;
  }
  const auto index = maybe_selected_index.value();

  // When going up in the menu, we need to decrement the index, because menu items are display from top to bottom.
  // This is a bit confusing.
  if (dir == IncrementDir::increment)
  {
    maybe_selected_index = index == 0 ? items.size() - 1 : index - 1;
  }
  else
  {
    maybe_selected_index = (index + 1) % items.size();
  }
}

std::optional<std::reference_wrapper<const MenuItem>> Menu::try_get_selected_item() const
{
  if (!maybe_selected_index)
  {
    return std::nullopt;
  }
  if (maybe_selected_index.value() >= items.size())
  {
    return std::nullopt;
  }
  return items[maybe_selected_index.value()];
}

Menu& OptionsView::get_selected_menu()
{
  return menus_.at(selected_menu_);
}
void OptionsView::power_on()
{
  option_ctrl_ptr_->power_on();
}
void OptionsView::power_off()
{
  option_ctrl_ptr_->power_off();
}

void OptionsView::on_menu_press()
{
  auto& menu = get_selected_menu();
  const auto& maybe_menu_item = menu.try_get_selected_item();
  if (!maybe_menu_item)
  {
    return;
  }
  const auto& menu_item = maybe_menu_item.value().get();
  switch (menu_item.type)
  {
    case MenuItemType::increment_item:
      option_ctrl_ptr_->increment_option(menu_item.option, IncrementDir::increment);
      break;
    case MenuItemType::focus_item:
      is_focus_ = !is_focus_;
      break;
    case MenuItemType::change_menu:
      if (menu_item.menu_to_swap == OptionMenuScreen::exit)
      {
        state_machine_ptr_->change_state(State::main_menu);
      }
      else
      {
        selected_menu_ = menu_item.menu_to_swap;
      }
      break;
    case MenuItemType::text:
    case MenuItemType::enum_length:
      break;
  }
  on_button_press_ = true;
}

void OptionsView::menu_up()
{
  menu_change(IncrementDir::increment);
}

void OptionsView::menu_down()
{
  menu_change(IncrementDir::decrement);
}

void OptionsView::menu_change(const IncrementDir& dir)
{
  auto& menu = get_selected_menu();
  const auto& maybe_menu_item = menu.try_get_selected_item();
  if (!maybe_menu_item)
  {
    return;
  }
  const auto& menu_item = maybe_menu_item.value().get();
  switch (menu_item.type)
  {
    case MenuItemType::text:
    case MenuItemType::change_menu:
    case MenuItemType::increment_item:
      menu.change_selected_item(dir);
      break;
    case MenuItemType::focus_item:
    {
      if (is_focus_)
      {
        option_ctrl_ptr_->increment_option(menu_item.option, dir);
      }
      else
      {
        menu.change_selected_item(dir);
      }
      break;
    }
    case MenuItemType::enum_length:
      break;
  }
  on_button_press_ = true;
}

void OptionsView::draw(Display& display, const bool has_state_changed)
{
  draw_menu(display, has_state_changed);
  draw_volume(display, has_state_changed);
  on_button_press_ = false;
}

void OptionsView::draw_volume(Display& display, const bool has_state_changed)
{
  const int max_time_since_last_change = 5000;
  static auto prev_volume = volume_ctrl_ptr_->get_volume_db();
  const bool has_volume_changed = volume_ctrl_ptr_->get_volume_db() != prev_volume;

  if (!has_state_changed && !has_volume_changed)
  {
    return;
  }

  static int time_since_last_change = -max_time_since_last_change;
  if (has_volume_changed)
  {
    time_since_last_change = millis();
    prev_volume = volume_ctrl_ptr_->get_volume_db();
  }

  const uint32_t y_end = 40;
  const uint32_t y_text_top = (y_end - font_.get_height_px()) / 2;

  // prev_state = state_machine.get_state();
  if (millis() - time_since_last_change >= max_time_since_last_change)
  {
    return;
  }

  char option_buffer[15] = {0};
  if (volume_ctrl_ptr_->is_muted())
  {
    strcpy(option_buffer, "[MUTED]");
  }
  else
  {
    snprintf(option_buffer, 15, "Vol: %ddB", volume_ctrl_ptr_->get_volume_db());
  }
  draw_string_fast(display, option_buffer, 0, y_text_top, LCD_WIDTH, font_);
}

void OptionsView::draw_menu(Display& display, const bool has_state_changed)
{
  static auto prev_selected_menu = selected_menu_;
  // static auto prev_option_menu = selected_menu_;
  auto& menu = get_selected_menu();
  static auto prev_menu_selection = menu.maybe_selected_index;
  const bool has_menu_changed = prev_selected_menu != selected_menu_;
  const bool has_menu_selection_changed = prev_menu_selection != menu.maybe_selected_index;

  prev_selected_menu = selected_menu_;

  if (!has_state_changed && !has_menu_changed && !has_menu_selection_changed && !on_button_press_)
  {
    prev_menu_selection = menu.maybe_selected_index;
    return;
  }
  const bool partial_redraw = !has_state_changed && !has_menu_changed;

  if (has_menu_changed)  // || prev_option_menu != option_ctrl.get_current_menu_screen())
  {
    display.clear_screen(BLACK_COLOR);
  }

  const uint32_t ver_spacing = font_.get_height_px() + 8;
  const auto num_menu_items = menu.items.size();
  for (uint8_t i = 0; i < num_menu_items; ++i)
  {
    const auto& item = menu.items[i];
    const auto curr_option_box_top_y = (i + 1) * ver_spacing + 5;
    // const auto curr_option_center_y = (i + 1) * ver_spacing + 5;
    const auto curr_option_bot_y = curr_option_box_top_y + ver_spacing;
    const auto curr_option_center_y = curr_option_box_top_y + ver_spacing / 2;
    const auto curr_option_text_top_y = curr_option_center_y - font_.get_height_px() / 2;

    // If we reach end of screen
    if (curr_option_bot_y >= LCD_HEIGHT)
    {
      break;
    }

    const auto is_prev_selected = prev_menu_selection && i == prev_menu_selection.value();
    const auto is_selected = menu.maybe_selected_index && i == menu.maybe_selected_index.value();
    const auto& label_str = item.label;

    // When there was only a button press, only update the selected option and previously selected option
    if (!is_selected && !is_prev_selected && partial_redraw)
    {
      continue;
    }

    display.draw_rectangle(
      0, curr_option_box_top_y, LCD_WIDTH, curr_option_bot_y + 1, is_selected ? WHITE_COLOR : BLACK_COLOR);

    switch (item.type)
    {
      case MenuItemType::text:
      case MenuItemType::change_menu:
        // Center the option's label
        draw_string_fast(display, label_str, 0, curr_option_text_top_y, LCD_WIDTH, font_, !is_selected, false);
        break;
      case MenuItemType::increment_item:
      case MenuItemType::focus_item:
      {
        // Draw a field label and values
        draw_string_fast(display, label_str, 0, curr_option_text_top_y, LCD_WIDTH / 2, font_, !is_selected, false);
        const char* value_str = string_format_option(item.option, is_focus_ && is_selected).value_or("ERR");
        draw_string_fast(
          display, value_str, LCD_WIDTH / 2, curr_option_text_top_y, LCD_WIDTH, font_, !is_selected, false);
        break;
      }
      case MenuItemType::enum_length:
        break;
    }
  }
  prev_menu_selection = menu.maybe_selected_index;
}

std::optional<const char*> OptionsView::string_format_option(const Option& option, const bool is_focus)
{
  switch (option)
  {
    case Option::gain:
      switch (persistent_data_ptr_->get_gain())
      {
        case GainOption::low:
          return "LOW";
        case GainOption::high:
          return "HIGH";
        default:
          return "ERR1";
      }
    case Option::output_mode:
      switch (persistent_data_ptr_->output_mode_value)
      {
        case OutputModeOption::phones:
          return "PHONES";
        case OutputModeOption::line_out:
          return "LINE OUT";
        default:
          return "ERR2";
      }
    case Option::output_type:
      switch (persistent_data_ptr_->output_type_value)
      {
        case OutputTypeOption::se:
          return "SE";
        case OutputTypeOption::bal:
          return "BAL";
        default:
          return "ERR3";
      }
    case Option::subwoofer:
      switch (persistent_data_ptr_->sufwoofer_enable_value)
      {
        case OnOffOption::on:
          return "ON";
        case OnOffOption::off:
          return "OFF";
        default:
          return "ERR5";
      }
    case Option::balance:
    {
      const auto str_template = is_focus ? "<%+d>" : " %+d ";
      snprintf(tmp_format_str_buffer_, tmp_format_str_len_, str_template, persistent_data_ptr_->left_right_balance_db);

      return tmp_format_str_buffer_;
    }
    case Option::bias:
    {
      const auto str_template = is_focus ? "<%d%%>" : " %d%% ";
      snprintf(tmp_format_str_buffer_, tmp_format_str_len_, str_template, persistent_data_ptr_->bias);

      return tmp_format_str_buffer_;
    }
    case Option::rename_bal:
    case Option::rename_rca1:
    case Option::rename_rca2:
    case Option::rename_rca3:
    {
      return option_ctrl_ptr_->get_input_rename_value(get_audio_input_from_rename_option(option));
    }
    case Option::more_options:
      return {};
    case Option::back:
      return {};
    case Option::enum_length:
      return "ERR4";
    default:
      return "ERR6";
  };
}