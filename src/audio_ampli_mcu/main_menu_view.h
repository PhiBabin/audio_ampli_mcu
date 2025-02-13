#ifndef MAIN_MENU_VIEW_GUARD_H_
#define MAIN_MENU_VIEW_GUARD_H_

#include "draw_primitives.h"
#include "option_enums.h"
#include "options_controller.h"

class MainMenuView
{
public:
  MainMenuView(
    OptionController* option_ctrl_ptr,
    VolumeController* volume_ctrl_ptr,
    PersistentData* persistent_data_ptr,
    StateMachine* state_machine_ptr,
    const LvFontWrapper& small_font,
    const LvFontWrapper& digit_font);
  void menu_up();
  void menu_down();
  void menu_change(const IncrementDir& dir);
  void draw(Display& display);

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  void draw_volume(Display& display);
  void draw_audio_inputs(Display& display);
  void draw_left_right_bal_indicator(Display& display);
  // const char* get_audio_input_renamed_str(const AudioInput& audio_input) const;

  // Non-owning pointer to the option controler
  OptionController* option_ctrl_ptr_;
  /// Non-owning pointer to the volume controler
  VolumeController* volume_ctrl_ptr_;
  // Non-owning pointer to the persistent data
  PersistentData* persistent_data_ptr_;
  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Font use to draw audio input options
  const LvFontWrapper& small_font_;
  // Font use to draw the volume's digit
  const LvFontWrapper& digit_font_;
};

#endif  // MAIN_MENU_VIEW_GUARD_H_