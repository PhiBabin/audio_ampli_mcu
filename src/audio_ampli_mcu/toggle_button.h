#ifndef TOG_BUTTON_GUARD_H_
#define TOG_BUTTON_GUARD_H_

#include "InputDebounce.h"

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

class ToggleButton : public InputDebounce
{
public:
  // Constructor
  ToggleButton(
    const int8_t pin_in = -1,
    unsigned long deb_delay = DEFAULT_INPUT_DEBOUNCE_DELAY,
    PinInMode pin_in_mode = PIM_INT_PULL_UP_RES,
    unsigned long pressed_duration = 0);
  virtual ~ToggleButton() = default;

  // Get current state of the toggle button
  bool get_state() const;

protected:
  virtual void pressed();

private:
  bool state_;
};

#endif  // TOG_BUTTON_GUARD_H_