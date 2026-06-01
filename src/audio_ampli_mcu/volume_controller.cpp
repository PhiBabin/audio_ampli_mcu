#include "volume_controller.h"
#include "pinout_config.h"

#include <cstdlib>

VolumeController::VolumeController(
  StateMachine& state_machine,
  PersistentData& persistent_data,
  PioEncoder& vol_encoder,
  GpioHandler& gpio_handler)
  : state_machine_(state_machine)
  , persistent_data_(persistent_data)
  , gpio_handler_(gpio_handler)
  , prev_encoder_count_(0)
  , tick_per_db_(TICK_PER_VOLUME_INCREMENT)
  , vol_encoder_(vol_encoder)
{
}

void VolumeController::init()
{
  gpio_handler_.cache_init_output(pin_out::set_low_gain, LOW);
#if defined(USE_V2_PCB)
  gpio_handler_.cache_init_output(pin_out::set_high_gain, LOW);
#endif

#if defined(USE_V1_PCB)
  gpio_handler_.cache_init_output(pin_out::latch_left_vol, LOW);
  gpio_handler_.cache_init_output(pin_out::latch_right_vol, LOW);
  gpio_handler_.apply();
#endif

  // init the pins
#if defined(USE_V2_PCB)
  for (const auto& pin : pin_out::left_volume_bits)
  {
    gpio_handler_.cache_init_output(pin, LOW);
  }
  for (const auto& pin : pin_out::right_volume_bits)
  {
    gpio_handler_.cache_init_output(pin, LOW);
  }

#else
  for (const auto& pin : pin_out::volume_bits)
  {
    gpio_handler_.cache_init_output(pin, LOW);
  }
#endif

  // Restore the volume db from the flash
  reset_volume_tick_count_based_volume_db();
  gpio_handler_.apply();
}

void VolumeController::on_option_change()
{
  reset_volume_tick_count_based_volume_db();
  set_gpio_based_on_volume();
}

void VolumeController::set_mute(const bool is_mute)
{
  is_muted_ = is_mute;
}

void VolumeController::reset_volume_tick_count_based_volume_db()
{
  set_volume_db(persistent_data_.get_volume_db());
}

void VolumeController::set_gpio_volume(
  const std::array<GpioPin, 7U>& volume_pins, const uint8_t vol_7bit, const uint8_t mask)
{

  for (size_t i = 0; i < volume_pins.size(); ++i)
  {
    const auto should_set_it = ((mask >> i) & 1) == 1;
    if (should_set_it)
    {
      const auto& pin = volume_pins[i];
      const auto is_set = ((vol_7bit >> i) & 1) == 1;
      gpio_handler_.cache_write_pin(pin, is_set ? HIGH : LOW);
    }
  }
  gpio_handler_.apply();
}

void VolumeController::set_gpio_volume(
  const std::array<GpioPin, 6U>& volume_pins, const uint8_t vol_6bit, const uint8_t mask)
{

  for (size_t i = 0; i < volume_pins.size(); ++i)
  {
    const auto should_set_it = ((mask >> i) & 1) == 1;
    if (should_set_it)
    {
      const auto& pin = volume_pins[i];
      const auto is_set = ((vol_6bit >> i) & 1) == 1;
      gpio_handler_.cache_write_pin(pin, is_set ? HIGH : LOW);
    }
  }
  gpio_handler_.apply();
}

void VolumeController::latch_volume_gpio_one_side_v2(
  const uint8_t prev_vol_7bit, const uint8_t vol_7bit, const std::array<GpioPin, 7U>& volume_pins)
{
  constexpr uint32_t relay_0_to_1_transition_time_us = 1500;
  constexpr uint32_t relay_1_to_0_transition_time_us = 680;

  // If volume didn't change -> noop
  if (prev_vol_7bit == vol_7bit)
  {
    return;
  }

  // Changing a relay from 1 -> 0 (~0.68ms) is must faster than 0 -> 1 (~1.5ms), so to make it look like all the relay
  // are changing at the same time we need to set the GPIO in two stages.

  // 1) Apply all 0 -> 1 changes (also apply no-op 1 -> 1)
  set_gpio_volume(volume_pins, vol_7bit, /*mask = */ vol_7bit);

  // 2) Wait ~0.82ms (1.5ms - 0.68ms)
  constexpr auto wait_time_us = relay_0_to_1_transition_time_us - relay_1_to_0_transition_time_us;
  delayMicroseconds(wait_time_us);

  // 3) Apply all 1 -> 0 changes (also apply no-op 0 -> 0)
  set_gpio_volume(volume_pins, vol_7bit, /*mask = */ ~vol_7bit);
}

