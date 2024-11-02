#include "volume_controller.h"

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

VolumeController::VolumeController(
  StateMachine* state_machine_ptr,
  PersistentData* persistent_data_ptr,
  const std::array<pin_size_t, 6> gpio_pin_vol_select,
  PioEncoder* vol_encoder_ptr,
  const int mute_button_pin,
  const int set_mute_pin,
  const int32_t total_tick_for_63db)
  : state_machine_ptr_(state_machine_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , gpio_pin_vol_select_(gpio_pin_vol_select)
  , mute_button_pin_(mute_button_pin)
  , set_mute_pin_(set_mute_pin)
  , volume_(0)
  , prev_encoder_count_(0)
  , total_tick_for_63db_(total_tick_for_63db)
  , vol_encoder_ptr_(vol_encoder_ptr)
{
}

void VolumeController::init()
{
  mute_button_.setup(mute_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);

  // pinMode(set_mute_pin_, OUTPUT);
  // digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low

  // Restore the volume db from the flash
  reset_volume_tick_count_based_volume_db();

  for (const auto& pin : gpio_pin_vol_select_)
  {
    pinMode(pin, OUTPUT);
  }
  set_gpio_based_on_volume();
}

void VolumeController::on_audio_input_change()
{
  reset_volume_tick_count_based_volume_db();
  set_gpio_based_on_volume();
}

void VolumeController::reset_volume_tick_count_based_volume_db()
{
  set_volume_db(persistent_data_ptr_->get_volume_db());
}

void VolumeController::set_gpio_based_on_volume()
{
  constexpr uint32_t relay_0_to_1_transition_time_us = 800;
  constexpr uint32_t relay_1_to_0_transition_time_us = 1500;
  uint8_t vol_6bit = 0;
  if (!is_muted())
  {
    // Map volume to 6 bit (64 state)
    vol_6bit = static_cast<uint8_t>(map(volume_, 0, total_tick_for_63db_, 0, 64));
  }

  // Changing a relay from 0 -> 1 (~0.8ms) is must faster than 1 -> 0 (~1.5ms), so to make it look like all the relay
  // are changing at the same time we need to set the GPIO in two stages.

  // 1) Apply all 1 -> 0 changes

  for (size_t i = 0; i < gpio_pin_vol_select_.size(); ++i)
  {
    const auto was_set = ((prev_vol_6bit_set_on_gpio_ >> i) & 1) == 1;
    const auto is_set = ((vol_6bit >> i) & 1) == 1;
    if (was_set && !is_set)
    {
      const auto& pin = gpio_pin_vol_select_[i];
      digitalWrite(pin, is_set ? HIGH : LOW);
    }
  }

  // 2) Wait ~0.7ms (1.5ms - 0.8ms)

  constexpr auto wait_time_us = relay_1_to_0_transition_time_us - relay_0_to_1_transition_time_us;
  delayMicroseconds(wait_time_us);

  // 3) Apply all 0 -> 1 changes (also apply no-op  0 -> 0 and 1 -> 1)

  for (size_t i = 0; i < gpio_pin_vol_select_.size(); ++i)
  {
    const auto was_set = ((prev_vol_6bit_set_on_gpio_ >> i) & 1) == 1;
    const auto is_set = ((vol_6bit >> i) & 1) == 1;
    if (!(was_set && !is_set))
    {
      const auto& pin = gpio_pin_vol_select_[i];
      digitalWrite(pin, is_set ? HIGH : LOW);
    }
  }

  prev_vol_6bit_set_on_gpio_ = vol_6bit;
}

bool VolumeController::is_muted() const
{
  return mute_button_.get_state();
}

int32_t VolumeController::get_volume_db() const
{
  return persistent_data_ptr_->get_volume_db();
}

void VolumeController::set_volume_db(const int32_t new_volume_db)
{
  const auto constraint_volume_db = constrain(new_volume_db, -63, 1);
  volume_ = map(constraint_volume_db, -63, 1, 0, total_tick_for_63db_);
  set_gpio_based_on_volume();

  // Update volume DB in the persistent data
  persistent_data_ptr_->get_volume_db_mutable() = constraint_volume_db;
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
  // Apply volume change
  volume_ += current_count - prev_encoder_count_;
  // Count cannot increase beyond the -63 to 0 range
  volume_ = constrain(volume_, 0, total_tick_for_63db_ - 1);
  prev_encoder_count_ = current_count;

  // Apply volume to GPIO and persistent data
  const auto new_volume_db = map(volume_, 0, total_tick_for_63db_, -63, 1);
  persistent_data_ptr_->get_volume_db_mutable() = new_volume_db;
  set_gpio_based_on_volume();
  return true;
}

bool VolumeController::update_mute()
{
  const bool prev_mute_state = mute_button_.get_state();
  unsigned long now = millis();
  mute_button_.process(now);
  const bool has_changed = mute_button_.get_state() != prev_mute_state;
  if (has_changed)
  {
    // digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low
    set_gpio_based_on_volume();
  }
  return has_changed;
}

bool VolumeController::update()
{
  bool change = false;
  change |= latched_volume_updated_;
  change |= update_volume();
  change |= update_mute();

  if (latched_volume_updated_)
  {
    latched_volume_updated_ = false;
  }
  return change;
}