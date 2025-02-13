#ifndef OPTIONS_CTRL_GUARD_H_
#define OPTIONS_CTRL_GUARD_H_

#ifdef SIM
#include "sim/pio_encoder.h"
#include "sim/toggle_button.h"
#else
#include "pio_encoder.h"
#include "toggle_button.h"
#endif

#include "audio_input_controller.h"
#include "io_expander.h"
#include "option_enums.h"
#include "persistent_data.h"
#include "state_machine.h"
#include "volume_controller.h"

// Forward declaration
class RP2040_PWM;

/// Convert option to a human readable string.
AudioInput get_audio_input_from_rename_option(const Option option);

struct OptionContollerPins
{
  const std::array<pin_size_t, 4> iox_gpio_pin_audio_in_select;
  const int in_out_unipolar_pin;
  const int in_out_bal_unipolar_pin;
  const int set_low_gain_pin;
  const int out_bal_pin;
  const int preamp_out_pin;
  const int bias_out_pin;
  const int out_se_pin;
  const int out_lfe_bal_pin;
  const int out_lfe_se_pin;
  const int trigger_12v;
};

class ValueControlerInterface
{
  virtual char* get_string() = 0;
};

class OptionController
{
public:
  // Construtor
  OptionController(
    StateMachine* state_machine_ptr,
    PersistentData* persistent_data_ptr,
    IoExpander* io_expander_ptr,
    VolumeController* volume_ctrl_ptr,
    const pin_size_t bias_out_pin,
    const int power_enable_pin,
    OptionContollerPins pins);

  // Init GPIO pins
  void init();

  void increment_option(const Option& option, const IncrementDir& increment_dir);

  std::optional<const char*> get_input_rename_value(const AudioInput& audio_input) const;

  void update_gpio();

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  constexpr static uint8_t bias_increment = 5;
  constexpr static int8_t left_right_balance_range = 5;

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Non-owning pointer to the persistent data
  PersistentData* persistent_data_ptr_;

  // PWM pin that control the bias level.
  pin_size_t bias_out_pin_;

  // Output pin to turn power on/off
  pin_size_t power_enable_pin_;

  // The various pins on the IO expander that are controled by the options
  OptionContollerPins pins_;

  /// Non-owning pointer to the io expander
  IoExpander* io_expander_ptr_;

  /// Non-owning pointer to the volume controler
  VolumeController* volume_ctrl_ptr_;

  RP2040_PWM* PWM_Instance_;
};

#endif  // OPTIONS_CTRL_GUARD_H_