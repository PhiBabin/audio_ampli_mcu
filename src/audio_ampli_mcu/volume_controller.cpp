#include "volume_controller.h"

#include <iostream>

VolumeController::VolumeController(
  StateMachine* state_machine_ptr,
  PersistentData* persistent_data_ptr,
  PioEncoder* vol_encoder_ptr,
  GpioHandler* gpio_handler_ptr,
  const int32_t total_tick_for_63db)
  : state_machine_ptr_(state_machine_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , gpio_handler_ptr_(gpio_handler_ptr)
  , prev_encoder_count_(0)
  , total_tick_for_63db_(total_tick_for_63db)
  , tick_per_db_(total_tick_for_63db_ / 64)
  , vol_encoder_ptr_(vol_encoder_ptr)
{
}

void VolumeController::init()
{
#if defined(USE_V1_PCB)
  gpio_handler_ptr_->cache_init_output(pin_out::latch_left_vol, LOW);
  gpio_handler_ptr_->cache_init_output(pin_out::latch_right_vol, LOW);
  gpio_handler_ptr_->apply();
#endif

  // pinMode(set_mute_pin_, OUTPUT);
  // digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low

  // Restore the volume db from the flash
  reset_volume_tick_count_based_volume_db();

#if defined(USE_V2_PCB)
  for (const auto& pin : pin_out::left_volume_bits)
  {
    gpio_handler_ptr_->cache_init_output(pin, LOW);
  }
  for (const auto& pin : pin_out::right_volume_bits)
  {
    gpio_handler_ptr_->cache_init_output(pin, LOW);
  }

#else
  for (const auto& pin : pin_out::volume_bits)
  {
    gpio_handler_ptr_->cache_init_output(pin, LOW);
  }
#endif
  gpio_handler_ptr_->apply();
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
  set_volume_db(persistent_data_ptr_->get_volume_db());
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
      gpio_handler_ptr_->cache_write_pin(pin, is_set ? HIGH : LOW);
    }
  }
  gpio_handler_ptr_->apply();
}

void VolumeController::latch_volume_gpio_one_side_v2(
  const uint8_t prev_vol_6bit, const uint8_t vol_6bit, const std::array<GpioPin, 6U>& volume_pins)
{
  constexpr uint32_t relay_0_to_1_transition_time_us = 1500;
  constexpr uint32_t relay_1_to_0_transition_time_us = 680;

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
  gpio_handler_ptr_->write_pin(latch_pin, HIGH);

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
  gpio_handler_ptr_->write_pin(latch_pin, LOW);
}

std::tuple<int16_t, int16_t> VolumeController::get_left_right_bias_compensation()
{
  const int16_t sign = persistent_data_ptr_->left_right_balance_db < 0 ? -1 : 1;
  const int16_t change = sign * abs(persistent_data_ptr_->left_right_balance_db) / 2;

  // If there is an odd offset (e.g. +3dB), the distribution is not symmetric, so we increase the right side if the bias
  // is positive and the left side if negative.
  if (abs(persistent_data_ptr_->left_right_balance_db) % 2 == 1)
  {
    if (persistent_data_ptr_->left_right_balance_db > 0)
    {
      return std::make_tuple(-change, change + sign);
    }
    return std::make_tuple(-change - sign, change);
  }
  return std::make_tuple(-change, change);
}

void VolumeController::set_gpio_based_on_volume()
{
  uint8_t left_vol_6bit = 0;
  uint8_t right_vol_6bit = 0;
  if (!is_muted())
  {
    // Map volume to 6 bit, -63 -> 0,  0 -> 63
    const uint8_t positive_volume = 63 + get_volume_db();

    const auto [left_bias, right_bias] = get_left_right_bias_compensation();
    left_vol_6bit = constrain(positive_volume + left_bias, 0, 63);
    right_vol_6bit = constrain(positive_volume + right_bias, 0, 63);
    switch (persistent_data_ptr_->mute_channel)
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
  return persistent_data_ptr_->get_volume_db();
}

void VolumeController::increase_volume_db(const int32_t delta_volume_db)
{
  set_volume_db(get_volume_db() + delta_volume_db);
}

void VolumeController::set_volume_db(const int32_t new_volume_db)
{
  const auto constraint_volume_db = constrain(new_volume_db, -63, 0);

  // Update volume DB in the persistent data
  persistent_data_ptr_->get_volume_db_mutable() = constraint_volume_db;
  set_gpio_based_on_volume();
  latched_volume_updated_ = true;
}

bool VolumeController::update_volume()
{
  const int32_t current_count = vol_encoder_ptr_->getCount();
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

  const auto delta_tick = current_count - prev_encoder_count_;
  if (delta_tick >= tick_per_db_)
  {
    increase_volume_db(delta_tick / tick_per_db_);
    const auto remainder = delta_tick % tick_per_db_;
    prev_encoder_count_ = prev_encoder_count_ + delta_tick - remainder;
    return true;
  }
  if (delta_tick <= -tick_per_db_)
  {
    increase_volume_db(delta_tick / tick_per_db_);
    const auto remainder = (-delta_tick) % tick_per_db_;
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