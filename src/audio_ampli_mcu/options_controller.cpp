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

OptionController::OptionController(
  StateMachine* state_machine_ptr,
  PersistentData* persistent_data_ptr,
  PioEncoder* option_encoder_ptr,
  IoExpander* io_expander_ptr,
  VolumeController* volume_ctrl_ptr,
  const pin_size_t select_button_pin,
  const pin_size_t bias_out_pin,
  const int power_enable_pin,
  const int32_t tick_per_option,
  OptionContollerPins pins)
  : state_machine_ptr_(state_machine_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , prev_encoder_count_(0)
  , tick_per_option_(tick_per_option)
  , option_encoder_ptr_(option_encoder_ptr)
  , select_button_pin_(select_button_pin)
  , bias_out_pin_(bias_out_pin)
  , power_enable_pin_(power_enable_pin)
  , pins_(pins)
  , io_expander_ptr_(io_expander_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
{
  PWM_Instance_ = new RP2040_PWM(bias_out_pin_, pwm_frequency, persistent_data_ptr_->bias);
  PWM_Instance_->setPWM(bias_out_pin_, pwm_frequency, persistent_data_ptr_->bias);
}

void OptionController::init()
{
  pinMode(power_enable_pin_, OUTPUT);
  digitalWrite(power_enable_pin_, LOW);
  select_button_.setup(select_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  // Initialize GPIOs
  update_gpio();
}

void OptionController::update_gpio()
{
  // Set Low gain GPIO
  io_expander_ptr_->cache_write_pin(
    pins_.set_low_gain_pin, persistent_data_ptr_->get_gain() == GainOption::low ? HIGH : LOW);

  // Set line out / preamp output GPIO
  io_expander_ptr_->cache_write_pin(
    pins_.preamp_out_pin, persistent_data_ptr_->output_mode_value == OutputModeOption::line_out ? HIGH : LOW);

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
  io_expander_ptr_->cache_write_pin(pins_.out_bal_pin, output_bal_value);
  io_expander_ptr_->cache_write_pin(pins_.out_se_pin, output_se_value);
  io_expander_ptr_->cache_write_pin(pins_.in_out_unipolar_pin, in_out_unipolar_value);
  io_expander_ptr_->cache_write_pin(pins_.in_out_bal_unipolar_pin, in_out_bal_unipolar_value);
  io_expander_ptr_->cache_write_pin(pins_.out_lfe_bal_pin, out_lfe_bal);
  io_expander_ptr_->cache_write_pin(pins_.out_lfe_se_pin, out_lfe_se);

  // Actually apply the change to the GPIOs
  io_expander_ptr_->apply_write();

  // Set PWM for bias
  PWM_Instance_->setPWM(bias_out_pin_, pwm_frequency, persistent_data_ptr_->bias);
}

void OptionController::on_audio_input_change()
{
  update_gpio();
}

OptionMenuScreen OptionController::get_current_menu_screen() const
{
  return selected_screen_;
}

size_t OptionController::get_num_options() const
{
  if (selected_screen_ == OptionMenuScreen::main)
  {
    return static_cast<uint8_t>(Option::enum_length);
  }
  else
  {
    return static_cast<uint8_t>(AdvanceMenuOption::enum_length);
  }
}

const char* OptionController::get_option_label_string(const uint8_t& option_num)
{
  if (selected_screen_ == OptionMenuScreen::main)
  {
    if (option_num < static_cast<uint8_t>(Option::enum_length))
    {
      return option_to_string(static_cast<Option>(option_num));
    }
  }
  else
  {
    if (option_num < static_cast<uint8_t>(AdvanceMenuOption::enum_length))
    {
      return advance_option_to_string(static_cast<AdvanceMenuOption>(option_num));
    }
  }
  return "ERR44";
}

std::optional<const char*> OptionController::get_option_value_string(const uint8_t& option_num)
{
  if (selected_screen_ == OptionMenuScreen::main)
  {
    const auto option_enum = static_cast<Option>(option_num);

    if (option_num < static_cast<uint8_t>(Option::enum_length))
    {
      return get_main_option_value_string(option_enum);
    }
  }
  else
  {
    const auto option_enum = static_cast<AdvanceMenuOption>(option_num);
    if (option_num < static_cast<uint8_t>(AdvanceMenuOption::enum_length))
    {
      return get_advance_menu_option_value_string(option_enum);
    }
  }
  return "ERR55";
}

bool OptionController::is_option_selected(const uint8_t option_num) const
{
  if (selected_screen_ == OptionMenuScreen::main)
  {
    return static_cast<Option>(option_num) == selected_option_;
  }
  else
  {
    return static_cast<AdvanceMenuOption>(option_num) == selected_advance_menu_option_;
  }
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
    case Option::subwoofer:
      return "SUBWOOFER";
    case Option::balance:
      return "L/RIGHT BAL";
    case Option::more_options:
      return "MORE OPTIONS";
    case Option::back:
      return "BACK";
    case Option::enum_length:
      return "ERR5";
    default:
      return "ERR6";
  };
}

const char* advance_option_to_string(const AdvanceMenuOption option)
{
  switch (option)
  {
    case AdvanceMenuOption::bias:
      return "BIAS";
    case AdvanceMenuOption::rename_bal:
      return "RENAME BAL ";
    case AdvanceMenuOption::rename_rca1:
      return "RENAME RCA1";
    case AdvanceMenuOption::rename_rca2:
      return "RENAME RCA2";
    case AdvanceMenuOption::rename_rca3:
      return "RENAME RCA3";
    case AdvanceMenuOption::back:
      return "BACK";
    case AdvanceMenuOption::enum_length:
      return "ERR5";
    default:
      return "ERR6";
  };
}

AudioInput get_audio_input_from_advance_option(const AdvanceMenuOption option)
{
  switch (option)
  {
    case AdvanceMenuOption::rename_bal:
      return AudioInput::bal;
    case AdvanceMenuOption::rename_rca1:
      return AudioInput::rca_1;
    case AdvanceMenuOption::rename_rca2:
      return AudioInput::rca_2;
    case AdvanceMenuOption::rename_rca3:
      return AudioInput::rca_3;
    default:
      return AudioInput::bal;
  }
}

std::optional<const char*> OptionController::get_input_rename_value(const AudioInput& audio_input) const
{
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

std::optional<const char*> OptionController::get_advance_menu_option_value_string(const AdvanceMenuOption& option)
{
  switch (option)
  {
    case AdvanceMenuOption::bias:
    {
      const auto str_template = enabled_bias_scrolling_ ? "<%d%%>" : " %d%% ";
      snprintf(bias_str_buffer_, bias_str_buffer_len_, str_template, persistent_data_ptr_->bias);

      return bias_str_buffer_;
    }
    case AdvanceMenuOption::rename_bal:
    case AdvanceMenuOption::rename_rca1:
    case AdvanceMenuOption::rename_rca2:
    case AdvanceMenuOption::rename_rca3:
    {
      return get_input_rename_value(get_audio_input_from_advance_option(option));
    }
    case AdvanceMenuOption::back:
      return {};
    case AdvanceMenuOption::enum_length:
      return "ERR4";
    default:
      return "ERR6";
  }
}

std::optional<const char*> OptionController::get_main_option_value_string(const Option& option)
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
    case Option::subwoofer:
      switch (persistent_data_ptr_->sufwoofer_enable_value)
      {
        case OnOffOption::on:
          return "ON";
        case OnOffOption::off:
          return "OFF";
        default:
          return "ERR5";
      }
    case Option::balance:
    {
      if (enabled_balance_scrolling_)
      {
        snprintf(balance_str_buffer_, balance_str_buffer_len_, "<%+d>", persistent_data_ptr_->left_right_balance_db);
      }
      else
      {
        snprintf(balance_str_buffer_, balance_str_buffer_len_, " %+d ", persistent_data_ptr_->left_right_balance_db);
      }

      return balance_str_buffer_;
    }
    case Option::more_options:
      return {};
    case Option::back:
      return {};
    case Option::enum_length:
      return "ERR4";
    default:
      return "ERR6";
  };
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
  digitalWrite(power_enable_pin_, LOW);

  // 4) Wait for power off to be applied
  delay(500);

  // 5) Enter standby mode
  state_machine_ptr_->change_state(State::standby);

  // 6) Turn off external power
  io_expander_ptr_->cache_write_pin(pins_.trigger_12v, LOW);
  io_expander_ptr_->apply_write();
}

void OptionController::power_on()
{
  // 1) Power off
  digitalWrite(power_enable_pin_, LOW);

  // 2) Mute output
  volume_ctrl_ptr_->set_mute(true);
  volume_ctrl_ptr_->set_gpio_based_on_volume();
  update_gpio();

  // 3) wait 50ms to make sure that the volume is applied
  delay(50);

  // 4) Power on
  digitalWrite(power_enable_pin_, HIGH);

  // 5) Wait for power on to be applied
  delay(500);

  // 6) Unmute
  volume_ctrl_ptr_->set_mute(false);
  volume_ctrl_ptr_->set_gpio_based_on_volume();
  update_gpio();

  // 7) Exit standby mode
  state_machine_ptr_->change_state(State::main_menu);

  // 8) Turn on external power
  io_expander_ptr_->cache_write_pin(pins_.trigger_12v, HIGH);
  io_expander_ptr_->apply_write();
}

bool OptionController::on_menu_press()
{
  if (state_machine_ptr_->get_state() == State::main_menu)
  {
    state_machine_ptr_->change_state(State::option_menu);
    return true;
  }
  switch (selected_screen_)
  {
    case OptionMenuScreen::main:
      switch (selected_option_)
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
          increment_enum(GainOption::enum_length, persistent_data_ptr_->get_gain_mutable());
          update_gpio();
          break;
        }
        case Option::output_mode:
          increment_enum(OutputModeOption::enum_length, persistent_data_ptr_->output_mode_value);
          update_gpio();
          break;
        case Option::output_type:
          increment_enum(OutputTypeOption::enum_length, persistent_data_ptr_->output_type_value);
          update_gpio();
          break;
        case Option::subwoofer:
          increment_enum(OnOffOption::enum_length, persistent_data_ptr_->sufwoofer_enable_value);
          update_gpio();
          break;
        case Option::balance:
          enabled_balance_scrolling_ = !enabled_balance_scrolling_;
          break;
        case Option::more_options:
          selected_screen_ = OptionMenuScreen::advance;
          break;
        case Option::back:
          state_machine_ptr_->change_state(State::main_menu);
          break;
        case Option::enum_length:
          break;
        default:
          break;
      }
      break;
    case OptionMenuScreen::advance:
      switch (selected_advance_menu_option_)
      {
        case AdvanceMenuOption::bias:
          enabled_bias_scrolling_ = !enabled_bias_scrolling_;
          break;
        case AdvanceMenuOption::rename_bal:
        case AdvanceMenuOption::rename_rca1:
        case AdvanceMenuOption::rename_rca2:
        case AdvanceMenuOption::rename_rca3:
        {
          const auto audio_input = get_audio_input_from_advance_option(selected_advance_menu_option_);
          // Get the name alias
          auto& name_alias = persistent_data_ptr_->get_per_audio_input_data_mutable(audio_input).name_alias;
          increment_enum(InputNameAliasOption::enum_length, name_alias);
          break;
        }
        case AdvanceMenuOption::back:
          selected_screen_ = OptionMenuScreen::main;
          break;
        case AdvanceMenuOption::enum_length:
          break;
        default:
          break;
      }
      break;
    case OptionMenuScreen::enum_length:
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
  const bool is_main_menu = selected_screen_ == OptionMenuScreen::main;
  const bool is_scroll_for_bias = selected_advance_menu_option_ == AdvanceMenuOption::bias && enabled_bias_scrolling_;
  const bool is_scroll_for_balance = selected_option_ == Option::balance && enabled_balance_scrolling_;
  if (!is_main_menu && is_scroll_for_bias)
  {
    if (persistent_data_ptr_->bias < bias_increment)
    {
      persistent_data_ptr_->bias = 0;
    }
    else
    {
      persistent_data_ptr_->bias -= bias_increment;
    }
    update_gpio();
  }
  else if (is_main_menu && is_scroll_for_balance)
  {
    --persistent_data_ptr_->left_right_balance_db;
    persistent_data_ptr_->left_right_balance_db =
      constrain(persistent_data_ptr_->left_right_balance_db, -left_right_balance_range, left_right_balance_range);
    update_gpio();
  }
  else
  {
    if (is_main_menu)
    {
      decrement_enum(Option::enum_length, selected_option_);
    }
    else
    {
      decrement_enum(AdvanceMenuOption::enum_length, selected_advance_menu_option_);
    }
  }
}

