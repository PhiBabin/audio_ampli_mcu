#include "sim/toggle_button.h"

ToggleButton::ToggleButton(const int8_t pin_in, unsigned long deb_delay, PinInMode pin_in_mode, unsigned long pressed_duration)
    : state_(false)
  {}

  void ToggleButton::setup(int8_t pinIn,
            unsigned long debounceDelay,
            PinInMode pinInMode,
            unsigned long pressedDurationMode,
            SwitchType switchType)
  {}
  
  bool ToggleButton::get_state() const
  {
    return state_;
  }

  void ToggleButton::pressed()
  {
    state_ = !state_;
  }
  
  void ToggleButton::process(unsigned long)
  {}