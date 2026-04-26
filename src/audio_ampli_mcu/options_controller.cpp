#include "options_controller.h"

#ifdef SIM
#include "sim/RP2040_PWM.h"
#else
#include "RP2040_PWM.h"
#endif

#include <type_traits>

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

// Frequency high enough to be filter out by the ampli
constexpr uint32_t pwm_frequency = 100000;

template <typename T>
void increment_enum(const T& max_enum_value, T& enum_value_out)
{
  using IntegerType = typename std::underlying_type<T>::type;
  enum_value_out =
    static_cast<T>((static_cast<IntegerType>(enum_value_out) + 1) % static_cast<IntegerType>(max_enum_value));
}

template <typename T>
void decrement_enum(const T& max_enum_value, T& enum_value_out)
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
void change_enum(const T& max_enum_value, T& enum_value_out, const IncrementDir& dir)
{
  if (dir == IncrementDir::increment)
  {
    increment_enum(max_enum_value, enum_value_out);
  }
  else
  {
    decrement_enum(max_enum_value, enum_value_out);
  }
}

template <typename T>
void change_integer_within_range(
  T& value, const T& min_value, const T& max_value, const T& increment, const IncrementDir& dir)
{
  if (dir == IncrementDir::increment)
  {
    if (value > max_value - increment)
    {
      value = max_value;
    }
    else
    {
      value += increment;
    }
  }
  else
  {
    if (value < min_value + increment)
    {
      value = min_value;
    }
    else
    {
      value -= increment;
    }
  }
}

OptionController::OptionController(
  StateMachine* state_machine_ptr,
  PersistentData* persistent_data_ptr,
  VolumeController* volume_ctrl_ptr,
  GpioHandler* gpio_handler_ptr)
  : state_machine_ptr_(state_machine_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , gpio_handler_ptr_(gpio_handler_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
{
  PWM_Instance_ = new RP2040_PWM(pin_out::bias_pwm.pin, pwm_frequency, persistent_data_ptr_->bias);
  PWM_Instance_->setPWM(pin_out::bias_pwm.pin, pwm_frequency, persistent_data_ptr_->bias);
  prev_bias_ = persistent_data_ptr_->bias;
}

void OptionController::init()
{
  gpio_handler_ptr_->cache_init_output(pin_out::power_enable, LOW);
  gpio_handler_ptr_->apply();
  // select_button_.setup(select_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  // Initialize GPIOs
  update_gpio();
}

void OptionController::update_gpio()
{
  // Basically all non-phono GPIOs
  update_io_expander_gpio();

  if (has_phono_card())
  {
    update_phono_gpio();
  }
  // Set PWM for bias
  if (prev_bias_ != persistent_data_ptr_->bias)
  {
    PWM_Instance_->setPWM(pin_out::bias_pwm.pin, pwm_frequency, persistent_data_ptr_->bias);
    prev_bias_ = persistent_data_ptr_->bias;
  }
}

void OptionController::update_io_expander_gpio()
{
  // Set audio input
  const auto& audio_input = persistent_data_ptr_->selected_audio_input;
  for (uint8_t i = 0; i < pin_out::audio_input_pins.size(); ++i)
  {
    const auto& pin = pin_out::audio_input_pins[i];
    const bool is_selected = i == static_cast<uint8_t>(audio_input);
    gpio_handler_ptr_->cache_write_pin(pin, is_selected ? HIGH : LOW);
  }

  // Enable phono in if the RCA3 input is selected
  const int in_phono_pin = persistent_data_ptr_->selected_audio_input == AudioInput::rca_3 ? HIGH : LOW;
  gpio_handler_ptr_->cache_write_pin(pin_out::in_phono, in_phono_pin);

  // Set Low gain GPIO
  gpio_handler_ptr_->cache_write_pin(
    pin_out::set_low_gain, persistent_data_ptr_->get_gain() == GainOption::low ? HIGH : LOW);

  // Set line out / preamp output GPIO
  gpio_handler_ptr_->cache_write_pin(
    pin_out::preamp_out, persistent_data_ptr_->output_mode_value == OutputModeOption::line_out ? HIGH : LOW);

  const bool is_bal_input = persistent_data_ptr_->selected_audio_input == AudioInput::bal;
  const bool is_bal_output = persistent_data_ptr_->output_type_value == OutputTypeOption::bal;
  const bool is_lfe_enable = persistent_data_ptr_->sufwoofer_enable_value == OnOffOption::on;
  const bool is_mute = volume_ctrl_ptr_->is_muted();

  // This is an expression of the look up table
  // Note: input balance is set by the audio input controler
  const int output_bal_value = is_bal_output && !is_mute ? HIGH : LOW;
  const int output_se_value = !is_bal_output && !is_mute ? HIGH : LOW;
  const int in_out_unipolar_value = is_bal_input && !is_bal_output ? HIGH : LOW;
  const int in_out_bal_unipolar_value = !is_bal_input && is_bal_output ? HIGH : LOW;

  // If the output is BAL, LFE/subwoofer will use the SE and vice-versa.
  const int out_lfe_bal = is_lfe_enable && !is_bal_output && !is_mute ? HIGH : LOW;
  const int out_lfe_se = is_lfe_enable && is_bal_output && !is_mute ? HIGH : LOW;

  // Update GPIO accordingly
  gpio_handler_ptr_->cache_write_pin(pin_out::out_bal, output_bal_value);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_se, output_se_value);
  gpio_handler_ptr_->cache_write_pin(pin_out::in_out_unipolar, in_out_unipolar_value);
  gpio_handler_ptr_->cache_write_pin(pin_out::in_out_bal_unipolar, in_out_bal_unipolar_value);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_lfe_bal, out_lfe_bal);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_lfe_se, out_lfe_se);

  // Actually apply the change to the GPIOs
  gpio_handler_ptr_->apply();
}

