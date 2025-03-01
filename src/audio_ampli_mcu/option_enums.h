#ifndef OPTIONS_ENUM_GUARD_H_
#define OPTIONS_ENUM_GUARD_H_

#include <stdint.h>

enum class OptionMenuScreen : uint8_t
{
  // Main option menu
  main = 0,
  // Adance option menu
  advance,
  // Support info menu
  firmware_version,
  // Get out of the option menu and come back to the Main Volume screeen
  exit,
  enum_length
};
enum class Option : uint8_t
{
  gain = 0,
  output_mode,
  output_type,
  subwoofer,
  balance,
  bias,
  rename_bal,
  rename_rca1,
  rename_rca2,
  rename_rca3,
  more_options,
  back,
  audio_input,
  text,
  enum_length
};

enum class GainOption : uint8_t
{
  low = 0,
  high,
  enum_length
};

enum class OutputModeOption : uint8_t
{
  phones = 0,
  line_out,
  enum_length
};

enum class OutputTypeOption : uint8_t
{
  se = 0,
  bal,
  enum_length
};

enum class InputNameAliasOption : uint8_t
{
  no_alias = 0,
  dac,
  cd,
  phono,
  tuner,
  aux,
  stream,
  enum_length
};

enum class OnOffOption : uint8_t
{
  off = 0,
  on,
  enum_length
};

enum class IncrementDir : uint8_t
{
  increment = 0,
  decrement
};

constexpr uint8_t NUM_OUTPUT_MODE = static_cast<uint8_t>(OutputModeOption::enum_length);
constexpr uint8_t NUM_OUTPUT_TYPE = static_cast<uint8_t>(OutputTypeOption::enum_length);
#endif  // OPTIONS_ENUM_GUARD_H_