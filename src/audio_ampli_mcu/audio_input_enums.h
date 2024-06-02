#ifndef AUDIO_INPUT_ENUM_GUARD_H_
#define AUDIO_INPUT_ENUM_GUARD_H_

#include <stdint.h>

enum class AudioInput : uint8_t
{
  bal = 0,
  rca_1,
  rca_2,
  rca_3,
  audio_input_enum_length
};

constexpr uint8_t NUM_AUDIO_INPUT = static_cast<uint8_t>(AudioInput::audio_input_enum_length);

#endif  // AUDIO_INPUT_ENUM_GUARD_H_