void VolumeController::latch_volume_gpio_one_side_v1(
  const uint8_t prev_vol_6bit,
  const uint8_t vol_6bit,
  const GpioPin& latch_pin,
  const std::array<GpioPin, 6U>& volume_pins)
{
  constexpr uint32_t relay_0_to_1_transition_time_us = 800;
  constexpr uint32_t relay_1_to_0_transition_time_us = 1500;

  // Set GPIO to there previous value, if we don't we'll latch with the other's side volume
  set_gpio_volume(volume_pins, prev_vol_6bit);

  // Start latching
  gpio_handler_.write_pin(latch_pin, HIGH);

  // Changing a relay from 0 -> 1 (~0.8ms) is must faster than 1 -> 0 (~1.5ms), so to make it look like all the relay
  // are changing at the same time we need to set the GPIO in two stages.

  // 1) Apply all 1 -> 0 changes (also apply no-op 0 -> 0)
  set_gpio_volume(volume_pins, vol_6bit, /*mask = */ ~vol_6bit);

  // 2) Wait ~0.7ms (1.5ms - 0.8ms)
  constexpr auto wait_time_us = relay_1_to_0_transition_time_us - relay_0_to_1_transition_time_us;
  delayMicroseconds(wait_time_us);

  // 3) Apply all 0 -> 1 changes (also apply no-op 1 -> 1)
  set_gpio_volume(volume_pins, vol_6bit, /*mask = */ vol_6bit);

  // Wait for the latching to occur
  delayMicroseconds(1);

  // Unlatch
  gpio_handler_.write_pin(latch_pin, LOW);
}

std::tuple<int16_t, int16_t> VolumeController::get_left_right_bias_compensation()
{
  const int16_t bal_half = persistent_data_.left_right_balance_db / 5;
  const int16_t sign = bal_half < 0 ? -1 : 1;
  const int16_t abs_bal = bal_half * sign;
  const int16_t change = sign * (abs_bal / 2);

  int16_t left_half{0};
  int16_t right_half{0};

  // If there is an odd offset (e.g. +3dB), the distribution is not symmetric, so we increase the right side if the bias
  // is positive and the left side if negative.
  if (abs_bal % 2 == 0)
  {
    left_half = -change;
    right_half = change;
  }
  else if (bal_half > 0)
  {
    left_half = -change;
    right_half = change + sign;
  }
  else
  {
    left_half = -(change + sign);
    right_half = change;
  }
  return std::make_tuple(left_half * 5, right_half * 5);
}

void VolumeController::set_gain_based_on_volume(const int32_t left_vol_tenth, const int32_t right_vol_tenth)
{
  const int32_t max_vol_tenth = left_vol_tenth > right_vol_tenth ? left_vol_tenth : right_vol_tenth;

#if defined(USE_V2_PCB)
  if (max_vol_tenth < -120)
  {
    gpio_handler_.cache_write_pin(pin_out::set_low_gain, HIGH);
    gpio_handler_.cache_write_pin(pin_out::set_high_gain, LOW);
  }
  else if (max_vol_tenth <= 0)
  {
    gpio_handler_.cache_write_pin(pin_out::set_low_gain, LOW);
    gpio_handler_.cache_write_pin(pin_out::set_high_gain, LOW);
  }
  else
  {
    gpio_handler_.cache_write_pin(pin_out::set_low_gain, LOW);
    gpio_handler_.cache_write_pin(pin_out::set_high_gain, HIGH);
  }
#else
  if (max_vol_tenth < 0)
  {
    gpio_handler_.cache_write_pin(pin_out::set_low_gain, HIGH);
  }
  else
  {
    gpio_handler_.cache_write_pin(pin_out::set_low_gain, LOW);
  }
#endif
}

