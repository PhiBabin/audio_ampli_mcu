#ifndef AUDIO_INPUT_CTRL_GUARD_H_
#define AUDIO_INPUT_CTRL_GUARD_H_

#ifdef SIM
#include "sim/pio_encoder.h"
#else
#include "pio_encoder.h"
#endif

#include "audio_input_enums.h"
#include "io_expander.h"
#include "persistent_data.h"
#include "state_machine.h"

#include <array>

const char* audio_input_to_string(const AudioInput audio_in);

class AudioInputController
{
public:
  // Construtor
  AudioInputController(
    StateMachine* state_machine_ptr,
    PersistentData* persistent_data_ptr,
    PioEncoder* audio_in_encoder_ptr,
    IoExpander* io_expander_ptr,
    const std::array<pin_size_t, 4> iox_gpio_pin_audio_in_select,
    const int32_t tick_per_audio_in);

  // Init GPIO pins
  void init();

  // Return current volume as a value in the 0-100% range
  AudioInput get_audio_input() const;

  // Read encoder and update audio input state
  // return true on change in audio input
  bool update();

  void menu_up();
  void menu_down();

private:
  // Set GPIO based on state
  void set_gpio();

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Non-owning pointer to the persistent data
  PersistentData* persistent_data_ptr_;
  // IO expander GPIO pins for each of the AUX 1, AUX 2, AUX 3 and BAL
  std::array<pin_size_t, 4> iox_gpio_pin_audio_in_select_;
  // Current audio input state
  // AudioInput audio_input_;
  /// Previous count of the encoder
  int32_t prev_encoder_count_;
  /// Number of encoder tick per audio in
  int32_t tick_per_audio_in_;
  ///  Non-owning pointer to the quadrature encoder
  PioEncoder* audio_in_encoder_ptr_;
  /// Non-owning pointer to the io expander
  IoExpander* io_expander_ptr_;
};

#endif  // AUDIO_INPUT_CTRL_GUARD_H_