#ifndef STATE_MACHINE_GUARD_H_
#define STATE_MACHINE_GUARD_H_

#include <cstdint>

enum class State : uint8_t
{
  main_menu = 0,
  option_menu,
  standby
};

class StateMachine
{
public:
  void change_state(const State new_state);
  State get_state() const;
  bool update();

private:
  State state_{State::main_menu};
  bool state_has_changed_{false};
};

#endif  // STATE_MACHINE_GUARD_H_