#ifndef TOG_BUTTON_GUARD_H_
#define TOG_BUTTON_GUARD_H_

#include <cstdint>

#define DEFAULT_INPUT_DEBOUNCE_DELAY 20  // [ms]

class InputDebounce
{
public:
  enum PinInMode
  {
    PIM_EXT_PULL_DOWN_RES,
    PIM_EXT_PULL_UP_RES,
    PIM_INT_PULL_UP_RES
  };
  enum SwitchType
  {
    ST_NORMALLY_OPEN,
    ST_NORMALLY_CLOSED
  };
};

void toggle_button(const uint8_t pin);

class ToggleButton : public InputDebounce
{
public:
  // Constructor
  ToggleButton(
    const int8_t pin_in = -1,
    unsigned long deb_delay = DEFAULT_INPUT_DEBOUNCE_DELAY,
    InputDebounce::PinInMode pin_in_mode = InputDebounce::PIM_INT_PULL_UP_RES,
    unsigned long pressed_duration = 0);
  ~ToggleButton() = default;
  void setup(
    int8_t pinIn,
    unsigned long debounceDelay = DEFAULT_INPUT_DEBOUNCE_DELAY,
    PinInMode pinInMode = PIM_INT_PULL_UP_RES,
    unsigned long pressedDurationMode = 0,
    SwitchType switchType = ST_NORMALLY_OPEN);

  // Get current state of the toggle button
  bool get_state() const;
  void process(unsigned long);

private:
  bool state_{false};
  int8_t pin_in_;
};

#endif  // TOG_BUTTON_GUARD_H_