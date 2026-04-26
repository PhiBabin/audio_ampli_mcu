#ifndef GPIO_HANDLER_GUARD_H_
#define GPIO_HANDLER_GUARD_H_

#include "io_expander.h"
#include "pinout_config.h"

#include <vector>

/// Handles all physical input (buttons and encoders) and call the appropriate view
class GpioHandler
{
public:
  using ModuleEnumExpanderPair = std::pair<GpioModule, IoExpander*>;

  GpioHandler(std::vector<ModuleEnumExpanderPair> io_expanders_ptrs);

  void cache_init_input(const GpioPin pin);
  void cache_init_output(const GpioPin pin, const uint8_t value);
  void cache_write_pin(const GpioPin pin, const uint8_t value);
  void write_pin(const GpioPin pin, const uint8_t value);
  void apply();

  bool is_module_connected(const GpioModule& module);

private:
  IoExpander* try_get_io_expander(const GpioModule& module);
  void set_dirty(const GpioModule& module);

  std::vector<ModuleEnumExpanderPair> io_expanders_ptrs_;
  // Quick lookup table to get the io expander pointer
  std::array<IoExpander*, static_cast<uint8_t>(GpioModule::enum_length)> gpio_module_to_io_expander_;
  // Each bit represent if there is a cached init or write since the last time apply() was called.
  uint8_t dirty_flag_{0};
};

#endif  // GPIO_HANDLER_GUARD_H_