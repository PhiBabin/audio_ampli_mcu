#include "sim/toggle_button.h"

bool button_state[40] = {0};

void toggle_button(const uint8_t pin)
{
  if (pin < 40)
  {
    button_state[pin] = !button_state[pin];
  }
}

ToggleButton::ToggleButton(
  const int8_t pin_in, unsigned long deb_delay, PinInMode pin_in_mode, unsigned long pressed_duration)
  : pin_in_(pin_in)
{
}

void ToggleButton::setup(
  int8_t pinIn,
  unsigned long debounceDelay,
  PinInMode pinInMode,
  unsigned long pressedDurationMode,
  SwitchType switchType)
{
  pin_in_ = pinIn;
}

bool ToggleButton::get_state() const
{
  return state_;
}

void ToggleButton::process(unsigned long)
{
  state_ = button_state[pin_in_];
}