void VolumeController::set_gpio_based_on_volume()
{
  uint8_t left_vol_bits = 0;
  uint8_t right_vol_bits = 0;
  if (!is_muted())
  {
    const int32_t volume_tenth_db = get_volume_db();
    const auto [left_bias, right_bias] = get_left_right_bias_compensation();

#if defined(USE_V2_PCB)
    constexpr int32_t min_vol_tenth = -755; // -75.5dB
    constexpr int32_t max_vol_tenth = 120; // 12dB
    constexpr int32_t low_gain_threshold = -12;
    constexpr int32_t low_gain_boost = -12;
    constexpr int32_t medium_gain_boost = 0;
    constexpr int32_t high_gain_boost = 12;
#else
    constexpr int32_t min_vol_tenth = -635; // -63.5dB
    constexpr int32_t max_vol_tenth = 140; // 14dB
    constexpr int32_t low_gain_boost = 0;
    constexpr int32_t high_gain_boost = 14;
#endif

    const int32_t left_eff_tenth = constrain(volume_tenth_db + left_bias, min_vol_tenth, max_vol_tenth);
    const int32_t right_eff_tenth = constrain(volume_tenth_db + right_bias, min_vol_tenth, max_vol_tenth);

    #if !defined(USE_V2_PCB)
#if !defined(USE_V2_PCB)
    const int32_t left_int_db = left_eff_tenth >= 0 ? left_eff_tenth / 10 : (left_eff_tenth - 9) / 10;
    const int32_t right_int_db = right_eff_tenth >= 0 ? right_eff_tenth / 10 : (right_eff_tenth - 9) / 10;
#endif
#endif

    set_gain_based_on_volume(left_eff_tenth, right_eff_tenth);

    const int32_t max_vol_eff_tenth = left_eff_tenth > right_eff_tenth ? left_eff_tenth : right_eff_tenth;

#if defined(USE_V2_PCB)
    int32_t gain_boost;
    if (max_vol_eff_tenth < low_gain_threshold * 10)
    {
      gain_boost = low_gain_boost;
    }
    else if (max_vol_eff_tenth <= 0)
    {
      gain_boost = medium_gain_boost;
    }
    else
    {
      gain_boost = high_gain_boost;
    }
#else
    const int32_t gain_boost = (max_vol_eff_tenth < 0) ? low_gain_boost : high_gain_boost;
#endif

#if !defined(USE_V2_PCB)
    const int32_t left_relay_int = constrain(left_int_db - gain_boost, -63, 0);
    const int32_t right_relay_int = constrain(right_int_db - gain_boost, -63, 0);
#endif

#if defined(USE_V2_PCB)
    left_vol_bits = static_cast<uint8_t>(constrain(
      left_eff_tenth / 5 - 2 * gain_boost + 127, 0, 127));
    right_vol_bits = static_cast<uint8_t>(constrain(
      right_eff_tenth / 5 - 2 * gain_boost + 127, 0, 127));
#else
    left_vol_bits = static_cast<uint8_t>(63 + left_relay_int);
    right_vol_bits = static_cast<uint8_t>(63 + right_relay_int);
#endif

    // char vol_buf[8];
    // Serial.print(" Vol: L=");
    // for (int8_t b = 6; b >= 0; --b) { vol_buf[6 - b] = '0' + ((left_vol_bits >> b) & 1); }
    // vol_buf[7] = '\0';
    // Serial.print(vol_buf);
    // Serial.print(" R=");
    // for (int8_t b = 6; b >= 0; --b) { vol_buf[6 - b] = '0' + ((right_vol_bits >> b) & 1); }
    // Serial.print(vol_buf);
    // Serial.println("");

    switch (persistent_data_.mute_channel)
    {
      case MuteChannel::mute_left:
        left_vol_bits = 0;
        break;
      case MuteChannel::mute_right:
        right_vol_bits = 0;
        break;
      case MuteChannel::both_channel_enabled:
      default:
        break;
    }
  }

#if defined(USE_V2_PCB)
  latch_volume_gpio_one_side_v2(prev_vol_set_on_left_, left_vol_bits, pin_out::left_volume_bits);
  latch_volume_gpio_one_side_v2(prev_vol_set_on_right_, right_vol_bits, pin_out::right_volume_bits);
  prev_vol_set_on_left_ = left_vol_bits;
  prev_vol_set_on_right_ = right_vol_bits;

#elif defined(USE_V1_PCB)
  latch_volume_gpio_one_side_v1(
    prev_vol_set_on_left_, left_vol_bits, pin_out::latch_left_vol, pin_out::volume_bits);
  latch_volume_gpio_one_side_v1(
    prev_vol_set_on_right_, right_vol_bits, pin_out::latch_right_vol, pin_out::volume_bits);
  prev_vol_set_on_left_ = left_vol_bits;
  prev_vol_set_on_right_ = right_vol_bits;
#else
  latch_volume_gpio_one_side(prev_vol_set_on_left_, left_vol_bits, pin_out::latch_left_vol);
  prev_vol_set_on_left_ = left_vol_bits;
#endif
}

