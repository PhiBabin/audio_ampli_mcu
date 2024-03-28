/*****************************************************************************
* | File        :   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*                Used to shield the underlying layers of each master 
*                and enhance portability
*----------------
* | This version:   V1.0
* | Date        :   2018-11-22
* | Info        :

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
#include "DEV_Config.h"

void GPIO_Init()
{
  pinMode(DEV_CS_PIN, OUTPUT);
  pinMode(DEV_RST_PIN, OUTPUT);
  pinMode(DEV_DC_PIN, OUTPUT);
  pinMode(DEV_BL_PIN, OUTPUT);
  pinMode(DEV_DIN_PIN, OUTPUT);
  pinMode(DEV_DOUT_PIN, INPUT);
  analogWrite(DEV_BL_PIN, 140);
}
 
void Config_Init()
{

  GPIO_Init();

  //spi
  // SPI.setDataMode(SPI_MODE3);
  // SPI.setBitOrder(MSBFIRST);
  // SPI.setClockDivider(SPI_CLOCK_DIV2);
  // Which pin is that?

  // #define DEV_CS_PIN  9
  // #define DEV_DC_PIN  8
  // #define DEV_RST_PIN 12
  // #define DEV_BL_PIN  13

  // // Pin not set by the script
  // #define DEV_CLK_PIN  10
  // #define DEV_DIN_PIN  11
  // #define DEV_DOUT_PIN  14 

  // SPI.setSCK(14);
  // SPI.setCS(13);
  // SPI.setRX(12);
  // SPI.setTX(11);

  SPI.setSCK(DEV_CLK_PIN);
  SPI.setCS(DEV_CS_PIN);
  SPI.setRX(DEV_DOUT_PIN);
  SPI.setTX(DEV_DIN_PIN);
  SPI.begin(/*hwCS = */ false);
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE3)); // I think SPI_CLOCK_DIV2 is around 8Mhz
}