void OptionController::menu_down()
{
  const bool is_main_menu = selected_screen_ == OptionMenuScreen::main;
  const bool is_scroll_for_bias = selected_advance_menu_option_ == AdvanceMenuOption::bias && enabled_bias_scrolling_;
  const bool is_scroll_for_balance = selected_option_ == Option::balance && enabled_balance_scrolling_;
  if (!is_main_menu && is_scroll_for_bias)
  {
    persistent_data_ptr_->bias += bias_increment;
    if (persistent_data_ptr_->bias > 100)
    {
      persistent_data_ptr_->bias = 100;
    }
    update_gpio();
  }
  else if (is_main_menu && is_scroll_for_balance)
  {
    ++persistent_data_ptr_->left_right_balance_db;
    persistent_data_ptr_->left_right_balance_db =
      constrain(persistent_data_ptr_->left_right_balance_db, -left_right_balance_range, left_right_balance_range);
    update_gpio();
  }
  else
  {
    if (is_main_menu)
    {
      increment_enum(Option::enum_length, selected_option_);
    }
    else
    {
      increment_enum(AdvanceMenuOption::enum_length, selected_advance_menu_option_);
    }
  }
}

bool OptionController::update_encoder()
{
  const int32_t current_count = option_encoder_ptr_->getCount();

  if (state_machine_ptr_->get_state() == State::main_menu)
  {
    prev_encoder_count_ = current_count;
    return false;
  }

  if (current_count - prev_encoder_count_ >= tick_per_option_)
  {
    menu_down();
    prev_encoder_count_ = current_count;
    return true;
  }
  if (current_count - prev_encoder_count_ <= -tick_per_option_)
  {
    menu_up();
    prev_encoder_count_ = current_count;
    return true;
  }
  return false;
}