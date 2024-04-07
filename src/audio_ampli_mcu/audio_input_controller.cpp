#include "audio_input_controller.h"

const char* audio_input_to_string(const AudioInput audio_in)
{
  switch (audio_in)
  {
    case AudioInput::AUX_1:
      return "AUX 1";
    case AudioInput::AUX_2:
      return "AUX 2";
    case AudioInput::AUX_3:
      return "AUX 3";
    case AudioInput::BAL:
      return "BAL";
    default:
      return "INVALID";
  }
}

AudioInputController::AudioInputController(  // const std::array<pin_size_t, 6> gpio_pin_audio_in_select,  // <== Don't
                                             // know where they go

  StateMachine* state_machine_ptr,
  PioEncoder* audio_in_encoder_ptr,
  const AudioInput startup_audio_in,
  const int32_t tick_per_audio_in)
  : state_machine_ptr_(state_machine_ptr)
  , audio_input_(startup_audio_in)
  , prev_encoder_count_(0)
  , tick_per_audio_in_(tick_per_audio_in)
  , audio_in_encoder_ptr_(audio_in_encoder_ptr)
{
}

void AudioInputController::init()
{
}

AudioInput AudioInputController::get_audio_input() const
{
  return audio_input_;
}

bool AudioInputController::update()
{
  const int32_t current_count = audio_in_encoder_ptr_->getCount();
  if (state_machine_ptr_->get_state() == State::option_menu)
  {
    prev_encoder_count_ = current_count;
    return false;
  }

  const auto max_enum_value = static_cast<uint8_t>(AudioInput::audio_input_enum_length);
  auto audio_input_int = static_cast<uint8_t>(audio_input_);
  if (current_count - prev_encoder_count_ > tick_per_audio_in_)
  {
    audio_input_ = static_cast<AudioInput>((audio_input_int + 1) % max_enum_value);
    prev_encoder_count_ = current_count;
    return true;
  }
  if (-tick_per_audio_in_ > current_count - prev_encoder_count_)
  {
    if (audio_input_int == 0)
    {
      audio_input_int = max_enum_value - 1;
    }
    else
    {
      --audio_input_int;
    }
    audio_input_ = static_cast<AudioInput>(audio_input_int);
    prev_encoder_count_ = current_count;
    return true;
  }
  return false;
}
