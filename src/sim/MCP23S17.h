#ifndef __MCP23S17_DRIVER_H
#define __MCP23S17_DRIVER_H

#include "sim/SPI.h"

#include <cstdint>

class MCP23S17
{
public:
  //       SOFTWARE SPI
  // MCP23S17(uint8_t select, uint8_t dataIn, uint8_t dataOut, uint8_t clock, uint8_t address = 0x00);
  // MCP23S17(int select, SPIClass* spi = &SPI);
  MCP23S17(int select, int address = 0x00, SPIClass* spi = &SPI);
  bool begin(bool pullup = true);
  bool write8(uint8_t port, uint8_t value);
  bool pinMode8(uint8_t port, uint8_t value);

  uint8_t getInterruptPolarity();
  //       polarity: 0 = LOW, 1 = HIGH, 2 = NONE/ODR
  bool setInterruptPolarity(uint8_t polarity);

private:
  void print_status();

  uint8_t chip_select_;
  uint8_t address_;
  // Cache of the current GPIO direction for port A and B
  uint8_t direction_gpio_[2];
  // Cache of the current GPIO value for port A and B
  uint8_t value_gpio_[2];
  // Value of the interrupt polarity register.
  uint8_t polarity_{0};
};

#endif  // __MCP23S17_DRIVER_H