void OptionController::update_phono_gpio()
{
  // Gain
  int out_gain_0 = LOW;
  int out_gain_1 = LOW;
  int out_gain_2 = LOW;
  if (persistent_data_ptr_->phono_mode_option == PhonoMode::mm)
  {
    switch (persistent_data_ptr_->phono_mm_gain)
    {
      case MMPhonoGain::gain_40dB:
        out_gain_0 = LOW;
        out_gain_1 = HIGH;
        out_gain_2 = LOW;
        break;
      case MMPhonoGain::gain_45dB:
        out_gain_0 = LOW;
        out_gain_1 = LOW;
        out_gain_2 = HIGH;
        break;
      case MMPhonoGain::gain_50dB:
        out_gain_0 = LOW;
        out_gain_1 = LOW;
        out_gain_2 = LOW;
        break;
      case MMPhonoGain::enum_length:
        break;
    }
  }
  else
  {
    switch (persistent_data_ptr_->phono_mc_gain)
    {
      case MCPhonoGain::gain_55dB:
        out_gain_0 = HIGH;
        out_gain_1 = HIGH;
        out_gain_2 = LOW;
        break;
      case MCPhonoGain::gain_60dB:
        out_gain_0 = HIGH;
        out_gain_1 = LOW;
        out_gain_2 = HIGH;
        break;
      case MCPhonoGain::gain_65dB:
        out_gain_0 = HIGH;
        out_gain_1 = LOW;
        out_gain_2 = LOW;
        break;
      case MCPhonoGain::enum_length:
        break;
    }
  }
  gpio_handler_ptr_->cache_write_pin(pin_out::out_gain_0, out_gain_0);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_gain_1, out_gain_1);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_gain_2, out_gain_2);

  // Resistance Load
  auto resistance_load_enum = persistent_data_ptr_->phono_resistance_load;
  if (persistent_data_ptr_->phono_mode_option == PhonoMode::mm)
  {
    resistance_load_enum = PhonoResistanceLoad::r_47k;
  }
  // Enums's integer value matches the 3bits of the IO output
  const auto out_resistance_load = static_cast<uint8_t>(resistance_load_enum);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_res_0, (out_resistance_load >> 0) & 1);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_res_1, (out_resistance_load >> 1) & 1);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_res_2, (out_resistance_load >> 2) & 1);

  // Capacity Load
  auto capacitance_load_enum = persistent_data_ptr_->phono_capacitance_load;
  if (persistent_data_ptr_->phono_mode_option == PhonoMode::mc)
  {
    capacitance_load_enum = PhonoCapacitanceLoad::c_0f;
  }
  // Enums's integer value matches the 3bits of the IO output
  const auto out_capacitance_load = static_cast<uint8_t>(capacitance_load_enum);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_cap_0, (out_capacitance_load >> 0) & 1);
  gpio_handler_ptr_->cache_write_pin(pin_out::out_cap_1, (out_capacitance_load >> 1) & 1);

  // Rumble filter
  const int out_rumble_filter = persistent_data_ptr_->phono_rumble_filter == OnOffOption::on ? HIGH : LOW;
  gpio_handler_ptr_->cache_write_pin(pin_out::out_rumble_filter, out_rumble_filter);

  // Actually apply the change to the GPIOs
  gpio_handler_ptr_->apply();
}

