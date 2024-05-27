#include "options_controller.h"

#include <type_traits>

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

template <typename T>
void increment_enum(const T& max_enum_value, T& enum_value_out)
{
  using IntegerType = typename std::underlying_type<T>::type;
  const auto max_enum_int = static_cast<IntegerType>(max_enum_value);
  uint8_t enum_int = static_cast<IntegerType>(enum_value_out);
  if (enum_int == 0)
  {
    enum_int = max_enum_int - 1;
  }
  else
  {
    --enum_int;
  }
  enum_value_out = static_cast<T>(enum_int);
}

template <typename T>
void decrement_enum(const T& max_enum_value, T& enum_value_out)
{
  using IntegerType = typename std::underlying_type<T>::type;
  enum_value_out =
    static_cast<T>((static_cast<IntegerType>(enum_value_out) + 1) % static_cast<IntegerType>(max_enum_value));
}

OptionController::OptionController(
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
  const int preamp_out_pin)
  : state_machine_ptr_(state_machine_ptr)
  , audio_input_ctrl_ptr_(audio_input_ctrl_ptr)
  , prev_encoder_count_(0)
  , tick_per_option_(tick_per_option)
  , option_encoder_ptr_(option_encoder_ptr)
  , select_button_pin_(select_button_pin)
  , in_out_unipolar_pin_(in_out_unipolar_pin)
  , in_out_bal_unipolar_pin_(in_out_bal_unipolar_pin)
  , set_low_gain_pin_(set_low_gain_pin)
  , out_bal_pin_(out_bal_pin)
  , preamp_out_pin_(preamp_out_pin)
  , io_expander_ptr_(io_expander_ptr)
{
}

void OptionController::init()
{
  select_button_.setup(select_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  // Initialize GPIOs
  update_gpio();
}

void OptionController::update_gpio()
{
  auto audio_input = audio_input_ctrl_ptr_->get_audio_input();

  // Set Low gain GPIO
  io_expander_ptr_->cache_write_pin(set_low_gain_pin_, gain_value_ == GainOption::low ? 1 : 0);

  // Set line out / preamp output GPIO
  io_expander_ptr_->cache_write_pin(preamp_out_pin_, output_mode_value_ == OutputModeOption::line_out ? 1 : 0);

  const bool is_bal_input = audio_input == AudioInput::bal;
  const bool is_bal_output = output_type_value_ == OutputTypeOption::bal;

  // This is an expression of the look up table
  // Note: input balance is set by the audio input controler
  const int output_bal_value = is_bal_output ? HIGH : LOW;
  const int in_out_unipolar_value = is_bal_input && !is_bal_output ? HIGH : LOW;
  const int in_out_bal_unipolar_value = !is_bal_input && is_bal_output ? HIGH : LOW;

  // Update GPIO accordingly
  io_expander_ptr_->cache_write_pin(out_bal_pin_, output_bal_value);
  io_expander_ptr_->cache_write_pin(in_out_unipolar_pin_, in_out_unipolar_value);
  io_expander_ptr_->cache_write_pin(in_out_bal_unipolar_pin_, in_out_bal_unipolar_value);

  // Actually apply the change to the GPIOs
  io_expander_ptr_->apply_write();
}

void OptionController::on_audio_input_change()
{
  update_gpio();
}

Option OptionController::get_selected_option() const
{
  return selected_option_;
}

const char* option_to_string(const Option option)
{
  switch (option)
  {
    case Option::gain:
      return "GAIN";
    case Option::output_mode:
      return "OUTPUT";
    case Option::output_type:
      return "TYPE";
    case Option::back:
      return "";
    case Option::option_enum_length:
      return "ERR5";
    default:
      return "ERR6";
  };
}

const char* OptionController::get_option_value_string(const Option& option)
{
  switch (option)
  {
    case Option::gain:
      switch (gain_value_)
      {
        case GainOption::low:
          return "LOW";
        case GainOption::high:
          return "HIGH";
        default:
          return "ERR1";
      }
    case Option::output_mode:
      switch (output_mode_value_)
      {
        case OutputModeOption::phones:
          return "PHONES";
        case OutputModeOption::line_out:
          return "LINE OUT";
        default:
          return "ERR2";
      }
    case Option::output_type:
      switch (output_type_value_)
      {
        case OutputTypeOption::se:
          return "SE";
        case OutputTypeOption::bal:
          return "BAL";
        default:
          return "ERR3";
      }
    case Option::back:
      return "";
    case Option::option_enum_length:
      return "ERR4";
    default:
      return "ERR6";
  };
}

bool OptionController::update_selection()
{
  const bool prev_select_state = select_button_.get_state();
  unsigned long now = millis();
  select_button_.process(now);
  if (select_button_.get_state() == prev_select_state)
  {
    return false;
  }

  if (state_machine_ptr_->get_state() == State::main_menu)
  {
    state_machine_ptr_->change_state(State::option_menu);
    return true;
  }
  switch (selected_option_)
  {
    case Option::gain:
      increment_enum(GainOption::enum_length, gain_value_);
      update_gpio();
      break;
    case Option::output_mode:
      increment_enum(OutputModeOption::enum_length, output_mode_value_);
      update_gpio();
      break;
    case Option::output_type:
      increment_enum(OutputTypeOption::enum_length, output_type_value_);
      update_gpio();
      break;
    case Option::back:
      state_machine_ptr_->change_state(State::main_menu);
      break;
    case Option::option_enum_length:
      break;
    default:
      break;
  };

  return true;
}

bool OptionController::update()
{
  bool has_changed = update_selection();
  has_changed |= update_encoder();
  return has_changed;
}

bool OptionController::update_encoder()
{
  const int32_t current_count = option_encoder_ptr_->getCount();

  if (state_machine_ptr_->get_state() == State::main_menu)
  {
    prev_encoder_count_ = current_count;
    return false;
  }

  if (current_count - prev_encoder_count_ > tick_per_option_)
  {
    decrement_enum(Option::option_enum_length, selected_option_);
    prev_encoder_count_ = current_count;
    return true;
  }
  if (-tick_per_option_ > current_count - prev_encoder_count_)
  {
    increment_enum(Option::option_enum_length, selected_option_);
    prev_encoder_count_ = current_count;
    return true;
  }
  return false;
}