bool VolumeController::is_muted() const
{
  return is_muted_;
}

int32_t VolumeController::get_volume_db() const
{
  return persistent_data_.get_volume_db();
}

int32_t VolumeController::get_volume_db_int() const
{
  const int32_t h = persistent_data_.get_volume_db();
  return h / 10;
}

uint8_t VolumeController::get_volume_tenth_db_rem() const
{
  const int32_t h = persistent_data_.get_volume_db();
  return static_cast<uint8_t>(abs(h) % 10);
}

void VolumeController::increase_volume_db(const int32_t delta_volume_tenth_db)
{
  set_volume_db(get_volume_db() + delta_volume_tenth_db);
}

void VolumeController::set_volume_db(const int32_t new_volume_tenth_db)
{
#if defined(USE_V2_PCB)
  const auto constraint_volume_db = constrain(new_volume_tenth_db, -755, 120); // -75.5dB and +12.0dB
#else
  const auto constraint_volume_db = constrain(new_volume_tenth_db, -635, 140); // -63.5dB and +14.0dB
#endif

  // Update volume DB in the persistent data
  persistent_data_.get_volume_db_mutable() = constraint_volume_db;
  set_gpio_based_on_volume();
  latched_volume_updated_ = true;
}

bool VolumeController::update_volume()
{
  const int32_t current_count = vol_encoder_.getCount();
  if (current_count == prev_encoder_count_)
  {
    return false;
  }
  // If muted don't update the volume
  if (is_muted())
  {
    prev_encoder_count_ = current_count;
    return false;
  }

  // e.g. 4 - 8 => -4
  const auto delta_tick = current_count - prev_encoder_count_;

  if (delta_tick >= tick_per_db_)
  {
    increase_volume_db((delta_tick / tick_per_db_) * VOLUME_STEP_TENTH_DB);
    const auto remainder = delta_tick % tick_per_db_;
    prev_encoder_count_ = prev_encoder_count_ + delta_tick - remainder;
    return true;
  }
  // e.g. -4 <= 3
  if (delta_tick <= -tick_per_db_)
  {
    // e.g -4 / 3 => increase_volume_db(-1)
    increase_volume_db((delta_tick / tick_per_db_) * VOLUME_STEP_TENTH_DB);
    // e.g. (-(-4)) % 3 => 1
    const auto remainder = (-delta_tick) % tick_per_db_;
    /// 8 - 4 + 1 => 5
    /// so if count stay the same there will be current_count == 4 and prev_encoder_count == 5 so a delta of -1
    prev_encoder_count_ = prev_encoder_count_ + delta_tick + remainder;
    return true;
  }
  return false;
}

void VolumeController::toggle_mute()
{
  is_muted_ = !is_muted_;
  latched_volume_updated_ = true;
}

bool VolumeController::update()
{
  bool change = false;
  change |= latched_volume_updated_;
  change |= update_volume();

  if (latched_volume_updated_)
  {
    latched_volume_updated_ = false;
  }
  return change;
}
