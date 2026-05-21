/*****************************************************************************
* | File      	:	LCD_Driver.h
* | Author      :   Waveshare team
* | Function    :   LCD driver
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2018-12-18
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

/// This is the driver to communicate with the ST7789V screen display controler.
/// A 3-Line Serial Interface II is used to push data to the display controler, this is really slow, so everything is
/// drawn in 12bit/pixel (RGB444).

#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include "pinout_config.h"

#include <stdint.h>
#include <stdio.h>

#ifdef SIM
#include "sim/arduino.h"
#include "sim/SPI.h"
#else
#include "SPI.h"
#endif

#define LCD_BACKLIGHT 57  // Between 0-256 for 0 to 100% brightness

#define LCD_WIDTH 320   // LCD width
#define LCD_HEIGHT 240  // LCD height

/**
 * Registers
 */

#define LCD_REG_COL_ADDR_SET 0x2a
#define LCD_REG_ROW_ADDR_SET 0x2b
#define LCD_REG_MEM_WRITE 0x2C

/**
 * GPIO read and write
 **/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0 ? LOW : HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

#define DEV_SPI_BEGIN_TRANS SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
#define DEV_SPI_END_TRANS SPI.endTransaction();
/**
 * SPI
 **/
#define DEV_SPI_WRITE(_dat) SPI.transfer(_dat)

/**
 * PWM_BL
 **/
#define DEV_Set_PWM(_Value) analogWrite(pin_out::lcd_backlight.pin, _Value)

#define FRAME_BUFFER_LEN (LCD_WIDTH * LCD_HEIGHT * 3 / 2)

class Display
{
public:
  void gpio_init();
  void init();
  void set_backlight(uint16_t value);

  void blip_framebuffer();

  void clear_screen(const uint32_t color_12bit);
  void draw_rectangle(
    const uint16_t x_start_,
    const uint16_t y_start_,
    const uint16_t x_end_,
    const uint16_t y_end_,
    const uint32_t color_12bit);
  void set_pixel(const uint16_t x, const uint16_t y, const uint32_t color_12bit);
  void set_pixel_unsafe(const uint16_t x, const uint16_t y, const uint32_t color_12bit);

  /// Replace every other pixel with black (checkerboard pattern) for a dissolve transition.
  /// Operates directly on the framebuffer for maximum speed.
  void checkerboard_dissolve();

  /// Start a vertical melt transition. Call advance_transition() each frame until it returns true.
  void start_melt();
  /// Start a roto-zoom transition (spins the old screen into a vortex).
  void start_roto_zoom();
  /// Advance the active transition by one frame. Returns true when the transition is complete.
  bool advance_transition();
  /// Returns true if a transition is currently running.
  bool is_transition_active() const { return transition_type_ != TransitionType::none; }

private:
  enum class TransitionType : uint8_t { none, melt, roto_zoom };

  TransitionType transition_type_{TransitionType::none};
  uint8_t transition_frame_{0};
  unsigned long start_time_{0};
  uint8_t melt_front_[LCD_WIDTH];
  uint8_t* roto_backup_{nullptr};
  uint8_t rot_angle_{0};

  bool advance_melt();
  bool advance_roto_zoom();

private:
  void set_window(const uint16_t x_start, const uint16_t y_start, const uint16_t x_end, const uint16_t y_end);

  uint8_t frame_buffer_[FRAME_BUFFER_LEN] = {0};
  bool has_screen_changed{true};
};

#endif
