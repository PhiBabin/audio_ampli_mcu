#ifndef IOX_EXPANDER_GUARD_H_
#define IOX_EXPANDER_GUARD_H_

#ifdef SIM
#include "sim/MCP23S17.h"
#include "sim/arduino.h"
#else
#include "MCP23S17.h"
#endif

#include "pinout_config.h"

#include <cstdint>
#include <optional>

class IoExpander
{
public:
  IoExpander(const int iox_chip_select_pin);
  void begin();
  bool is_connected();
  // void write_pin(const int pin, const uint8_t value);
  // void cache_write_pin(int pin, const uint8_t value);
  void apply();

  void init_input(const GpioPort& port, const uint8_t pin);
  void cache_init_input(const GpioPort& port, const uint8_t pin);
  void write_pin(const GpioPort& port, const uint8_t pin, const uint8_t value);
  void cache_write_pin(const GpioPort& port, const uint8_t pin, const uint8_t value);
  void cache_init_output(const GpioPort& port, const uint8_t pin, const uint8_t value);

private:
  uint8_t port_enum_to_port_idx(const GpioPort& port);

  // IO expander driver
  MCP23S17 io_expander_;
  // Cache of the current GPIO direction for port A and B
  uint8_t direction_gpio_[2];
  // Cache of the current GPIO value for port A and B
  uint8_t value_gpio_[2];
  // If is_connected() was called, the result is cached in this variable
  std::optional<bool> maybe_is_connected_;
};

#endif  // IOX_EXPANDER_GUARD_H_