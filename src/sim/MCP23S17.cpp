#include "sim/MCP23S17.h"

#include "sim/arduino.h"

#include <cassert>
#include <tuple>
#include <vector>

MCP23S17::MCP23S17(uint8_t chip_select) : chip_select_(chip_select)
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
  if (port == 1)
  {
    print_status();
  }
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
  const static std::vector<std::tuple<const char*, int>> table1 = {
    {{"in_bal", 0},
     {"in_rca1", 1},
     {"in_rca2", 2},
     {"in_rca3", 3},
     {"in_out_unipolar", 4},
     {"in_out_bal_unipolar", 5},
     {"in_phono_pin", 6},
     {"low_gain", 8},
     {"out_bal", 9},
     {"out_pream", 10},
     {"out_se", 11},
     {"out_lfe_bal", 12},
     {"out_lfe_se", 13},
     {"trigger_12v", 15}}};
  const static std::vector<std::tuple<const char*, int>> table2 = {
    {{"out_gain_0_pin", 15},
     {"out_gain_1_pin", 1},
     {"out_gain_2_pin", 2},
     {"out_res_0_pin", 7},
     {"out_res_1_pin", 6},
     {"out_res_2_pin", 5},
     {"out_cap_0_pin", 4},
     {"out_cap_1_pin", 3},
     {"out_rumble_filter_pin", 14}}};
  const auto& table = chip_select_ == 7 ? table1 : table2;
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