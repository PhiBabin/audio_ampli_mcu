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
  // Just a way to display text
  text,
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
  Menu(const OptionMenuScreen& _type, std::vector<MenuItem> _items, bool take_last_item = true);
  void change_selected_item(const IncrementDir& dir);
  std::optional<std::reference_wrapper<const MenuItem>> try_get_selected_item() const;

  const char* get_label() const;

  OptionMenuScreen type;
  std::vector<MenuItem> items;
  std::optional<size_t> maybe_selected_index;
};

class OptionsView
{
public:
  OptionsView(
    OptionController& option_ctrl,
    VolumeController& volume_ctrl,
    PersistentData& persistent_data,
    StateMachine& state_machine,
    const LvFontWrapper& font,
    const LvFontWrapper& medium_font,
    const LvFontWrapper& large_font);

  void init();
  void on_menu_press();
  void menu_up();
  void menu_down();
  void menu_change(const IncrementDir& dir);
  void draw(Display& display, const bool has_state_changed);

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  void draw_menu(Display& display, const bool has_state_changed);
  void draw_volume(Display& display, const bool has_state_changed);
  Menu& get_selected_menu();
  std::optional<const char*> string_format_option(const Option& option, const bool is_focus);

  // Duration of the changing option animation
  static constexpr uint32_t slice_duration_ms = 300;

  // Whether we are using the large size menu theme or the small size
  bool use_large_ui_{true};

  // Reference to the option controler
  OptionController& option_ctrl_;
  /// Reference to the volume controler
  VolumeController& volume_ctrl_;
  // Reference to the persistent data
  PersistentData& persistent_data_;
  // Reference to the state machine
  StateMachine& state_machine_;
  // Font use to draw the menu (preferably small)
  const LvFontWrapper& font_;
  // Font use to draw the menu titles
  const LvFontWrapper& medium_font_;
  // Font use to draw the current option's value
  const LvFontWrapper& large_font_;
  // Menu screens
  std::unordered_map<OptionMenuScreen, Menu> menus_;
  // Switch menu screen is selected
  OptionMenuScreen selected_menu_{OptionMenuScreen::main};
  // Do we have an option focused? (aka the menu up/down change the option's value instead of moving thru the different
  // options)
  bool is_focus_{false};
  // Force partial redraw on button press
  bool on_button_press_{false};

  // Page counter buffer (reused each frame)
  char page_count_buffer_[12];

  // Temporary buffer used for string_format_option().
  constexpr static size_t tmp_format_str_len_ = 100;
  char tmp_format_str_buffer_[tmp_format_str_len_];

  // Horizontal slide transition when switching between menu items.
  struct MenuSlideState
  {
    bool active{false};
    IncrementDir dir{IncrementDir::increment};
    std::optional<std::reference_wrapper<const MenuItem>> maybe_old_item;
    std::optional<std::reference_wrapper<const MenuItem>> maybe_new_item;
  };
  MenuSlideState menu_slide_;

  // Horizontal slide transition when the option value changes.
  struct ValueSlideState
  {
    bool active{false};
    IncrementDir dir{IncrementDir::increment};
    // The old text needs to be copied, because it might point to a temporary buffer
    // which get overidden when the text is updated.
    char old_text[50];
    // The new text value doesn't need to be copied
    const char* new_text{nullptr};
  };
  ValueSlideState value_slide_{};

};

#endif  // OPTIONS_VIEW_GUARD_H_
