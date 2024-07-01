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
#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include <SPI.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>

#define LCD_BACKLIGHT 57  // Between 0-256 for 0 to 100% brightness

#define LCD_WIDTH 320   // LCD width
#define LCD_HEIGHT 240  // LCD height

/**
 * GPIO config
 **/
#define DEV_CS_PIN 1
#define DEV_DC_PIN 8
#define DEV_RST_PIN 12
#define DEV_BL_PIN 13

#define DEV_CLK_PIN 2
#define DEV_DIN_PIN 3
#define DEV_DOUT_PIN 0

/**
 * GPIO read and write
 **/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0 ? LOW : HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

#define DEV_SPI_BEGIN_TRANS SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE3));
#define DEV_SPI_END_TRANS SPI.endTransaction();
/**
 * SPI
 **/
#define DEV_SPI_WRITE(_dat) SPI.transfer(_dat)

/**
 * delay x ms
 **/
#define DEV_Delay_ms(__xms) delay(__xms)

/**
 * PWM_BL
 **/
#define DEV_Set_PWM(_Value) analogWrite(DEV_BL_PIN, _Value)

void LCD_GPIO_Init(void);

void LCD_Init(void);

void LCD_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);

void LCD_write_2pixel_color(const uint32_t color_2pixels);

void LCD_SetBackLight(uint16_t Value);

void LCD_Clear_12bitRGB(uint32_t color_12bit);
void LCD_ClearWindow_12bitRGB(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint32_t color_12bit);

#endif
