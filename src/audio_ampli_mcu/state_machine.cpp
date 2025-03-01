#include "state_machine.h"

void StateMachine::change_state(const State new_state)
{
  state_has_changed_ = new_state != state_;
  state_ = new_state;
}

State StateMachine::get_state() const
{
  return state_;
}

bool StateMachine::update()
{
  if (state_has_changed_)
  {
    state_has_changed_ = false;
    return true;
  }
  return false;
}