AudioInput get_audio_input_from_rename_option(const Option option)
{
  switch (option)
  {
    case Option::rename_bal:
      return AudioInput::bal;
    case Option::rename_rca1:
      return AudioInput::rca_1;
    case Option::rename_rca2:
      return AudioInput::rca_2;
    case Option::rename_rca3:
      return AudioInput::rca_3;
    default:
      return AudioInput::bal;
  }
}

const char* audio_input_to_string(const AudioInput audio_in)
{
  switch (audio_in)
  {
    case AudioInput::rca_1:
      return "RCA 1";
    case AudioInput::rca_2:
      return "RCA 2";
    case AudioInput::rca_3:
      return "RCA 3";
    case AudioInput::bal:
      return "BAL";
    default:
      return "INVALID";
  }
}

std::optional<const char*> OptionController::get_input_rename_value(const AudioInput& audio_input)
{
  if (has_phono_card() && audio_input == AudioInput::rca_3)
  {
    return "PHONO";
  }
  // Get the name alias
  const auto& name_alias = persistent_data_ptr_->get_per_audio_input_data(audio_input).name_alias;
  switch (name_alias)
  {
    case InputNameAliasOption::no_alias:
      return audio_input_to_string(audio_input);
      break;
    case InputNameAliasOption::dac:
      return "DAC";
    case InputNameAliasOption::cd:
      return "CD";
    case InputNameAliasOption::phono:
      return "PHONO";
    case InputNameAliasOption::tuner:
      return "TUNER";
    case InputNameAliasOption::aux:
      return "AUX";
    case InputNameAliasOption::stream:
      return "STREAM";
    case InputNameAliasOption::enum_length:
      return "ERR1";
  }
  return "ERR2";
}

void OptionController::power_off()
{
  // 1) Set the volume to mute
  volume_ctrl_ptr_->set_mute(true);
  volume_ctrl_ptr_->set_gpio_based_on_volume();
  update_gpio();

  // 2) wait 50ms to make sure that the volume is applied
  delay(50);

  // 3) Power off
  gpio_handler_ptr_->write_pin(pin_out::power_enable, LOW);

  // 4) Wait for power off to be applied
  delay(500);

  // 5) Turn off external power
  gpio_handler_ptr_->write_pin(pin_out::trigger_12v, LOW);
}

void OptionController::power_on()
{
  // 1) Power off
  gpio_handler_ptr_->write_pin(pin_out::power_enable, LOW);

  // 2) Mute output
  volume_ctrl_ptr_->set_mute(true);
  volume_ctrl_ptr_->set_gpio_based_on_volume();
  update_gpio();

  // 3) wait 50ms to make sure that the volume is applied
  delay(50);

  // 4) Power on
  gpio_handler_ptr_->write_pin(pin_out::power_enable, HIGH);

  // 5) Wait for power on to be applied
  delay(500);

  // 6) Unmute
  volume_ctrl_ptr_->set_mute(false);
  volume_ctrl_ptr_->set_gpio_based_on_volume();
  update_gpio();

  // 7) Turn on external power
  gpio_handler_ptr_->write_pin(pin_out::trigger_12v, HIGH);
}

