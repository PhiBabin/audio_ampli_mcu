#ifndef INTERACTION_HANDLER_GUARD_H_
#define INTERACTION_HANDLER_GUARD_H_

#include "main_menu_view.h"
#include "options_view.h"
#include "state_machine.h"

/// Handles all physical input (buttons and encoders) and call the appropriate view
class InteractionHandler
{
public:
  InteractionHandler(
    OptionsView& option_view,
    MainMenuView& main_menu_view,
    VolumeController& volume_ctrl,
    StateMachine& state_machine,
    PioEncoder& option_encoder);

  void init();
  bool update();

  void on_menu_press();
  void on_mute_button_press();
  void on_power_button_press();
  void menu_change(const IncrementDir& dir);

private:
  bool update_selection();
  bool update_encoder();
  bool update_mute_button();

  // Reference to the option view
  OptionsView& option_view_;
  // Reference to the main menu view
  MainMenuView& main_menu_view_;
  /// Reference to the volume controler
  VolumeController& volume_ctrl_;
  // Reference to the state machine
  StateMachine& state_machine_;
  /// Reference to the quadrature encoder
  PioEncoder& option_encoder_;
  /// Toggle button for the selection
  ToggleButton select_button_;
  /// Toggle button for the mutting and power off
  ToggleButton mute_button_;

  /// Previous count of the encoder
  int32_t prev_encoder_count_{0};
};

#endif  // INTERACTION_HANDLER_GUARD_H_
