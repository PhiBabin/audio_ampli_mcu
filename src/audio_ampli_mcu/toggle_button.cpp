#include "toggle_button.h"

ToggleButton::ToggleButton(const int8_t pin_in, unsigned long deb_delay, PinInMode pin_in_mode, unsigned long pressed_duration)
    : InputDebounce(pin_in, deb_delay, pin_in_mode, pressed_duration)
    , state_(false)
  {}
  
  bool ToggleButton::get_state() const
  {
    return state_;
  }

  void ToggleButton::pressed()
  {
    state_ = !state_;
  }