void OptionController::increment_option(const Option& option, const IncrementDir& increment_dir)
{
  switch (option)
  {
    case Option::gain:
    {
      constexpr auto volume_change = 14;
      // When the gain is set from low to high, reduce the volume by 14db.
      if (persistent_data_ptr_->get_gain_mutable() == GainOption::low)
      {
        volume_ctrl_ptr_->set_volume_db(volume_ctrl_ptr_->get_volume_db() - volume_change);
      }
      else
      {
        volume_ctrl_ptr_->set_volume_db(volume_ctrl_ptr_->get_volume_db() + volume_change);
      }
      change_enum(GainOption::enum_length, persistent_data_ptr_->get_gain_mutable(), increment_dir);
      break;
    }
    case Option::output_mode:
      change_enum(OutputModeOption::enum_length, persistent_data_ptr_->output_mode_value, increment_dir);
      break;
    case Option::output_type:
      change_enum(OutputTypeOption::enum_length, persistent_data_ptr_->output_type_value, increment_dir);
      break;
    case Option::subwoofer:
      change_enum(OnOffOption::enum_length, persistent_data_ptr_->sufwoofer_enable_value, increment_dir);
      break;
    case Option::audio_input:
      change_enum(AudioInput::enum_length, persistent_data_ptr_->selected_audio_input, increment_dir);
      break;
    case Option::bias:
      Serial.println(persistent_data_ptr_->bias);
      change_integer_within_range(persistent_data_ptr_->bias, uint8_t{0}, uint8_t{100}, bias_increment, increment_dir);
      break;
    case Option::balance:
      change_integer_within_range(
        persistent_data_ptr_->left_right_balance_db,
        int8_t{-left_right_balance_range},
        left_right_balance_range,
        int8_t{1},
        increment_dir);
      break;
    case Option::rename_bal:
    case Option::rename_rca1:
    case Option::rename_rca2:
    case Option::rename_rca3:
    {
      const auto audio_input = get_audio_input_from_rename_option(option);
      // Get the name alias
      auto& name_alias = persistent_data_ptr_->get_per_audio_input_data_mutable(audio_input).name_alias;
      change_enum(InputNameAliasOption::enum_length, name_alias, increment_dir);
      break;
    }
    case Option::more_options:
      break;
    case Option::phono_mode:
      change_enum(PhonoMode::enum_length, persistent_data_ptr_->phono_mode_option, increment_dir);
      break;
    case Option::mute_channel:
      change_enum(MuteChannel::enum_length, persistent_data_ptr_->mute_channel, increment_dir);
      break;
    case Option::phono_gain:
      if (persistent_data_ptr_->phono_mode_option == PhonoMode::mm)
      {
        change_enum(MMPhonoGain::enum_length, persistent_data_ptr_->phono_mm_gain, increment_dir);
      }
      else
      {
        change_enum(MCPhonoGain::enum_length, persistent_data_ptr_->phono_mc_gain, increment_dir);
      }
      break;
    case Option::resistance_load:
      if (persistent_data_ptr_->phono_mode_option == PhonoMode::mc)
      {
        change_enum(PhonoResistanceLoad::enum_length, persistent_data_ptr_->phono_resistance_load, increment_dir);
      }
      break;
    case Option::capacitance_load:
      if (persistent_data_ptr_->phono_mode_option == PhonoMode::mm)
      {
        change_enum(PhonoCapacitanceLoad::enum_length, persistent_data_ptr_->phono_capacitance_load, increment_dir);
      }
      break;
    case Option::rumble_filter:
      change_enum(OnOffOption::enum_length, persistent_data_ptr_->phono_rumble_filter, increment_dir);
      break;
    case Option::back:
      break;
    case Option::enum_length:
      break;
    default:
      break;
  };
  update_gpio();
  volume_ctrl_ptr_->on_option_change();
}

bool OptionController::has_phono_card()
{
  return gpio_handler_ptr_->is_module_connected(GpioModule::io_expander_phono);
}