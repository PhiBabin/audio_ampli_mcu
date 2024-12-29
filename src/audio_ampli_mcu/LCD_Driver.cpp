/*****************************************************************************
* | File      	:	LCD_Driver.c
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
#include "LCD_Driver.h"

#include "RP2040_PWM.h"

#include <cassert>

/// PWM instance to control backlight
RP2040_PWM* PWM_Instance;

void LCD_GPIO_Init(void)
{

  pinMode(DEV_CS_PIN, OUTPUT);
  pinMode(DEV_RST_PIN, OUTPUT);
  pinMode(DEV_DC_PIN, OUTPUT);
  pinMode(DEV_BL_PIN, OUTPUT);
  pinMode(DEV_DIN_PIN, OUTPUT);
  pinMode(DEV_DOUT_PIN, INPUT);

  // Frequency high enough to be filter out by the ampli
  constexpr uint32_t pwm_frequency = 20000;
  PWM_Instance = new RP2040_PWM(DEV_BL_PIN, pwm_frequency, LCD_BACKLIGHT);
  PWM_Instance->setPWM(DEV_BL_PIN, pwm_frequency, LCD_BACKLIGHT);

  SPI.setSCK(DEV_CLK_PIN);
  SPI.setCS(DEV_CS_PIN);
  SPI.setRX(DEV_DOUT_PIN);
  SPI.setTX(DEV_DIN_PIN);
  SPI.begin(/*hwCS = */ false);
}

/*******************************************************************************
function:
  Hardware reset
*******************************************************************************/
static void LCD_Reset(void)
{
  delay(200);
  DEV_Digital_Write(DEV_RST_PIN, 0);
  delay(200);
  DEV_Digital_Write(DEV_RST_PIN, 1);
  delay(200);
}

/*******************************************************************************
function:
    Write data and commands
*******************************************************************************/
static void LCD_Write_Command(uint8_t data)
{
  DEV_Digital_Write(DEV_CS_PIN, 0);
  DEV_Digital_Write(DEV_DC_PIN, 0);
  DEV_SPI_WRITE(data);
}

static void LCD_WriteData_Byte(uint8_t data)
{
  DEV_Digital_Write(DEV_CS_PIN, 0);
  DEV_Digital_Write(DEV_DC_PIN, 1);
  DEV_SPI_WRITE(data);
  DEV_Digital_Write(DEV_CS_PIN, 1);
}

void LCD_WriteData_Word(uint16_t data)
{
  DEV_Digital_Write(DEV_CS_PIN, 0);
  DEV_Digital_Write(DEV_DC_PIN, 1);
  DEV_SPI_WRITE((data >> 8) & 0xff);
  DEV_SPI_WRITE(data);
  DEV_Digital_Write(DEV_CS_PIN, 1);
}

/******************************************************************************
function:
    Common register initialization
******************************************************************************/
void LCD_Init(void)
{
  DEV_SPI_BEGIN_TRANS;
  LCD_Reset();

  LCD_Write_Command(0x36);  // MADCTL (36h): Memory Data Access Control
  LCD_WriteData_Byte(0xA0);

  LCD_Write_Command(0x3A);  // COLMOD (3Ah): Interface Pixel Format
  // LCD_WriteData_Byte(0x05); // 16bit/pixel
  LCD_WriteData_Byte(0x03);  // 12bit/pixel

  LCD_Write_Command(0x21);  // INVON (21h): Display Inversion O

  LCD_Write_Command(0x2A);  // 2Ah: Column Address Set
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0x01);
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0x3F);

  LCD_Write_Command(0x2B);  // (2Bh): Row Address Se
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0xEF);

  LCD_Write_Command(0xB2);  //  (B2h): Porch Setting
  LCD_WriteData_Byte(0x0C);
  LCD_WriteData_Byte(0x0C);
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0x33);
  LCD_WriteData_Byte(0x33);

  LCD_Write_Command(0xB7);  // (B7h): Gate Control
  LCD_WriteData_Byte(0x35);

  LCD_Write_Command(0xBB);  // (BBh): VCOMS Setting
  LCD_WriteData_Byte(0x1F);

  LCD_Write_Command(0xC0);  //  (C0h): LCM Control
  LCD_WriteData_Byte(0x2C);

  LCD_Write_Command(0xC2);  //  (C2h): VDV and VRH Command Enable
  LCD_WriteData_Byte(0x01);

  LCD_Write_Command(0xC3);  // (C3h): VRH Se
  LCD_WriteData_Byte(0x12);

  LCD_Write_Command(0xC4);  //  (C4h): VDV Set
  LCD_WriteData_Byte(0x20);

  LCD_Write_Command(0xC6);   //  (C6h): Frame Rate Control in Normal Mode
  LCD_WriteData_Byte(0x0F);  // 60hz

  LCD_Write_Command(0xD0);  //  (D0h): Power Control 1
  LCD_WriteData_Byte(0xA4);
  LCD_WriteData_Byte(0xA1);

  LCD_Write_Command(0xE0);  //  (E0h): Positive Voltage Gamma Control
  LCD_WriteData_Byte(0xD0);
  LCD_WriteData_Byte(0x08);
  LCD_WriteData_Byte(0x11);
  LCD_WriteData_Byte(0x08);
  LCD_WriteData_Byte(0x0C);
  LCD_WriteData_Byte(0x15);
  LCD_WriteData_Byte(0x39);
  LCD_WriteData_Byte(0x33);
  LCD_WriteData_Byte(0x50);
  LCD_WriteData_Byte(0x36);
  LCD_WriteData_Byte(0x13);
  LCD_WriteData_Byte(0x14);
  LCD_WriteData_Byte(0x29);
  LCD_WriteData_Byte(0x2D);

  LCD_Write_Command(0xE1);  //  (E1h): Negative Voltage Gamma Contro
  LCD_WriteData_Byte(0xD0);
  LCD_WriteData_Byte(0x08);
  LCD_WriteData_Byte(0x10);
  LCD_WriteData_Byte(0x08);
  LCD_WriteData_Byte(0x06);
  LCD_WriteData_Byte(0x06);
  LCD_WriteData_Byte(0x39);
  LCD_WriteData_Byte(0x44);
  LCD_WriteData_Byte(0x51);
  LCD_WriteData_Byte(0x0B);
  LCD_WriteData_Byte(0x16);
  LCD_WriteData_Byte(0x14);
  LCD_WriteData_Byte(0x2F);
  LCD_WriteData_Byte(0x31);
  LCD_Write_Command(0x21);  //  (21h): Display Inversion On

  LCD_Write_Command(0x11);  //  (11h): Sleep Out

  LCD_Write_Command(0x29);  //  (29h): Display On

  DEV_SPI_END_TRANS;
}

