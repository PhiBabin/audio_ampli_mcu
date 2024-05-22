#ifndef OPTIONS_CTRL_GUARD_H_
#define OPTIONS_CTRL_GUARD_H_

#ifdef SIM
#include "sim/pio_encoder.h"
#include "sim/toggle_button.h"
#else
#include "pio_encoder.h"
#include "toggle_button.h"
#endif

#include "state_machine.h"

#include "MCP23S17.h"

enum class Option : uint8_t
{
  gain = 0,
  output,
  lfe_channel,
  back,
  option_enum_length
};
enum class GainOption : uint8_t
{
  low = 0,
  high,
  enum_length
};

enum class OutputOption : uint8_t
{
  jack = 0,
  bal,
  preamp,
  enum_length
};

enum class LowFrequencyEffectOption : uint8_t
{
  off = 0,
  on,
  enum_length
};

const char* option_to_string(const Option option);

class OptionController
{
public:
  // Construtor
  OptionController(
    StateMachine* state_machine_ptr,
    PioEncoder* option_encoder_ptr,
    MCP23S17* io_expander_ptr,
    const int select_button_pin,
    const int32_t tick_per_option);

  // Init GPIO pins
  void init();

  Option get_selected_option() const;

  const char* get_option_value_string(const Option& option);

  //   // Get mutable gain option
  //   GainOption& get_gain_mutable();

  //   // Get mutable output option
  //   OutputOption& get_output_mutable();

  //   // Get mutable LFE option
  //   LowFrequencyEffectOption& get_lfe_mutable();

  // Read encoder, update state and set GPIO pin that set the volume.
  // return true on change in volume or mute status
  bool update();

private:
  bool update_selection();
  bool update_encoder();

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Pin for the mute toggle button
  pin_size_t select_button_pin_;
  /// Previous count of the encoder
  int32_t prev_encoder_count_;
  /// Number of encoder tick per audio in
  int32_t tick_per_option_;
  /// Non-owning pointer to the quadrature encoder
  PioEncoder* option_encoder_ptr_;
  /// Toggle button for the mutting
  ToggleButton select_button_;
  /// Non-owning pointer to the io expander
  MCP23S17* io_expander_ptr_;

  // Selected option
  Option selected_option_{Option::back};
  // Option value:
  GainOption gain_value_{GainOption::low};                             // TODO set in constructor
  OutputOption output_value_{OutputOption::jack};                      // TODO set in constructor
  LowFrequencyEffectOption lfe_value_{LowFrequencyEffectOption::off};  // TODO set in constructor
};

#endif  // OPTIONS_CTRL_GUARD_H_