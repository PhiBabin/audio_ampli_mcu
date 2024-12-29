#ifndef ARDUINO_SPI_GUARD_H_
#define ARDUINO_SPI_GUARD_H_

#include <cstddef>
#include <cstdint>

#define LSBFIRST 0
#define MSBFIRST 1

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C     // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03    // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

class SPISettings
{
public:
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode);
};

/// Mock of the RP2040 SPI class. All transfers are forwarded to the LCD simulator
class SPIClass
{
public:
  void setSCK(int pin);
  void setCS(int pin);
  void setRX(int pin);
  void setTX(int pin);
  void begin(bool hwCS);
  void transfer(const uint8_t& data);
  void transfer(const void* txbuf, void* rxbuf, size_t count);
  void beginTransaction(SPISettings settings);
  void endTransaction();
};

extern SPIClass SPI;
#endif