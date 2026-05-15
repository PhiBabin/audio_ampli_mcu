#ifndef OPTIONS_CTRL_GUARD_H_
#define OPTIONS_CTRL_GUARD_H_

#ifdef SIM
#include "sim/pio_encoder.h"
#include "sim/toggle_button.h"
#else
#include "pio_encoder.h"
#include "toggle_button.h"
#endif

#include "gpio_handler.h"
#include "option_enums.h"
#include "persistent_data.h"
#include "state_machine.h"
#include "volume_controller.h"

#include <optional>

#ifdef SIM
#include "sim/RP2040_PWM.h"
#else
#include "RP2040_PWM.h"
#endif

/// Convert option to a human readable string.
AudioInput get_audio_input_from_rename_option(const Option option);

/// Convert a audio input enum into string.
const char* audio_input_to_string(const AudioInput audio_in);

class OptionController
{
public:
  // Constructor
  OptionController(
    StateMachine& state_machine,
    PersistentData& persistent_data,
    VolumeController& volume_ctrl,
    GpioHandler& gpio_handler);

  // Init GPIO pins
  void init();

  void increment_option(const Option& option, const IncrementDir& increment_dir);

  std::optional<const char*> get_input_rename_value(const AudioInput& audio_input);

  bool has_phono_card();

  void update_gpio();

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  constexpr static uint8_t bias_increment = 5;
  // In tenth-dB units (5 dB = 50 tenth-dB, 0.5dB step = 5 tenth-dB)
  constexpr static int8_t left_right_balance_range = 50;

  void update_io_expander_gpio();
  void update_phono_gpio();

  // Reference to the state machine
  StateMachine& state_machine_;
  // Reference to the persistent data
  PersistentData& persistent_data_;

  // Handler to read/write to GPIO from the pico or to IO expander
  GpioHandler& gpio_handler_;

  /// Reference to the volume controler
  VolumeController& volume_ctrl_;

  RP2040_PWM pwm_bias_;

  /// Previous bias set
  uint8_t prev_bias_;
};

#endif  // OPTIONS_CTRL_GUARD_H_
