#include "options_controller.h"

#ifdef SIM
#include "sim/RP2040_PWM.h"
#else
#include "RP2040_PWM.h"
#endif

#include <type_traits>

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

// Frequency high enough to be filter out by the ampli
constexpr uint32_t pwm_frequency = 20000;

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
  PersistentData* persistent_data_ptr,
  PioEncoder* option_encoder_ptr,
  IoExpander* io_expander_ptr,
  VolumeController* volume_ctrl_ptr,
  const int select_button_pin,
  const int32_t tick_per_option,
  const int in_out_unipolar_pin,
  const int in_out_bal_unipolar_pin,
  const int set_low_gain_pin,
  const int out_bal_pin,
  const int preamp_out_pin,
  const int bias_out_pin)
  : state_machine_ptr_(state_machine_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , prev_encoder_count_(0)
  , tick_per_option_(tick_per_option)
  , option_encoder_ptr_(option_encoder_ptr)
  , select_button_pin_(select_button_pin)
  , in_out_unipolar_pin_(in_out_unipolar_pin)
  , in_out_bal_unipolar_pin_(in_out_bal_unipolar_pin)
  , set_low_gain_pin_(set_low_gain_pin)
  , out_bal_pin_(out_bal_pin)
  , preamp_out_pin_(preamp_out_pin)
  , bias_out_pin_(bias_out_pin)
  , io_expander_ptr_(io_expander_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
{
  PWM_Instance_ = new RP2040_PWM(bias_out_pin_, pwm_frequency, bias_);
  PWM_Instance_->setPWM(bias_out_pin_, pwm_frequency, bias_);
}

void OptionController::init()
{
  select_button_.setup(select_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  // Initialize GPIOs
  update_gpio();
}

void OptionController::update_gpio()
{

  // Set Low gain GPIO
  io_expander_ptr_->cache_write_pin(
    set_low_gain_pin_, persistent_data_ptr_->get_gain() == GainOption::low ? HIGH : LOW);

  // Set line out / preamp output GPIO
  io_expander_ptr_->cache_write_pin(
    preamp_out_pin_, persistent_data_ptr_->output_mode_value == OutputModeOption::line_out ? HIGH : LOW);

  const bool is_bal_input = persistent_data_ptr_->selected_audio_input == AudioInput::bal;
  const bool is_bal_output = persistent_data_ptr_->output_type_value == OutputTypeOption::bal;

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

  // Set PWM for bias
  PWM_Instance_->setPWM(bias_out_pin_, pwm_frequency, bias_);
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
    case Option::bias:
      return "BIAS";
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
      switch (persistent_data_ptr_->get_gain())
      {
        case GainOption::low:
          return "LOW";
        case GainOption::high:
          return "HIGH";
        default:
          return "ERR1";
      }
    case Option::output_mode:
      switch (persistent_data_ptr_->output_mode_value)
      {
        case OutputModeOption::phones:
          return "PHONES";
        case OutputModeOption::line_out:
          return "LINE OUT";
        default:
          return "ERR2";
      }
    case Option::output_type:
      switch (persistent_data_ptr_->output_type_value)
      {
        case OutputTypeOption::se:
          return "SE";
        case OutputTypeOption::bal:
          return "BAL";
        default:
          return "ERR3";
      }
    case Option::bias:
    {
      if (enabled_bias_scrolling_)
      {
        sprintf(bias_str_buffer_, "<%d>", bias_);
      }
      else
      {
        sprintf(bias_str_buffer_, " %d ", bias_);
      }

      return bias_str_buffer_;
    }
    case Option::back:
      return "";
    case Option::option_enum_length:
      return "ERR4";
    default:
      return "ERR6";
  };
}

bool OptionController::on_menu_press()
{
  if (state_machine_ptr_->get_state() == State::main_menu)
  {
    state_machine_ptr_->change_state(State::option_menu);
    return true;
  }
  constexpr auto volume_change = 14;
  switch (selected_option_)
  {
    case Option::gain:
      // When the gain is set from low to high, reduce the volume by 14db.
      if (persistent_data_ptr_->get_gain_mutable() == GainOption::low)
      {
        volume_ctrl_ptr_->set_volume_db(volume_ctrl_ptr_->get_volume_db() - volume_change);
      }
      else
      {
        volume_ctrl_ptr_->set_volume_db(volume_ctrl_ptr_->get_volume_db() + volume_change);
      }
      increment_enum(GainOption::enum_length, persistent_data_ptr_->get_gain_mutable());
      update_gpio();
      break;
    case Option::output_mode:
      increment_enum(OutputModeOption::enum_length, persistent_data_ptr_->output_mode_value);
      update_gpio();
      break;
    case Option::output_type:
      increment_enum(OutputTypeOption::enum_length, persistent_data_ptr_->output_type_value);
      update_gpio();
      break;
    case Option::bias:
      enabled_bias_scrolling_ = !enabled_bias_scrolling_;
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

bool OptionController::update_selection()
{
  const bool prev_select_state = select_button_.get_state();
  unsigned long now = millis();
  select_button_.process(now);
  if (select_button_.get_state() == prev_select_state)
  {
    return false;
  }

  return on_menu_press();
}

bool OptionController::update()
{
  bool has_changed = update_selection();
  has_changed |= update_encoder();
  return has_changed;
}

void OptionController::menu_up()
{
  increment_enum(Option::option_enum_length, selected_option_);
}
void OptionController::menu_down()
{
  decrement_enum(Option::option_enum_length, selected_option_);
}

bool OptionController::update_encoder()
{
  constexpr uint8_t bias_increment = 5;
  const int32_t current_count = option_encoder_ptr_->getCount();

  if (state_machine_ptr_->get_state() == State::main_menu)
  {
    prev_encoder_count_ = current_count;
    return false;
  }

  const bool is_scroll_for_bias = selected_option_ == Option::bias && enabled_bias_scrolling_;

  if (current_count - prev_encoder_count_ > tick_per_option_)
  {
    if (is_scroll_for_bias)
    {
      bias_ += bias_increment;
      if (bias_ > 100)
      {
        bias_ = 100;
      }
      update_gpio();
    }
    else
    {
      decrement_enum(Option::option_enum_length, selected_option_);
    }
    prev_encoder_count_ = current_count;
    return true;
  }
  if (-tick_per_option_ > current_count - prev_encoder_count_)
  {
    if (is_scroll_for_bias)
    {
      if (bias_ < bias_increment)
      {
        bias_ = 0;
      }
      else
      {
        bias_ -= bias_increment;
      }
      update_gpio();
    }
    else
    {
      increment_enum(Option::option_enum_length, selected_option_);
    }
    prev_encoder_count_ = current_count;
    return true;
  }
  return false;
}