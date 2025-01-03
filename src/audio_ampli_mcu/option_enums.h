#ifndef OPTIONS_ENUM_GUARD_H_
#define OPTIONS_ENUM_GUARD_H_

#include <stdint.h>

enum class Option : uint8_t
{
  gain = 0,
  output_mode,
  output_type,
  bias,
  subwoofer,
  back,
  option_enum_length
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

enum class OnOffOption : uint8_t
{
  off = 0,
  on,
  enum_length
};
constexpr uint8_t NUM_OUTPUT_MODE = static_cast<uint8_t>(OutputModeOption::enum_length);
constexpr uint8_t NUM_OUTPUT_TYPE = static_cast<uint8_t>(OutputTypeOption::enum_length);
#endif  // OPTIONS_ENUM_GUARD_H_