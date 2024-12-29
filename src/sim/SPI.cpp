#include "sim/spi.h"
#include "sim/lcd_simulator.h"

SPIClass SPI;

SPISettings::SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
{
}

void SPIClass::setSCK(int pin)
{
}
void SPIClass::setCS(int pin)
{
}
void SPIClass::setRX(int pin)
{
}
void SPIClass::setTX(int pin)
{
}

void SPIClass::begin(bool hwCS)
{
}

void SPIClass::endTransaction()
{
}

void SPIClass::transfer(const void* txbuf_, void* rxbuf, size_t count)
{
  const uint8_t* txbuf = reinterpret_cast<const uint8_t*>(txbuf_);
  for (size_t i = 0; i < count; ++i)
  {
    LCD_process_spi_data(txbuf[i]);
  }
}

void SPIClass::transfer(const uint8_t& data)
{
  LCD_process_spi_data(data);
}

void SPIClass::beginTransaction(SPISettings settings)
{
}