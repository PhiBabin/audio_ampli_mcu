#include "io_expander.h"

IoExpander::IoExpander(const int iox_chip_select_pin) : io_expander_(iox_chip_select_pin)
{
}

void IoExpander::begin()
{
  const auto result = io_expander_.begin(false);
  if (!result)
  {
    Serial.println("Failed to initialize io expander communication");
  }
  // Set all io as input
  io_expander_.pinMode8(0, 0xFF);
  io_expander_.pinMode8(1, 0xFF);
  direction_gpio_[0] = 0xff;
  direction_gpio_[1] = 0xff;

  // Set all value as low
  io_expander_.pinMode8(0, 0);
  io_expander_.pinMode8(1, 0);
  value_gpio_[0] = 0;
  value_gpio_[1] = 0;
}

void IoExpander::cache_write_pin(int pin, const uint8_t value)
{
  size_t port = 0;
  if (pin > 7)
  {
    port = 1;
    pin -= 8;
  }
  auto& current_direction = direction_gpio_[port];
  auto& current_value = value_gpio_[port];

  const uint8_t mask = 1 << pin;

  // Set direction to 0 (OUTPUT)
  current_direction &= ~mask;

  // Set value
  if (value != 0)
  {
    current_value |= mask;
  }
  else
  {
    current_value &= ~mask;
  }
}

void IoExpander::apply_write()
{
  for (int port = 0; port < 2; ++port)
  {
    io_expander_.pinMode8(port, direction_gpio_[port]);
    io_expander_.write8(port, value_gpio_[port]);
  }
}

void IoExpander::write_pin(const int pin, const uint8_t value)
{
  cache_write_pin(pin, value);

  const size_t port = pin > 7 ? 1 : 0;
  io_expander_.pinMode8(port, direction_gpio_[port]);
  io_expander_.write8(port, value_gpio_[port]);
}