void Display::gpio_init()
{
  LCD_GPIO_Init();
}

void Display::init()
{
  LCD_Init();
}

void Display::set_backlight(uint16_t value)
{
  // TODO
}

void Display::set_window(const uint16_t x_start, const uint16_t y_start, const uint16_t x_end, const uint16_t y_end)
{
  LCD_Write_Command(LCD_REG_COL_ADDR_SET);
  LCD_WriteData_Byte(x_start >> 8);
  LCD_WriteData_Byte(x_start & 0xff);
  LCD_WriteData_Byte((x_end - 1) >> 8);
  LCD_WriteData_Byte((x_end - 1) & 0xff);

  LCD_Write_Command(LCD_REG_ROW_ADDR_SET);
  LCD_WriteData_Byte(y_start >> 8);
  LCD_WriteData_Byte(y_start & 0xff);
  LCD_WriteData_Byte((y_end - 1) >> 8);
  LCD_WriteData_Byte((y_end - 1) & 0xff);

  LCD_Write_Command(LCD_REG_MEM_WRITE);
}

void Display::blip_framebuffer()
{
  DEV_SPI_BEGIN_TRANS;
  set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);
  DEV_Digital_Write(DEV_CS_PIN, 0);
  DEV_Digital_Write(DEV_DC_PIN, 1);
  SPI.transfer(frame_buffer_, nullptr, FRAME_BUFFER_LEN);
  DEV_Digital_Write(DEV_CS_PIN, 1);
  DEV_SPI_END_TRANS;
}

void Display::clear_screen(const uint32_t color_12bit)
{
  const uint8_t byte0 = (color_12bit >> 4) & 0xff;                                   // 8 MSb
  const uint8_t byte1 = ((color_12bit & 0xf) << 4) | ((color_12bit & 0x0f00) >> 8);  // 4 LSb + 4 MSb
  const uint8_t byte2 = color_12bit & 0xff;                                          // 8 LSb
  // Optimization: if all bytes are the same, just use memset. In most case, we are writing all black anyway.
  if (byte0 == byte1 && byte1 == byte2)
  {
    memset(frame_buffer_, byte0, FRAME_BUFFER_LEN);
  }
  else
  {
    assert(FRAME_BUFFER_LEN % 3 == 0);  // TODO: handle case where the number of pixel is not a multiple of 3
    for (uint32_t i = 0; i < FRAME_BUFFER_LEN; i += 3)
    {
      frame_buffer_[i] = byte0;
      frame_buffer_[i + 1] = byte1;
      frame_buffer_[i + 2] = byte2;
    }
  }
}

