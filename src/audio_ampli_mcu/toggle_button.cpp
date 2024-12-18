#include "toggle_button.h"

#define LONG_PRESS_MS 1000

ToggleButton::ToggleButton(
  const int8_t pin_in, unsigned long deb_delay, PinInMode pin_in_mode, unsigned long pressed_duration)
  : InputDebounce(pin_in, deb_delay, pin_in_mode, pressed_duration), state_(false)
{
}

unsigned long ToggleButton::process(unsigned long now)
{
  // Set flags for pressing
  flag_long_pressed_ = false;
  flag_short_pressed_ = false;
  // Let base class do the regular processing
  return InputDebounce::process(now);
}

void ToggleButton::pressedDuration(unsigned long duration)
{
  if (duration > LONG_PRESS_MS && was_released)
  {
    flag_long_pressed_ = true;
    was_released = false;
  }
}

void ToggleButton::pressed()
{
  state_ = !state_;
}

void ToggleButton::releasedDuration(unsigned long duration)
{
  // We assumes here that if the button was held long enough to be long press, the press callback would have caught it
  if (duration < LONG_PRESS_MS)
  {
    flag_short_pressed_ = true;
  }
  was_released = true;
}

bool ToggleButton::is_long_press() const
{
  return flag_long_pressed_;
}

bool ToggleButton::is_short_press() const
{
  return flag_short_pressed_;
}

bool ToggleButton::get_state() const
{
  return state_;
}
