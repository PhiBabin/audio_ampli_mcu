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
#include "state_machine.h"

enum class Option : uint8_t
{
  gain = 0,
  output_mode,
  output_type,
  back,
  option_enum_length
};
enum class GainOption : uint8_t
{
  low = 0,
  high,
  enum_length
};

enum class OutputModeOption : uint8_t
{
  phones = 0,
  line_out,
  enum_length
};

enum class OutputTypeOption : uint8_t
{
  se = 0,
  bal,
  enum_length
};
const char* option_to_string(const Option option);

class OptionController
{
public:
  // Construtor
  OptionController(
    StateMachine* state_machine_ptr,
    AudioInputController* audio_input_ctrl_ptr,
    PioEncoder* option_encoder_ptr,
    IoExpander* io_expander_ptr,
    const int select_button_pin,
    const int32_t tick_per_option,
    const int in_out_unipolar_pin,
    const int in_out_bal_unipolar_pin,
    const int set_low_gain_pin,
    const int out_bal_pin,
    const int preamp_out_pin);

  // Init GPIO pins
  void init();

  Option get_selected_option() const;

  const char* get_option_value_string(const Option& option);

  // Read encoder, update state and set GPIO pin that set the volume.
  // return true on change in volume or mute status
  bool update();

  void update_gpio();
  void on_audio_input_change();

private:
  bool update_selection();
  bool update_encoder();

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Non-owning pointer to the audio input controler
  AudioInputController* audio_input_ctrl_ptr_;
  /// Previous count of the encoder
  int32_t prev_encoder_count_;
  /// Number of encoder tick per audio in
  int32_t tick_per_option_;
  /// Non-owning pointer to the quadrature encoder
  PioEncoder* option_encoder_ptr_;
  /// Toggle button for the mutting
  ToggleButton select_button_;

  // Pin for the mute toggle button
  pin_size_t select_button_pin_;

  // The various pins that are controled by the options
  const int in_out_unipolar_pin_;
  const int in_out_bal_unipolar_pin_;
  const int set_low_gain_pin_;
  const int out_bal_pin_;
  const int preamp_out_pin_;

  /// Non-owning pointer to the io expander
  IoExpander* io_expander_ptr_;

  // Selected option
  Option selected_option_{Option::back};
  // Option value:
  GainOption gain_value_{GainOption::low};                        // TODO set in constructor
  OutputModeOption output_mode_value_{OutputModeOption::phones};  // TODO set in constructor
  OutputTypeOption output_type_value_{OutputTypeOption::se};      // TODO set in constructor
};

#endif  // OPTIONS_CTRL_GUARD_H_