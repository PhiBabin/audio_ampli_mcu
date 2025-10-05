#ifndef OPTIONS_ENUM_GUARD_H_
#define OPTIONS_ENUM_GUARD_H_

#include <stdint.h>

enum class OptionMenuScreen : uint8_t
{
  // Main option menu
  main = 0,
  // Advance option menu
  advance,
  // Phono option menu
  phono,
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
  phono_mode,
  phono_gain,
  resistance_load,
  capacitance_load,
  rumble_filter,
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

enum class PhonoMode : uint8_t
{
  mm = 0,
  mc,
  enum_length
};

enum class MMPhonoGain : uint8_t
{
  gain_40dB = 0,
  gain_45dB,
  gain_50dB,
  enum_length
};

enum class MCPhonoGain : uint8_t
{
  gain_55dB = 0,
  gain_60dB,
  gain_65dB,
  enum_length
};

enum class PhonoResistanceLoad : uint8_t
{
  r_47k = 0,
  r_1k,
  r_400,
  r_300,
  r_200,
  r_100,
  r_50,
  r_30,
  enum_length
};

enum class PhonoCapacitanceLoad : uint8_t
{
  c_0f = 0,
  c_100pf,
  c_470pf,
  c_1000pf,
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