#ifndef MAIN_MENU_VIEW_GUARD_H_
#define MAIN_MENU_VIEW_GUARD_H_

#include "draw_primitives.h"
#include "option_enums.h"
#include "options_controller.h"

class MainMenuView
{
public:
  MainMenuView(
    OptionController& option_ctrl,
    VolumeController& volume_ctrl,
    PersistentData& persistent_data,
    StateMachine& state_machine,
    const LvFontWrapper& small_font,
    const LvFontWrapper& digit_font,
    const LvFontWrapper& regular_medium_font
  );

  void init();
  void menu_up();
  void menu_down();
  void menu_change(const IncrementDir& dir);
  void draw(Display& display, const bool has_state_changed);

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  void draw_volume(Display& display, const bool has_state_changed);
  void draw_audio_inputs(Display& display, const bool has_state_changed);
  void draw_left_right_bal_indicator(Display& display, const bool has_state_changed);

  // Reference to the option controler
  OptionController& option_ctrl_;
  /// Reference to the volume controler
  VolumeController& volume_ctrl_;
  // Reference to the persistent data
  PersistentData& persistent_data_;
  // Reference to the state machine
  StateMachine& state_machine_;
  // Font use to draw audio input options
  const LvFontWrapper& small_font_;
  // Font use to draw the volume's digit
  const LvFontWrapper& digit_font_;
  // Font use to draw the small .5 next to the volume's digit
  const LvFontWrapper& regular_medium_font_;

  // Cached previous state for partial redraw optimization
  bool prev_mute_state_{false};
  int32_t prev_volume_db_{0};
  AudioInput prev_audio_input_{AudioInput::rca_1};
};

#endif  // MAIN_MENU_VIEW_GUARD_H_
