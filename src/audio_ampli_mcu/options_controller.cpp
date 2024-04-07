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
  PioEncoder* option_encoder_ptr,
  const int select_button_pin,
  const int32_t tick_per_option)
  : state_machine_ptr_(state_machine_ptr)
  , select_button_pin_(select_button_pin)
  , prev_encoder_count_(0)
  , tick_per_option_(tick_per_option)
  , option_encoder_ptr_(option_encoder_ptr)
{
}

void OptionController::init()
{
  select_button_.setup(select_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
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
    case Option::output:
      return "OUTPUT";
    case Option::lfe_channel:
      return "LFE";
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
    case Option::output:
      switch (output_value_)
      {
        case OutputOption::jack:
          return "JACK";
        case OutputOption::bal:
          return "BAL";
        case OutputOption::preamp:
          return "PREAMP";
        default:
          return "ERR2";
      }
    case Option::lfe_channel:
      switch (lfe_value_)
      {
        case LowFrequencyEffectOption::off:
          return "OFF";
        case LowFrequencyEffectOption::on:
          return "ON";
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
      break;
    case Option::output:
      increment_enum(OutputOption::enum_length, output_value_);
      break;
    case Option::lfe_channel:
      increment_enum(LowFrequencyEffectOption::enum_length, lfe_value_);
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