#ifndef OPTIONS_VIEW_GUARD_H_
#define OPTIONS_VIEW_GUARD_H_

#include "draw_primitives.h"
#include "option_enums.h"
#include "options_controller.h"

#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

enum class MenuItemType : uint8_t
{
  // A menu item which is incremented with the menu button
  increment_item = 0,
  // A menu item which once focused can be incremented/decremented with the menu up/menu down
  focus_item,
  // A menu item which change menu
  change_menu,
  enum_length
};

struct MenuItem
{
  Option option{Option::back};
  const char* label;
  MenuItemType type{MenuItemType::increment_item};
  OptionMenuScreen menu_to_swap{OptionMenuScreen::main};
};

struct Menu
{
  OptionMenuScreen type;
  std::vector<MenuItem> items;
  Option selected_option{Option::back};

  void change_selected_item(const IncrementDir& dir);
  std::optional<std::reference_wrapper<const MenuItem>> try_get_selected_item() const;
  std::optional<size_t> try_get_selected_index() const;
};

class OptionsView
{
public:
  OptionsView(
    OptionController* option_ctrl_ptr,
    VolumeController* volume_ctrl_ptr,
    PersistentData* persistent_data_ptr,
    StateMachine* state_machine_ptr,
    const LvFontWrapper& font);
  void on_menu_press();
  void menu_up();
  void menu_down();
  void menu_change(const IncrementDir& dir);
  void draw(Display& display);

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  void draw_menu(Display& display);
  void draw_volume(Display& display);
  Menu& get_selected_menu();
  std::optional<const char*> string_format_option(const Option& option, const bool is_focus);

  // Non-owning pointer to the option controler
  OptionController* option_ctrl_ptr_;
  /// Non-owning pointer to the volume controler
  VolumeController* volume_ctrl_ptr_;
  // Non-owning pointer to the persistent data
  PersistentData* persistent_data_ptr_;
  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Font use to draw the menu (preferably small)
  const LvFontWrapper& font_;
  // Menu screens
  std::unordered_map<OptionMenuScreen, Menu> menus_;
  // Switch menu screen is selected
  OptionMenuScreen selected_menu_{OptionMenuScreen::main};
  // Do we have an option focused (aka the menu up/down change the option's value instead of moving thru the different
  // options)
  bool is_focus_{false};

  // Temporary buffer used for string_format_option().
  constexpr static size_t tmp_format_str_len_ = 100;
  char tmp_format_str_buffer_[tmp_format_str_len_];
};

#endif  // OPTIONS_VIEW_GUARD_H_