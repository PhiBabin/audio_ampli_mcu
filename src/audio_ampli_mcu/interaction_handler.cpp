#include "interaction_handler.h"

InteractionHandler::InteractionHandler(
  OptionsView& option_view,
  MainMenuView& main_menu_view,
  VolumeController& volume_ctrl,
  StateMachine& state_machine,
  PioEncoder& option_encoder)
  : option_view_(option_view)
  , main_menu_view_(main_menu_view)
  , volume_ctrl_(volume_ctrl)
  , state_machine_(state_machine)
  , option_encoder_(option_encoder)
{
}

void InteractionHandler::init()
{
  select_button_.setup(pin_out::select_button.pin, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
  mute_button_.setup(pin_out::mute_button.pin, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);
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
  unsigned long now = millis();
  select_button_.process(now);

  if (select_button_.is_short_press())
  {
    on_menu_press();
  }
  else if (select_button_.is_long_press())
  {
    if (state_machine_.get_state() == State::option_menu)
    {
      state_machine_.change_state(State::main_menu);
    }
    else
    {
      state_machine_.change_state(State::option_menu);
    }
  }
  return select_button_.is_short_press() || select_button_.is_long_press();
}

void InteractionHandler::on_mute_button_press()
{
  volume_ctrl_.toggle_mute();
  volume_ctrl_.set_gpio_based_on_volume();
}

bool InteractionHandler::update_mute_button()
{
  unsigned long now = millis();
  mute_button_.process(now);
  if (mute_button_.is_short_press())
  {
    on_mute_button_press();
  }
  else if (mute_button_.is_long_press())
  {
    on_power_button_press();
  }
  return mute_button_.is_short_press() || mute_button_.is_long_press();
}

bool InteractionHandler::update_encoder()
{
  const int32_t current_count = option_encoder_.getCount();

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
  switch (state_machine_.get_state())
  {
    case State::main_menu:
      main_menu_view_.menu_change(dir);
      break;
    case State::option_menu:
      option_view_.menu_change(dir);
      break;
    case State::standby:
      // TODO standby view
      break;
  }
}

void InteractionHandler::on_power_button_press()
{
  if (state_machine_.get_state() != State::standby)
  {
    option_view_.power_off();
    state_machine_.change_state(State::standby);
  }
  else
  {
    option_view_.power_on();
    state_machine_.change_state(State::main_menu);
  }
}
void InteractionHandler::on_menu_press()
{
  switch (state_machine_.get_state())
  {
    case State::main_menu:
      state_machine_.change_state(State::option_menu);
      break;
    case State::option_menu:
      option_view_.on_menu_press();
      break;
    case State::standby:
      break;
  }
}
