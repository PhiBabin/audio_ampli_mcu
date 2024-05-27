#include "sim/MCP23S17.h"

#include "sim/arduino.h"

#include <array>
#include <cassert>
#include <tuple>

MCP23S17::MCP23S17(uint8_t)
{
  value_gpio_[0] = 0;
  value_gpio_[1] = 0;
  direction_gpio_[0] = 0xff;
  direction_gpio_[1] = 0xff;
}
bool MCP23S17::begin(bool pullup)
{
  return true;
}
bool MCP23S17::write8(uint8_t port, uint8_t value)
{
  assert(port < 2);
  value_gpio_[port] = value;
  print_status();
  return true;
}

bool MCP23S17::pinMode8(uint8_t port, uint8_t value)
{
  assert(port < 2);
  direction_gpio_[port] = value;
  return true;
}

void MCP23S17::print_status()
{
  const std::array<std::tuple<const char*, int>, 9> table = {
    {{"in_bal", 0},
     {"in_rca1", 1},
     {"in_rca2", 2},
     {"in_rca3", 3},
     {"in_out_unipolar", 4},
     {"in_out_bal_unipolar", 5},
     {"low_gain", 8},
     {"out_bal", 9},
     {"pream_out", 10}}};
  for (const auto& [label, abs_pin] : table)
  {
    uint8_t port = 0;
    uint8_t pin = abs_pin;
    if (pin > 7)
    {
      port = 1;
      pin -= 8;
    }
    const int value = ((value_gpio_[port] >> pin) & 1);
    const char* dir_str = ((direction_gpio_[port] >> pin) & 1) == 1 ? "(INPUT)" : "(OUTPUT)";
    Serial.print(label);
    Serial.print(": ");
    Serial.print(value);
    Serial.print(" ");
    Serial.println(dir_str);
  }
  Serial.println("");
}