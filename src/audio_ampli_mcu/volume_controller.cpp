#include "volume_controller.h"

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

VolumeController::VolumeController(
  StateMachine* state_machine_ptr,
  PersistentData* persistent_data_ptr,
  const std::array<pin_size_t, 6> gpio_pin_vol_select,
  PioEncoder* vol_encoder_ptr,
  const int mute_button_pin,
  const int set_mute_pin,
  const int power_enable_pin,
  const int latch_left_vol,
  const int latch_right_vol,
  const int32_t total_tick_for_63db)
  : state_machine_ptr_(state_machine_ptr)
  , persistent_data_ptr_(persistent_data_ptr)
  , gpio_pin_vol_select_(gpio_pin_vol_select)
  , mute_button_pin_(mute_button_pin)
  , set_mute_pin_(set_mute_pin)
  , power_enable_pin_(power_enable_pin)
  , latch_left_vol_(latch_left_vol)
  , latch_right_vol_(latch_right_vol)
  , volume_(0)
  , prev_encoder_count_(0)
  , total_tick_for_63db_(total_tick_for_63db)
  , vol_encoder_ptr_(vol_encoder_ptr)
{
}

void VolumeController::init()
{
  mute_button_.setup(mute_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);

  pinMode(latch_left_vol_, OUTPUT);
  pinMode(latch_right_vol_, OUTPUT);
  digitalWrite(latch_left_vol_, LOW);
  digitalWrite(latch_right_vol_, LOW);

  // pinMode(set_mute_pin_, OUTPUT);
  // digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low

  // Restore the volume db from the flash
  reset_volume_tick_count_based_volume_db();

  for (const auto& pin : gpio_pin_vol_select_)
  {
    pinMode(pin, OUTPUT);
  }
  set_gpio_based_on_volume();

  pinMode(power_enable_pin_, OUTPUT);
  digitalWrite(power_enable_pin_, HIGH);
}

void VolumeController::on_audio_input_change()
{
  reset_volume_tick_count_based_volume_db();
  set_gpio_based_on_volume();
}

void VolumeController::on_option_change()
{
  reset_volume_tick_count_based_volume_db();
  set_gpio_based_on_volume();
}

void VolumeController::reset_volume_tick_count_based_volume_db()
{
  set_volume_db(persistent_data_ptr_->get_volume_db());
}

void VolumeController::set_gpio_volume(const uint8_t vol_6bit, const uint8_t mask)
{

  for (size_t i = 0; i < gpio_pin_vol_select_.size(); ++i)
  {
    const auto should_set_it = ((mask >> i) & 1) == 1;
    if (should_set_it)
    {
      const auto& pin = gpio_pin_vol_select_[i];
      const auto is_set = ((vol_6bit >> i) & 1) == 1;
      digitalWrite(pin, is_set ? HIGH : LOW);
    }
  }
}

void VolumeController::latch_volume_gpio_one_side(
  const uint8_t prev_vol_6bit, const uint8_t vol_6bit, const pin_size_t latch_pin)
{
  constexpr uint32_t relay_0_to_1_transition_time_us = 800;
  constexpr uint32_t relay_1_to_0_transition_time_us = 1500;

  // Set GPIO to there previous value, if we don't we'll latch with the other's side volume
  set_gpio_volume(prev_vol_6bit);

  // Start latching
  digitalWrite(latch_pin, HIGH);

  // Changing a relay from 0 -> 1 (~0.8ms) is must faster than 1 -> 0 (~1.5ms), so to make it look like all the relay
  // are changing at the same time we need to set the GPIO in two stages.

  // 1) Apply all 1 -> 0 changes (also apply no-op 0 -> 0)
  set_gpio_volume(vol_6bit, /*mask = */ ~vol_6bit);

  // 2) Wait ~0.7ms (1.5ms - 0.8ms)
  constexpr auto wait_time_us = relay_1_to_0_transition_time_us - relay_0_to_1_transition_time_us;
  delayMicroseconds(wait_time_us);

  // 3) Apply all 0 -> 1 changes (also apply no-op 1 -> 1)
  set_gpio_volume(vol_6bit, /*mask = */ vol_6bit);

  // Wait for the latching to occur
  delayMicroseconds(1);

  // Unlatch
  digitalWrite(latch_pin, LOW);
}

void VolumeController::set_gpio_based_on_volume()
{
  // constexpr uint32_t relay_0_to_1_transition_time_us = 800;
  // constexpr uint32_t relay_1_to_0_transition_time_us = 1500;
  uint8_t vol_6bit = 0;
  if (!is_muted())
  {
    // Map volume to 6 bit (64 state)
    vol_6bit = static_cast<uint8_t>(map(volume_, 0, total_tick_for_63db_, 0, 64));
  }

#ifdef USE_V2_PCB
  latch_volume_gpio_one_side(prev_vol_6bit_set_on_left_, vol_6bit, latch_left_vol_);
  latch_volume_gpio_one_side(prev_vol_6bit_set_on_right_, vol_6bit, latch_right_vol_);
  prev_vol_6bit_set_on_left_ = vol_6bit;
  prev_vol_6bit_set_on_right_ = vol_6bit;
#else
  latch_volume_gpio_one_side(prev_vol_6bit_set_on_left_, vol_6bit, latch_left_vol_);
  prev_vol_6bit_set_on_left_ = vol_6bit;
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

void VolumeController::toggle_mute()
{
  is_muted_ = !is_muted_;
  latched_volume_updated_ = true;
}

void VolumeController::power_off()
{
  digitalWrite(power_enable_pin_, HIGH);
  state_machine_ptr_->change_state(State::standby);
}
void VolumeController::power_on()
{
  digitalWrite(power_enable_pin_, LOW);
  state_machine_ptr_->change_state(State::main_menu);
}

bool VolumeController::update_mute()
{
  unsigned long now = millis();
  mute_button_.process(now);
  if (mute_button_.is_short_press())
  {
    Serial.println("Short press");
    is_muted_ = !is_muted_;
    // digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low
    set_gpio_based_on_volume();
  }
  else if (mute_button_.is_long_press())
  {
    Serial.println("Long press");
    if (state_machine_ptr_->get_state() != State::standby)
    {
      power_off();
    }
    else
    {
      power_on();
    }
  }
  return mute_button_.is_short_press() || mute_button_.is_long_press();
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