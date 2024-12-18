#include "sim/toggle_button.h"

#include "sim/arduino.h"

#define LONG_PRESS_DURATION_MS 1000

bool button_state[40] = {0};
int when_press_ms[40] = {0};
int when_released_ms[40] = {0};

void button_pressed(const uint8_t pin, const bool is_repeat)
{
  if (pin < 40)
  {
    button_state[pin] = !button_state[pin];
    if (!is_repeat)
    {
      when_press_ms[pin] = millis();
      when_released_ms[pin] = -1;
    }
  }
}
void button_released(const uint8_t pin)
{
  if (pin < 40)
  {
    when_released_ms[pin] = millis();
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
  // Button was released
  if (when_press_ms[pin_in_] > 0 && when_released_ms[pin_in_] > 0)
  {
    const auto duration_ms = when_released_ms[pin_in_] - when_press_ms[pin_in_];
    flag_long_pressed_ = duration_ms > LONG_PRESS_DURATION_MS;
    flag_short_pressed_ = !flag_long_pressed_;
    when_press_ms[pin_in_] = -1;
    when_released_ms[pin_in_] = -1;
  }
  // Button has not yet been released, but it has been pressed long enough
  else if (when_press_ms[pin_in_] > 0 && millis() - when_press_ms[pin_in_] > LONG_PRESS_DURATION_MS)
  {
    flag_long_pressed_ = true;
    flag_short_pressed_ = false;
    when_press_ms[pin_in_] = -1;
    when_released_ms[pin_in_] = -1;
  }
  else
  {
    flag_long_pressed_ = false;
    flag_short_pressed_ = false;
  }
}

bool ToggleButton::is_long_press() const
{
  return flag_long_pressed_;
}

bool ToggleButton::is_short_press() const
{
  return flag_short_pressed_;
}
