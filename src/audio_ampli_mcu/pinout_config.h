#ifndef PINOUT_CONFIG_GUARD_H_
#define PINOUT_CONFIG_GUARD_H_

#include <array>
#include <stdint.h>

enum class GpioModule : uint8_t
{
  // GPIO directly connected to pico
  // Named GPX e.g. GP22
  pico = 0,
  // GPIO connected to main IO expander
  // Named GPAX or GPBX e.g. GPB3
  io_expander_a = 1,
  // GPIO connected to the IO expander on the optional Phono card
  // Named GPAX or GPBX e.g. GPB3
  io_expander_phono = 2,

  enum_length
};

enum class GpioPort : uint8_t
{
  // Not applicable / there is only a single bus (i.e. pico)
  na = 0,
  // Bus A (e.g. GPA3)
  a = 1,
  // Bus B (e.g. GPB3)
  b = 2,
  // Bus C (e.g. GPC3)
  c = 3,
  // Bus D (e.g. GPD3)
  d = 4
};

struct GpioPin
{
  GpioModule module;
  GpioPort port;
  uint8_t pin;
};

namespace pin_out
{
static constexpr GpioPin volume_bit0{GpioModule::pico, GpioPort::na, 22};
static constexpr GpioPin volume_bit1{GpioModule::pico, GpioPort::na, 4};
static constexpr GpioPin volume_bit2{GpioModule::pico, GpioPort::na, 5};
static constexpr GpioPin volume_bit3{GpioModule::pico, GpioPort::na, 9};
static constexpr GpioPin volume_bit4{GpioModule::pico, GpioPort::na, 10};
static constexpr GpioPin volume_bit5{GpioModule::pico, GpioPort::na, 11};

inline constexpr std::array volume_bits{volume_bit0, volume_bit1, volume_bit2, volume_bit3, volume_bit4, volume_bit5};

static constexpr GpioPin volume_encoder_b{GpioModule::pico, GpioPort::na, 18};
static constexpr GpioPin volume_encoder_a{GpioModule::pico, GpioPort::na, 19};
static constexpr GpioPin menu_select_encoder_b{GpioModule::pico, GpioPort::na, 20};
static constexpr GpioPin menu_select_encoder_a{GpioModule::pico, GpioPort::na, 21};

static constexpr GpioPin mute_button{GpioModule::pico, GpioPort::na, 16};
static constexpr GpioPin select_button{GpioModule::pico, GpioPort::na, 17};

static constexpr GpioPin io_expander_chip_select{GpioModule::pico, GpioPort::na, 7};
static constexpr GpioPin phono_io_expander_chip_select{GpioModule::pico, GpioPort::na, 26};

static constexpr GpioPin bias_pwm{GpioModule::pico, GpioPort::na, 27};
static constexpr GpioPin power_enable{GpioModule::pico, GpioPort::na, 14};

static constexpr GpioPin latch_left_vol{GpioModule::pico, GpioPort::na, 28};
static constexpr GpioPin latch_right_vol{GpioModule::pico, GpioPort::na, 15};

static constexpr GpioPin lcd_chip_select{GpioModule::pico, GpioPort::na, 1};
static constexpr GpioPin lcd_dc{GpioModule::pico, GpioPort::na, 8};
static constexpr GpioPin lcd_reset{GpioModule::pico, GpioPort::na, 12};
static constexpr GpioPin lcd_backlight{GpioModule::pico, GpioPort::na, 13};

static constexpr GpioPin spi_clk{GpioModule::pico, GpioPort::na, 2};
static constexpr GpioPin spi_mosi{GpioModule::pico, GpioPort::na, 3};
static constexpr GpioPin spi_miso{GpioModule::pico, GpioPort::na, 0};

static constexpr GpioPin ir_receive{GpioModule::pico, GpioPort::na, 6};

static constexpr GpioPin audio_in_select_bal{GpioModule::io_expander_a, GpioPort::a, 0};
static constexpr GpioPin audio_in_select_rca1{GpioModule::io_expander_a, GpioPort::a, 1};
static constexpr GpioPin audio_in_select_rca2{GpioModule::io_expander_a, GpioPort::a, 2};
static constexpr GpioPin audio_in_select_rca3{GpioModule::io_expander_a, GpioPort::a, 3};

inline constexpr std::array audio_input_pins{
  audio_in_select_bal, audio_in_select_rca1, audio_in_select_rca2, audio_in_select_rca3};

static constexpr GpioPin in_out_unipolar{GpioModule::io_expander_a, GpioPort::a, 4};
static constexpr GpioPin in_out_bal_unipolar{GpioModule::io_expander_a, GpioPort::a, 5};
static constexpr GpioPin in_phono{GpioModule::io_expander_a, GpioPort::a, 6};
static constexpr GpioPin set_low_gain{GpioModule::io_expander_a, GpioPort::b, 0};
static constexpr GpioPin out_bal{GpioModule::io_expander_a, GpioPort::b, 1};
static constexpr GpioPin preamp_out{GpioModule::io_expander_a, GpioPort::b, 2};
static constexpr GpioPin out_se{GpioModule::io_expander_a, GpioPort::b, 3};
static constexpr GpioPin out_lfe_bal{GpioModule::io_expander_a, GpioPort::b, 4};
static constexpr GpioPin out_lfe_se{GpioModule::io_expander_a, GpioPort::b, 5};

static constexpr GpioPin trigger_12v{GpioModule::io_expander_a, GpioPort::b, 7};

static constexpr GpioPin out_gain_0{GpioModule::io_expander_phono, GpioPort::b, 7};
static constexpr GpioPin out_gain_1{GpioModule::io_expander_phono, GpioPort::a, 1};
static constexpr GpioPin out_gain_2{GpioModule::io_expander_phono, GpioPort::a, 2};
static constexpr GpioPin out_res_0{GpioModule::io_expander_phono, GpioPort::a, 7};
static constexpr GpioPin out_res_1{GpioModule::io_expander_phono, GpioPort::a, 6};
static constexpr GpioPin out_res_2{GpioModule::io_expander_phono, GpioPort::a, 5};
static constexpr GpioPin out_cap_0{GpioModule::io_expander_phono, GpioPort::a, 4};
static constexpr GpioPin out_cap_1{GpioModule::io_expander_phono, GpioPort::a, 3};
static constexpr GpioPin out_rumble_filter{GpioModule::io_expander_phono, GpioPort::b, 6};

}  // namespace pin_out

#endif  // PINOUT_CONFIG_GUARD_H_