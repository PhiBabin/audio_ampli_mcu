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

  // Overload of the process() method, this overload just clear out the flags.
  virtual unsigned long process(unsigned long now) final;

  // Get current state of the toggle button
  bool get_state() const;
  // Was there a long press detected since the last call to process() ?
  bool is_long_press() const;
  // Was there a short press detected since the last call to process() ?
  bool is_short_press() const;

protected:
  virtual void pressed() final;
  virtual void pressedDuration(unsigned long duration) final;
  virtual void releasedDuration(unsigned long duration) final;

private:
  bool state_;
  bool flag_short_pressed_{false};
  bool flag_long_pressed_{false};
  bool was_flag_long_pressed_triggered_{true};
};

#endif  // TOG_BUTTON_GUARD_H_