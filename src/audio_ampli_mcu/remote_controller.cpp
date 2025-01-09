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
  // Apple Remote 1294
  RemoteCallbacks apple;
  apple[0xA] = std::bind(&RemoteController::handle_up, this);
  apple[0xC] = std::bind(&RemoteController::handle_down, this);
  apple[0x9] = std::bind(&RemoteController::handle_vol_down, this);
  apple[0x6] = std::bind(&RemoteController::handle_vol_up, this);
  apple[0x5] = std::bind(&RemoteController::handle_select, this);
  apple[0x3] = std::bind(&RemoteController::handle_menu, this);
  remotes_mapping_.emplace(0x87EE, std::move(apple));

  // Cheap Amazon clone of the Apple Siri remote
  // This remote use the same codes as the apple remove for the arrow + select + menu
  RemoteCallbacks amazon_volume;
  amazon_volume[0x2] = std::bind(&RemoteController::handle_vol_up, this);
  amazon_volume[0x3] = std::bind(&RemoteController::handle_vol_down, this);
  amazon_volume[0x9] = std::bind(&RemoteController::handle_mute, this);
  amazon_volume[0x8] = std::bind(&RemoteController::handle_power_on_off, this);
  remotes_mapping_.emplace(0xFB04, std::move(amazon_volume));
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
  const uint16_t address = TinyIRReceiverData.Address;
  const uint16_t command = TinyIRReceiverData.Command;
  const auto iter = remotes_mapping_.find(address);
  if (iter == remotes_mapping_.end())
  {
    return false;
  }
  const auto& supported_commands = iter->second;

  const auto cmd_iter = supported_commands.find(command);
  if (cmd_iter == supported_commands.end())
  {
    return false;
  }

  // Call command's callback
  cmd_iter->second();
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
    audio_input_ctrl_ptr_->menu_up();
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
    audio_input_ctrl_ptr_->menu_down();
  }
}

void RemoteController::handle_vol_down()
{
  volume_ctrl_ptr_->increase_volume_db(-volume_change);
}

void RemoteController::handle_vol_up()
{
  volume_ctrl_ptr_->increase_volume_db(volume_change);
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

void RemoteController::handle_power_on_off()
{
  if (state_machine_ptr_->get_state() == State::standby)
  {
    volume_ctrl_ptr_->power_on();
  }
  else
  {
    volume_ctrl_ptr_->power_off();
  }
}

void RemoteController::handle_mute()
{
  volume_ctrl_ptr_->toggle_mute();
}