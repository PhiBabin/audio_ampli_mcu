#ifndef IOX_EXPANDER_GUARD_H_
#define IOX_EXPANDER_GUARD_H_

#ifdef SIM
#include "sim/MCP23S17.h"
#include "sim/arduino.h"
#else
#include "MCP23S17.h"
#endif

#include <cstdint>

class IoExpander
{
public:
  IoExpander(const int iox_chip_select_pin);
  void begin();
  void write_pin(const int pin, const uint8_t value);
  void cache_write_pin(int pin, const uint8_t value);
  void apply_write();

private:
  // IO expander driver
  MCP23S17 io_expander_;
  // Cache of the current GPIO direction for port A and B
  uint8_t direction_gpio_[2];
  // Cache of the current GPIO value for port A and B
  uint8_t value_gpio_[2];
};

#endif  // IOX_EXPANDER_GUARD_H_