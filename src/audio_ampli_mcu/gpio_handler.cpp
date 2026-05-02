#include "gpio_handler.h"

#ifdef SIM
#include "sim/arduino.h"
#else
#endif

GpioHandler::GpioHandler(std::vector<ModuleEnumExpanderPair> io_expanders_ptrs)
  : io_expanders_ptrs_(std::move(io_expanders_ptrs))
{
  for (const auto& [module_enum, io_expander_ptr] : io_expanders_ptrs_)
  {
    gpio_module_to_io_expander_[static_cast<uint8_t>(module_enum)] = io_expander_ptr;
  }
}

bool GpioHandler::is_module_connected(const GpioModule& module)
{
  if (module == GpioModule::pico)
  {
    return true;
  }
  auto io_expander_ptr = try_get_io_expander(module);
  if (io_expander_ptr == nullptr)
  {
    return false;
  }
  return io_expander_ptr->is_connected();
}

void GpioHandler::set_dirty(const GpioModule& module)
{
  const auto module_idx = static_cast<uint8_t>(module);
  dirty_flag_ |= (1 << module_idx);
}

IoExpander* GpioHandler::try_get_io_expander(const GpioModule& module)
{
  if (module >= GpioModule::enum_length)
  {
    return nullptr;
  }
  if (module == GpioModule::pico)
  {
    return nullptr;
  }
  return gpio_module_to_io_expander_[static_cast<uint8_t>(module)];
}

void GpioHandler::cache_init_input(const GpioPin pin)
{
  if (pin.module == GpioModule::pico)
  {
    pinMode(pin.pin, INPUT);
    return;
  }

  auto io_expander_ptr = try_get_io_expander(pin.module);
  if (io_expander_ptr == nullptr)
  {
    return;
  }
  io_expander_ptr->init_input(pin.port, pin.pin);
  set_dirty(pin.module);
}

void GpioHandler::cache_init_output(const GpioPin pin, const uint8_t value)
{
  if (pin.module == GpioModule::pico)
  {
    pinMode(pin.pin, OUTPUT);
    digitalWrite(pin.pin, value == 0 ? LOW : HIGH);
    return;
  }

  auto io_expander_ptr = try_get_io_expander(pin.module);
  if (io_expander_ptr == nullptr)
  {
    return;
  }
  io_expander_ptr->cache_write_pin(pin.port, pin.pin, value);
  set_dirty(pin.module);
}

void GpioHandler::cache_write_pin(const GpioPin pin, const uint8_t value)
{
  if (pin.module == GpioModule::pico)
  {
    digitalWrite(pin.pin, value == 0 ? LOW : HIGH);
    return;
  }

  auto io_expander_ptr = try_get_io_expander(pin.module);
  if (io_expander_ptr == nullptr)
  {
    return;
  }
  io_expander_ptr->cache_write_pin(pin.port, pin.pin, value);
  set_dirty(pin.module);
}

void GpioHandler::write_pin(const GpioPin pin, const uint8_t value)
{
  if (pin.module == GpioModule::pico)
  {
    digitalWrite(pin.pin, value == 0 ? LOW : HIGH);
    return;
  }

  auto io_expander_ptr = try_get_io_expander(pin.module);
  if (io_expander_ptr == nullptr)
  {
    return;
  }
  io_expander_ptr->write_pin(pin.port, pin.pin, value);
}

void GpioHandler::apply()
{
  // For all possible
  for (uint8_t i = 0; i < static_cast<uint8_t>(GpioModule::enum_length); ++i)
  {
    if (dirty_flag_ & (1 << i))
    {
      IoExpander* io_expander = gpio_module_to_io_expander_[i];
      if (io_expander != nullptr)
      {
        io_expander->apply();
      }
    }
  }
  // Reset dirty flags
  dirty_flag_ = 0;
}
