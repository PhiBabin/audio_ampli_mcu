#ifndef __MCP23S17_DRIVER_H
#define __MCP23S17_DRIVER_H

#include <cstdint>

class MCP23S17
{
public:
  //       SOFTWARE SPI
  MCP23S17(uint8_t select);
  bool begin(bool pullup = true);
  bool write8(uint8_t port, uint8_t value);
  bool pinMode8(uint8_t port, uint8_t value);

private:
  void print_status();

  // Cache of the current GPIO direction for port A and B
  uint8_t direction_gpio_[2];
  // Cache of the current GPIO value for port A and B
  uint8_t value_gpio_[2];
};

#endif  // __MCP23S17_DRIVER_H