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
const char* option_to_string(const Option option);
const char* advance_option_to_string(const AdvanceMenuOption option);

struct OptionContollerPins
{
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
    PioEncoder* option_encoder_ptr,
    IoExpander* io_expander_ptr,
    VolumeController* volume_ctrl_ptr,
    const pin_size_t select_button_pin,
    const pin_size_t bias_out_pin,
    const int power_enable_pin,
    const int32_t tick_per_option,
    OptionContollerPins pins);

  // Init GPIO pins
  void init();

  OptionMenuScreen get_current_menu_screen() const;
  size_t get_num_options() const;
  const char* get_option_label_string(const uint8_t& option_num);
  std::optional<const char*> get_option_value_string(const uint8_t& option_num);
  std::optional<const char*> get_input_rename_value(const AudioInput& audio_input) const;
  bool is_option_selected(const uint8_t option_num) const;

  // Read encoder, update state and set GPIO pin that set the volume.
  // return true on change in volume or mute status
  bool update();

  void update_gpio();
  void on_audio_input_change();

  bool on_menu_press();
  void menu_up();
  void menu_down();

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

private:
  std::optional<const char*> get_main_option_value_string(const Option& option);
  std::optional<const char*> get_advance_menu_option_value_string(const AdvanceMenuOption& option);
  bool update_selection();
  bool update_encoder();

  constexpr static uint8_t bias_increment = 5;
  constexpr static uint8_t left_right_balance_range = 5;

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Non-owning pointer to the persistent data
  PersistentData* persistent_data_ptr_;
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

  // Selected option
  Option selected_option_{Option::back};
  AdvanceMenuOption selected_advance_menu_option_{AdvanceMenuOption::back};
  OptionMenuScreen selected_screen_{OptionMenuScreen::main};

  constexpr static size_t bias_str_buffer_len_ = 10;
  char bias_str_buffer_[bias_str_buffer_len_];
  constexpr static size_t balance_str_buffer_len_ = 10;
  char balance_str_buffer_[balance_str_buffer_len_];

  bool enabled_bias_scrolling_{false};
  bool enabled_balance_scrolling_{false};
};

#endif  // OPTIONS_CTRL_GUARD_H_