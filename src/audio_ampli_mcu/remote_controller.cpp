#include "remote_controller.h"

#ifdef SIM
#include "sim/TinyIRReceiver.h"
#else
#include "TinyIRReceiver.hpp"
#endif

RemoteController::RemoteController(
  StateMachine* state_machine_ptr,
  OptionController* option_ctrl_ptr,
  AudioInputController* audio_input_ctrl_ptr,
  VolumeController* volume_ctrl_ptr)
  : state_machine_ptr_(state_machine_ptr)
  , option_ctrl_ptr_(option_ctrl_ptr)
  , audio_input_ctrl_ptr_(audio_input_ctrl_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
{
}

void RemoteController::init()
{
  if (!initPCIInterruptForTinyReceiver())
  {
    Serial.print("No interrupt available for pin ");
    Serial.println(IR_RECEIVE_PIN);
  }
}

bool RemoteController::decode_command()
{
  if (!TinyReceiverDecode())
  {
    return false;
  }

  constexpr auto apple_address = 0x87EE;
  if (TinyIRReceiverData.Address != apple_address)
  {
    return false;
  }

  switch (TinyIRReceiverData.Command)
  {
    // Up arrow
    case 0xA:
      handle_up();
      break;
    // Down arrow
    case 0xC:
      handle_down();
      break;
    // Left arrow
    case 0x9:
      handle_left();
      break;
    // Right arrow
    case 0x6:
      handle_right();
      break;
    // Center button (also pause/play button)
    case 0x5:
      handle_select();
      break;
    // Menu button
    case 0x3:
      handle_menu();
      break;
    // Invalid command
    default:
    {
      return false;
    }
  }
  return true;
}

void RemoteController::handle_up()
{
  if (state_machine_ptr_->get_state() == State::option_menu)
  {
    option_ctrl_ptr_->menu_up();
  }
  else
  {
    audio_input_ctrl_ptr_->menu_down();
  }
}
void RemoteController::handle_down()
{
  if (state_machine_ptr_->get_state() == State::option_menu)
  {
    option_ctrl_ptr_->menu_down();
  }
  else
  {
    audio_input_ctrl_ptr_->menu_up();
  }
}

void RemoteController::handle_left()
{
  volume_ctrl_ptr_->set_volume_db(volume_ctrl_ptr_->get_volume_db() - volume_change);
}
void RemoteController::handle_right()
{
  volume_ctrl_ptr_->set_volume_db(volume_ctrl_ptr_->get_volume_db() + volume_change);
}
void RemoteController::handle_menu()
{
  if (state_machine_ptr_->get_state() == State::option_menu)
  {
    state_machine_ptr_->change_state(State::main_menu);
  }
  else
  {
    state_machine_ptr_->change_state(State::option_menu);
  }
}
void RemoteController::handle_select()
{
  if (state_machine_ptr_->get_state() == State::option_menu)
  {
    option_ctrl_ptr_->on_menu_press();
  }
  else
  {
    state_machine_ptr_->change_state(State::option_menu);
  }
}