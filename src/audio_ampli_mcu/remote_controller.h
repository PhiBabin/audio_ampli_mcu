#ifndef REMOTE_CONTROL_GUARD_H_
#define REMOTE_CONTROL_GUARD_H_

// Apple remote use the extended NEC protocol
#define USE_EXTENDED_NEC_PROTOCOL
#define ENABLE_NEC2_REPEATS

#include "audio_input_controller.h"
#include "config.h"
#include "options_controller.h"
#include "volume_controller.h"

#include <functional>
#include <unordered_map>

/// Receive and decode command from IR remote.
/// Right now it only supports the Apple Remote 1294.
class RemoteController
{
public:
  // Constructor
  RemoteController(
    StateMachine* state_machine_ptr,
    OptionController* option_ctrl_ptr,
    AudioInputController* audio_input_ctrl_ptr,
    VolumeController* volume_ctrl_ptr);

  // Init IR remote interrupt pins
  void init();
  bool decode_command();

private:
  // Handles based on button pressed
  void handle_up();
  void handle_down();
  void handle_vol_down();
  void handle_vol_up();
  void handle_mute();
  void handle_power_on_off();
  void handle_menu();
  void handle_select();

  // How much the remote change the volume per button press
  static const auto volume_change = 1;
  static const auto max_delay_between_repeats_ms = 800;

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Non-owning pointer to the option controler
  OptionController* option_ctrl_ptr_;
  // Non-owning pointer to the option controler
  AudioInputController* audio_input_ctrl_ptr_;
  /// Non-owning pointer to the volume controler
  VolumeController* volume_ctrl_ptr_;

  using RemoteCallbacks = std::unordered_map<uint16_t, std::function<void()>>;

  std::unordered_map<uint16_t, RemoteCallbacks> remotes_mapping_;

  uint16_t last_valid_addr_{0};
  uint16_t last_valid_cmd_{0};
  int last_valid_reception_{0};
  uint8_t repeat_count_{0};
};
#endif  // REMOTE_CONTROL_GUARD_H_