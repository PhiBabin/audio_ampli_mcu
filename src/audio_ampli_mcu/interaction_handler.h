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
    OptionsView* option_view_ptr,
    MainMenuView* main_menu_view_ptr,
    VolumeController* volume_ctrl_ptr,
    StateMachine* state_machine_ptr,
    PioEncoder* option_encoder_ptr,
    const pin_size_t select_button_pin,
    const pin_size_t mute_button_pin);

  void init();
  bool update();

  void on_menu_press();
  void on_power_button_press();
  void menu_change(const IncrementDir& dir);

private:
  bool update_selection();
  bool update_encoder();
  bool update_mute_button();

  // Non-owning pointer to the option view
  OptionsView* option_view_ptr_;
  // Non-owning pointer to the main menu view
  MainMenuView* main_menu_view_ptr_;
  /// Non-owning pointer to the volume controler
  VolumeController* volume_ctrl_ptr_;
  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  /// Non-owning pointer to the quadrature encoder
  PioEncoder* option_encoder_ptr_;
  // Pin for the select toggle button
  pin_size_t select_button_pin_;
  // Pin for the mute toggle button and standby button
  pin_size_t mute_button_pin_;
  /// Toggle button for the selection
  ToggleButton select_button_;
  /// Toggle button for the mutting and power off
  ToggleButton mute_button_;

  /// Previous count of the encoder
  int32_t prev_encoder_count_{0};
};

#endif  // INTERACTION_HANDLER_GUARD_H_