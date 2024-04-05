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

#include "DEV_Config.h"

#define LCD_WIDTH   320 //LCD width
#define LCD_HEIGHT  240 //LCD height

 
void LCD_WriteData_Word(UWORD da);

void LCD_SetCursor(UWORD X, UWORD Y);
void LCD_SetWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD  Yend);
void LCD_DrawPaint(UWORD x, UWORD y, UWORD Color);

void LCD_Init(void);
void LCD_SetBackLight(UWORD Value);

void LCD_Clear(UWORD Color);
void LCD_Clear_12bitRGB(uint32_t color_12bit);
void LCD_ClearWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,UWORD color);
void LCD_ClearWindow_12bitRGB(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, uint32_t color_12bit);

#endif
