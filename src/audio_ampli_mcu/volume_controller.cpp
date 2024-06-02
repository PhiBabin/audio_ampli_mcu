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

  pinMode(set_mute_pin_, OUTPUT);
  digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low

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
  volume_ = map(persistent_data_ptr_->get_volume_db(), -63, 1, 0, total_tick_for_63db_);
}


void VolumeController::set_gpio_based_on_volume()
{
  // Map volume to 6 bit (64 state)
  const uint8_t vol_6bit = static_cast<uint8_t>(map(volume_, 0, total_tick_for_63db_, 0, 64));
  for (size_t i = 0; i < gpio_pin_vol_select_.size(); ++i)
  {
    const auto& pin = gpio_pin_vol_select_[i];
    digitalWrite(pin, ((vol_6bit >> i) & 1) ? HIGH : LOW);
  }
}

bool VolumeController::is_muted() const
{
  return mute_button_.get_state();
}

int32_t VolumeController::get_volume_db() const
{
  return persistent_data_ptr_->get_volume_db();
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

  // Update volume DB in the persistent data
  persistent_data_ptr_->get_volume_db_mutable() = map(volume_, 0, total_tick_for_63db_, -63, 1);
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
    digitalWrite(set_mute_pin_, is_muted() ? LOW : HIGH);  // Mute is active low
  }
  return has_changed;
}

bool VolumeController::update()
{
  bool change = false;
  change |= update_volume();
  change |= update_mute();
  return change;
}