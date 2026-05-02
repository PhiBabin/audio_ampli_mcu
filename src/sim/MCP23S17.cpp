#include "sim/MCP23S17.h"

#include "audio_ampli_mcu/pinout_config.h"
#include "sim/arduino.h"
#include "sim/sim_config.h"

#include <cassert>
#include <tuple>
#include <vector>

// MCP23S17::MCP23S17(uint8_t select, uint8_t dataIn, uint8_t dataOut, uint8_t clock, uint8_t address)
//   : MCP23S17(select, address)
// {
// }

// MCP23S17(int select, SPIClass* spi) : MCP23S17(select)
// {
// }

MCP23S17::MCP23S17(int chip_select, int address, SPIClass* spi)
  : chip_select_(chip_select)
  , address_(address)
  , gpio_module_(
      chip_select_ == pin_out::io_expander_chip_select.pin
        ? (address_ == 0x1 ? GpioModule::io_expander_2 : GpioModule::io_expander_1)
        : GpioModule::io_expander_phono)
{
  value_gpio_[0] = 0;
  value_gpio_[1] = 0;
  direction_gpio_[0] = 0xff;
  direction_gpio_[1] = 0xff;
}


void MCP23S17::enableHardwareAddress()
{

}

bool MCP23S17::begin(bool pullup)
{
  return true;
}

bool MCP23S17::write8(uint8_t port, uint8_t value)
{
  // Serial.print(static_cast<uint8_t>(gpio_module_));
  // Serial.print("> Send write8 port=");
  // Serial.println(port);
  assert(port < 2);
  value_gpio_[port] = value;
  if (port == 1)
  {
    print_status();
  }
  return true;
}

bool MCP23S17::setInterruptPolarity(uint8_t polarity)
{
#ifdef HAS_PHONO_CARD
  polarity_ = polarity;
#else
  if (chip_select_ != 26)
  {
    polarity_ = polarity;
  }
#endif
  return true;
}

uint8_t MCP23S17::getInterruptPolarity()
{
  return polarity_;
}

bool MCP23S17::pinMode8(uint8_t port, uint8_t value)
{
  assert(port < 2);
  if (port < 2)
  {
    // Serial.print(static_cast<uint8_t>(gpio_module_));
    // Serial.print("> Send pinMode8 port=");
    // Serial.println(port);
    direction_gpio_[port] = value;
    return true;
  }
  return false;
}

#define stringify_var(var) #var, pin_out::var

void MCP23S17::print_status()
{
    using NameToGpioVector = std::vector<std::tuple<const char*, const GpioPin&>>;
#if defined(USE_V2_PCB)
  const static NameToGpioVector table_iox1 = {
    {{stringify_var(audio_in_select_bal)},
     {stringify_var(audio_in_select_rca1)},
     {stringify_var(audio_in_select_rca2)},
     {stringify_var(audio_in_select_rca3)},
     {stringify_var(in_out_unipolar)},
     {stringify_var(in_out_bal_unipolar)},
     {stringify_var(in_phono)},
     {stringify_var(high_gain)}}};

  const static NameToGpioVector table_iox2 = {{
    {stringify_var(l_volume_bit4)},
    {stringify_var(l_volume_bit5)},
    {stringify_var(l_volume_bit6)},
    {stringify_var(r_volume_bit4)},
    {stringify_var(r_volume_bit5)},
    {stringify_var(r_volume_bit6)},
    {stringify_var(set_low_gain)},
    {stringify_var(out_bal)},
    {stringify_var(preamp_out)},
    {stringify_var(out_se)},
    {stringify_var(out_lfe_bal)},
    {stringify_var(out_lfe_se)},
    {stringify_var(set_mono)},
  }};
#else
  const static NameToGpioVector table_iox1 = {
    {{stringify_var(audio_in_select_bal)},
     {stringify_var(audio_in_select_rca1)},
     {stringify_var(audio_in_select_rca2)},
     {stringify_var(audio_in_select_rca3)},
     {stringify_var(in_out_unipolar)},
     {stringify_var(in_out_bal_unipolar)},
     {stringify_var(in_phono)},
     {stringify_var(set_low_gain)},
     {stringify_var(out_bal)},
     {stringify_var(preamp_out)},
     {stringify_var(out_se)},
     {stringify_var(out_lfe_bal)},
     {stringify_var(out_lfe_se)},
     {stringify_var(trigger_12v)}
    }};
  const static NameToGpioVector table_iox2 = {}; // Doesn't exist
#endif
  const static NameToGpioVector table_phono = {
    {{stringify_var(out_gain_0)},
     {stringify_var(out_gain_1)},
     {stringify_var(out_gain_2)},
     {stringify_var(out_res_0)},
     {stringify_var(out_res_1)},
     {stringify_var(out_res_2)},
     {stringify_var(out_cap_0)},
     {stringify_var(out_cap_1)},
     {stringify_var(out_rumble_filter)}}};
  const NameToGpioVector& table = [this]() -> const NameToGpioVector&
      {
          switch (gpio_module_)
          {
              default:
              case GpioModule::io_expander_1:
                {
                    return table_iox1;
                }
                case GpioModule::io_expander_2:
                {
                    return table_iox2;
                }
                case GpioModule::io_expander_phono:
                {
                    return table_phono;
                }
          }
      }();
  for (const auto& [label, abs_pin] : table)
  {
    uint8_t port = abs_pin.port == GpioPort::a ? 0 : 1;
    uint8_t pin = abs_pin.pin;
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
