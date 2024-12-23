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
  DEV_Delay_ms(200);
  DEV_Digital_Write(DEV_RST_PIN, 0);
  DEV_Delay_ms(200);
  DEV_Digital_Write(DEV_RST_PIN, 1);
  DEV_Delay_ms(200);
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

/******************************************************************************
function:	Set the cursor position
parameter	:
    Xstart: 	Start uint16_t x coordinate
    Ystart:	Start uint16_t y coordinate
    Xend  :	End uint16_t coordinates
    Yend  :	End uint16_t coordinatesen
******************************************************************************/
void LCD_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
  LCD_Write_Command(0x2a);  // (2Ah): Column Address Set
  LCD_WriteData_Byte(Xstart >> 8);
  LCD_WriteData_Byte(Xstart & 0xff);
  LCD_WriteData_Byte((Xend - 1) >> 8);
  LCD_WriteData_Byte((Xend - 1) & 0xff);

  LCD_Write_Command(0x2b);  // (2Bh): Row Address Set
  LCD_WriteData_Byte(Ystart >> 8);
  LCD_WriteData_Byte(Ystart & 0xff);
  LCD_WriteData_Byte((Yend - 1) >> 8);
  LCD_WriteData_Byte((Yend - 1) & 0xff);

  LCD_Write_Command(0x2C);  // (2Ch): Memory write
}

void LCD_Clear_12bitRGB(uint32_t color_12bit)
{
  LCD_ClearWindow_12bitRGB(0, 0, LCD_WIDTH, LCD_HEIGHT, color_12bit);
}

void LCD_ClearWindow_12bitRGB(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint32_t color_12bit)
{
  unsigned int i, j;
  if ((Xend - Xstart) % 2 != 0)
  {
    ++Xend;
  }
  DEV_SPI_BEGIN_TRANS;
  LCD_SetWindow(Xstart, Ystart, Xend, Yend);
  DEV_Digital_Write(DEV_CS_PIN, 0);
  DEV_Digital_Write(DEV_DC_PIN, 1);
  for (j = Ystart; j < Yend; ++j)
  {
    for (i = 0; i < (Xend - Xstart) / 2; ++i)
    {
      DEV_SPI_WRITE((color_12bit >> 4) & 0xff);                                   // 8 MSb
      DEV_SPI_WRITE(((color_12bit & 0xf) << 4) + ((color_12bit & 0x0f00) >> 8));  // 4 LSb + 4 MSb
      DEV_SPI_WRITE(color_12bit & 0xff);                                          // 8 LSb
    }
  }
  DEV_Digital_Write(DEV_CS_PIN, 1);
  DEV_SPI_END_TRANS;
}

void LCD_write_2pixel_color(const uint32_t color_2pixels)
{
  DEV_Digital_Write(DEV_CS_PIN, 0);
  DEV_Digital_Write(DEV_DC_PIN, 1);
  DEV_SPI_WRITE((color_2pixels >> 16) & 0xff);
  DEV_SPI_WRITE((color_2pixels >> 8) & 0xff);
  DEV_SPI_WRITE(color_2pixels & 0xff);
  DEV_Digital_Write(DEV_CS_PIN, 1);
}