void Display::set_rectangle(
  const uint16_t x_start_,
  const uint16_t y_start_,
  const uint16_t x_end_,
  const uint16_t y_end_,
  const uint32_t color_12bit)
{
  auto x_end = x_end_ > LCD_WIDTH ? LCD_WIDTH : x_end_;
  auto x_start = x_start_ > x_end ? x_end : x_start_;
  const auto y_end = y_end_ > LCD_HEIGHT ? LCD_HEIGHT : y_end_;
  const auto y_start = y_start_ > y_end ? y_end : y_start_;

  // Serial.print("\tx_start=");
  // Serial.print(x_start);
  // Serial.print(" x_end=");
  // Serial.print(x_end);
  // Serial.print(" y_start=");
  // Serial.print(y_start);
  // Serial.print(" y_end=");
  // Serial.print(y_end);
  // Serial.println("");

  for (uint16_t y = y_start; y < y_end; ++y)
  {
    for (uint16_t x = x_start; x < x_end; ++x)
    {
      set_pixel_unsafe(x, y, color_12bit);
    }
  }

  // if x_start starts on an half bytes, set the MSB nibble first for all line
  // const uint32_t x_start_byte_offset = x_start * 3 / 2;
  // if ((x_start_byte_offset & 1) == 1)
  // {
  //   Serial.print("\tx start is offset at ");
  //   Serial.print(x_start_byte_offset);
  //   Serial.println("");
  //   for (uint16_t y = y_start; y < y_end; ++y)
  //   {
  //     const uint32_t px_offset = static_cast<uint32_t>(y) * LCD_WIDTH + x_start;
  //     const uint32_t bytes_offset = px_offset * 3 / 2;
  //     frame_buffer_[bytes_offset] = (frame_buffer_[bytes_offset] & 0xf0) | ((color_12bit & 0xf0) >> 4);
  //     frame_buffer_[bytes_offset + 1] = color_12bit & 0xff;  // 8 LSb
  //   }
  //   // Now we can skip the first vertical line
  //   ++x_start;
  // }
  // // if x_end - 1 is on lie on a bytes, we must set all the LSB nibble for all line
  // const uint32_t x_end_byte_offset = (x_end - 1) * 3 / 2;
  // if ((x_end_byte_offset & 1) == 0)
  // {
  //   Serial.print("\tx end is offset at ");
  //   Serial.print(x_end_byte_offset);
  //   Serial.println("");
  //   for (uint16_t y = y_start; y < y_end; ++y)
  //   {
  //     const uint32_t px_offset = static_cast<uint32_t>(y) * LCD_WIDTH + (x_end - 1);
  //     const uint32_t bytes_offset = px_offset * 3 / 2;

  //     frame_buffer_[bytes_offset] = (color_12bit >> 4) & 0xff;  // 8 MSb
  //     frame_buffer_[bytes_offset + 1] =
  //       ((color_12bit & 0xf) << 4) | (frame_buffer_[bytes_offset + 1] & 0x0f);  // 4 LSb + next pixel's 4 MSb;
  //   }
  //   // Now we can skip the last vertical line
  //   --x_end;
  // }

  // // Now that all start and end x edge case has been taken care off, we can write all the remaining pixels

  // const uint8_t byte0 = (color_12bit >> 4) & 0xff;                                   // 8 MSb
  // const uint8_t byte1 = ((color_12bit & 0xf) << 4) | ((color_12bit & 0x0f00) >> 8);  // 4 LSb + 4 MSb
  // const uint8_t byte2 = color_12bit & 0xff;                                          // 8 LSb
  // for (uint32_t y = y_start; y < y_end; ++y)
  // {
  //   if ((x_end - x_start) % 2 != 0)
  //   {
  //     Serial.println("failed check");
  //   }
  //   // assert((x_end - x_start) % 2 == 0);  // TODO: handle case where the number of pixel is not a multiple of 3
  //   for (uint32_t x = x_start; x < x_end; x += 2)
  //   {
  //     const uint32_t px_offset = y * LCD_WIDTH + x;
  //     const uint32_t bytes_offset = px_offset * 3 / 2;
  //     frame_buffer_[bytes_offset] = byte0;
  //     frame_buffer_[bytes_offset + 1] = byte1;
  //     frame_buffer_[bytes_offset + 2] = byte2;
  //   }
  // }
}

void Display::set_pixel_unsafe(const uint16_t x, const uint16_t y, const uint32_t color_12bit)
{
  const uint32_t px_offset = static_cast<uint32_t>(y) * LCD_WIDTH + x;
  const uint32_t bytes_offset = px_offset * 3 / 2;
  // Because pixels take 1.5bytes, we need to handle the case where the pixel is at the start of a byte or at the end
  if ((px_offset & 1) == 0)
  {
    frame_buffer_[bytes_offset] = (color_12bit >> 4) & 0xff;  // 8 MSb
    frame_buffer_[bytes_offset + 1] =
      ((color_12bit & 0xf) << 4) | (frame_buffer_[bytes_offset + 1] & 0x0f);  // 4 LSb + next pixel's 4 MSb;
  }
  else
  {
    frame_buffer_[bytes_offset] =
      (frame_buffer_[bytes_offset] & 0xf0) | ((color_12bit & 0xf0) >> 4);  // previous pixel's 4 LSb + 4 MSb
    frame_buffer_[bytes_offset + 1] = color_12bit & 0xff;                  // 8 LSb
  }
}
void Display::set_pixel(const uint16_t x, const uint16_t y, const uint32_t color_12bit)
{
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT)
  {
    return;
  }
  set_pixel_unsafe(x, y, color_12bit);
}
