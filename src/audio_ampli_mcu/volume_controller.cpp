#include "volume_controller.h"


#define BUTTON_DEBOUNCE_DELAY   20   // [ms]

 VolumeController::VolumeController(const std::array<pin_size_t, 6> gpio_pin_vol_select, PioEncoder* vol_encoder_ptr, const int mute_button_pin, const int32_t startup_volume_db, const int32_t total_tick_for_63db)
  : gpio_pin_vol_select_(gpio_pin_vol_select)
  , mute_button_pin_(mute_button_pin)
  , volume_(map(startup_volume_db, 0, 63, 0, total_tick_for_63db))
  , prev_encoder_count_(0)
  , vol_encoder_ptr_(vol_encoder_ptr)
  , total_tick_for_63db_(total_tick_for_63db)
{}

void VolumeController::init()
{
  mute_button_.setup(mute_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  for (const auto& pin : gpio_pin_vol_select_)
  {
    pinMode(pin, OUTPUT);
  }
  set_gpio_based_on_volume();
}

void VolumeController::set_gpio_based_on_volume()
{
  // Map volume to 6 bit (64 state)
  const uint8_t vol_6bit = static_cast<uint8_t>(map(volume_, 0, total_tick_for_63db_, 0, 63));
  for (int i = 0; i < gpio_pin_vol_select_.size(); ++i)
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
  if (volume_ < 0)
  {
    return 0;
  }
  return map(volume_, 0, total_tick_for_63db_, 0, 63);
}

bool VolumeController::update_volume()
{
  const int32_t current_count = vol_encoder_ptr_->getCount();
  if (current_count == prev_encoder_count_)
  {
    return false;
  }
  // Apply volume change
  volume_ += current_count - prev_encoder_count_;
  // Wrap arround
  volume_ = constrain(volume_, 0, total_tick_for_63db_ - 1);
  prev_encoder_count_ = current_count;

  set_gpio_based_on_volume();
  return true;
}


bool VolumeController::update_mute()
{
  const bool prev_mute_state = mute_button_.get_state();
  unsigned long now = millis();
  mute_button_.process(now);
  return mute_button_.get_state() != prev_mute_state;
}

bool VolumeController::update()
{
  bool change = false;
  change |= update_volume();
  change |= update_mute();
  return change;
}