#include "interaction_handler.h"

InteractionHandler::InteractionHandler(
  OptionsView* option_view_ptr,
  MainMenuView* main_menu_view_ptr,
  VolumeController* volume_ctrl_ptr,
  StateMachine* state_machine_ptr,
  PioEncoder* option_encoder_ptr,
  const pin_size_t select_button_pin,
  const pin_size_t mute_button_pin)
  : option_view_ptr_(option_view_ptr)
  , main_menu_view_ptr_(main_menu_view_ptr)
  , volume_ctrl_ptr_(volume_ctrl_ptr)
  , state_machine_ptr_(state_machine_ptr)
  , option_encoder_ptr_(option_encoder_ptr)
  , select_button_pin_(select_button_pin)
  , mute_button_pin_(mute_button_pin)
{
}

void InteractionHandler::init()
{
  select_button_.setup(select_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  mute_button_.setup(mute_button_pin_, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
}

bool InteractionHandler::update()
{
  bool has_changed = update_selection();
  has_changed |= update_encoder();
  has_changed |= update_mute_button();
  return has_changed;
}

bool InteractionHandler::update_selection()
{
  const bool prev_select_state = select_button_.get_state();
  unsigned long now = millis();
  select_button_.process(now);
  if (select_button_.get_state() == prev_select_state)
  {
    return false;
  }

  on_menu_press();
  return true;
}

bool InteractionHandler::update_mute_button()
{
  unsigned long now = millis();
  mute_button_.process(now);
  if (mute_button_.is_short_press())
  {
    volume_ctrl_ptr_->toggle_mute();
    volume_ctrl_ptr_->set_gpio_based_on_volume();
  }
  else if (mute_button_.is_long_press())
  {
    on_power_button_press();
  }
  return mute_button_.is_short_press() || mute_button_.is_long_press();
}

bool InteractionHandler::update_encoder()
{
  const int32_t current_count = option_encoder_ptr_->getCount();

  if (current_count - prev_encoder_count_ >= TICK_PER_AUDIO_IN)
  {
    menu_change(IncrementDir::decrement);
    prev_encoder_count_ = current_count;
    return true;
  }
  if (current_count - prev_encoder_count_ <= -TICK_PER_AUDIO_IN)
  {
    menu_change(IncrementDir::increment);
    prev_encoder_count_ = current_count;
    return true;
  }
  return false;
}

void InteractionHandler::menu_change(const IncrementDir& dir)
{
  switch (state_machine_ptr_->get_state())
  {
    case State::main_menu:
      main_menu_view_ptr_->menu_change(dir);
      break;
    case State::option_menu:
      option_view_ptr_->menu_change(dir);
      break;
    case State::standby:
      // TODO standby view
      break;
  }
}

void InteractionHandler::on_power_button_press()
{
  if (state_machine_ptr_->get_state() != State::standby)
  {
    option_view_ptr_->power_off();
    state_machine_ptr_->change_state(State::standby);
  }
  else
  {
    option_view_ptr_->power_on();
    state_machine_ptr_->change_state(State::main_menu);
  }
}
void InteractionHandler::on_menu_press()
{
  switch (state_machine_ptr_->get_state())
  {
    case State::main_menu:
      state_machine_ptr_->change_state(State::option_menu);
      break;
    case State::option_menu:
      option_view_ptr_->on_menu_press();
      break;
    case State::standby:
      break;
  }
}