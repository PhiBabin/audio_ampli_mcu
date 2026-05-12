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

  // Restore the volume db from the flash
  reset_volume_tick_count_based_volume_db();

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
  gpio_handler_.apply();
  set_gpio_based_on_volume();
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
  const uint8_t prev_vol_6bit, const uint8_t vol_6bit, const std::array<GpioPin, 6U>& volume_pins)
{
  constexpr uint32_t relay_0_to_1_transition_time_us = 1500;
  constexpr uint32_t relay_1_to_0_transition_time_us = 680;

  // If volume didn't change -> noop
  if (prev_vol_6bit == vol_6bit)
  {
    return;
  }

  // Changing a relay from 1 -> 0 (~0.68ms) is must faster than 0 -> 1 (~1.5ms), so to make it look like all the relay
  // are changing at the same time we need to set the GPIO in two stages.

  // 1) Apply all 0 -> 1 changes (also apply no-op 1 -> 1)
  set_gpio_volume(volume_pins, vol_6bit, /*mask = */ vol_6bit);

  // 2) Wait ~0.82ms (1.5ms - 0.68ms)
  constexpr auto wait_time_us = relay_0_to_1_transition_time_us - relay_1_to_0_transition_time_us;
  delayMicroseconds(wait_time_us);

  // 3) Apply all 1 -> 0 changes (also apply no-op 0 -> 0)
  set_gpio_volume(volume_pins, vol_6bit, /*mask = */ ~vol_6bit);
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
  const int16_t sign = persistent_data_.left_right_balance_db < 0 ? -1 : 1;
  const int16_t change = sign * abs(persistent_data_.left_right_balance_db) / 2;

  // If there is an odd offset (e.g. +3dB), the distribution is not symmetric, so we increase the right side if the bias
  // is positive and the left side if negative.
  if (abs(persistent_data_.left_right_balance_db) % 2 == 1)
  {
    if (persistent_data_.left_right_balance_db > 0)
    {
      return std::make_tuple(-change, change + sign);
    }
    return std::make_tuple(-change - sign, change);
  }
  return std::make_tuple(-change, change);
}

void VolumeController::set_gain_based_on_volume(const int32_t left_volume_db, const int32_t right_volume_db)
{
  const int32_t max_vol = left_volume_db > right_volume_db ? left_volume_db : right_volume_db;

#if defined(USE_V2_PCB)
  if (max_vol < -12)
  {
    gpio_handler_.cache_write_pin(pin_out::set_low_gain, HIGH);
    gpio_handler_.cache_write_pin(pin_out::set_high_gain, LOW);
  }
  else if (max_vol < 0)
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
  if (max_vol < 0)
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
  uint8_t left_vol_6bit = 0;
  uint8_t right_vol_6bit = 0;
  if (!is_muted())
  {
    const int32_t volume_db = get_volume_db();
    const auto [left_bias, right_bias] = get_left_right_bias_compensation();

#if defined(USE_V2_PCB)
    constexpr int32_t min_vol = -75;
    constexpr int32_t max_vol = 12;
    constexpr int32_t low_gain_threshold = -12;
    constexpr int32_t low_gain_boost = -12;
    constexpr int32_t medium_gain_boost = 0;
    constexpr int32_t high_gain_boost = 12;
#else
    constexpr int32_t min_vol = -63;
    constexpr int32_t max_vol = 14;
    constexpr int32_t low_gain_boost = 0;
    constexpr int32_t high_gain_boost = 14;
#endif

    const int32_t left_eff_vol = constrain(volume_db + left_bias, min_vol, max_vol);
    const int32_t right_eff_vol = constrain(volume_db + right_bias, min_vol, max_vol);

    set_gain_based_on_volume(left_eff_vol, right_eff_vol);

    const int32_t max_vol_eff = left_eff_vol > right_eff_vol ? left_eff_vol : right_eff_vol;

#if defined(USE_V2_PCB)
    int32_t gain_boost;
    if (max_vol_eff < low_gain_threshold)
    {
      gain_boost = low_gain_boost;
    }
    else if (max_vol_eff < 0)
    {
      gain_boost = medium_gain_boost;
    }
    else
    {
      gain_boost = high_gain_boost;
    }
#else
    const int32_t gain_boost = (max_vol_eff < 0) ? low_gain_boost : high_gain_boost;
#endif

    const int32_t left_relay_db = constrain(left_eff_vol - gain_boost, -63, 0);
    const int32_t right_relay_db = constrain(right_eff_vol - gain_boost, -63, 0);

    left_vol_6bit = 63 + left_relay_db;
    right_vol_6bit = 63 + right_relay_db;

    Serial.print("Vol: L=");
    Serial.print(left_vol_6bit);
    Serial.print(" R=");
    Serial.println(right_vol_6bit);

    switch (persistent_data_.mute_channel)
    {
      case MuteChannel::mute_left:
        left_vol_6bit = 0;
        break;
      case MuteChannel::mute_right:
        right_vol_6bit = 0;
        break;
      case MuteChannel::both_channel_enabled:
      default:
        break;
    }
  }

#if defined(USE_V2_PCB)
  latch_volume_gpio_one_side_v2(prev_vol_6bit_set_on_left_, left_vol_6bit, pin_out::left_volume_bits);
  latch_volume_gpio_one_side_v2(prev_vol_6bit_set_on_right_, right_vol_6bit, pin_out::right_volume_bits);
  prev_vol_6bit_set_on_left_ = left_vol_6bit;
  prev_vol_6bit_set_on_right_ = right_vol_6bit;

#elif defined(USE_V1_PCB)
  latch_volume_gpio_one_side_v1(
    prev_vol_6bit_set_on_left_, left_vol_6bit, pin_out::latch_left_vol, pin_out::volume_bits);
  latch_volume_gpio_one_side_v1(
    prev_vol_6bit_set_on_right_, right_vol_6bit, pin_out::latch_right_vol, pin_out::volume_bits);
  prev_vol_6bit_set_on_left_ = left_vol_6bit;
  prev_vol_6bit_set_on_right_ = right_vol_6bit;
#else
  latch_volume_gpio_one_side(prev_vol_6bit_set_on_left_, left_vol_6bit, pin_out::latch_left_vol);
  prev_vol_6bit_set_on_left_ = left_vol_6bit;
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

void VolumeController::increase_volume_db(const int32_t delta_volume_db)
{
  set_volume_db(get_volume_db() + delta_volume_db);
}

void VolumeController::set_volume_db(const int32_t new_volume_db)
{
#if defined(USE_V2_PCB)
  const auto constraint_volume_db = constrain(new_volume_db, -75, 12);
#else
  const auto constraint_volume_db = constrain(new_volume_db, -63, 14);
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
    increase_volume_db(delta_tick / tick_per_db_);
    const auto remainder = delta_tick % tick_per_db_;
    prev_encoder_count_ = prev_encoder_count_ + delta_tick - remainder;
    return true;
  }
  // e.g. -4 <= 3
  if (delta_tick <= -tick_per_db_)
  {
    // e.g -4 / 3 => increase_volume_db(-1)
    increase_volume_db(delta_tick / tick_per_db_);
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
