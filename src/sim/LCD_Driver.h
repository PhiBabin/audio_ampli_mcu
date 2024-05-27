#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include <cstdint>
#include <functional>

#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

#define LCD_WIDTH 320   // LCD width
#define LCD_HEIGHT 240  // LCD height

#define DEV_SPI_BEGIN_TRANS
#define DEV_SPI_END_TRANS

// forward declaration
class SDL_Surface;

void LCD_hook_sdl(SDL_Surface* surface, std::function<void(void)> funct);

void Config_Init();

void LCD_write_2pixel_color(const uint32_t color_2pixels);

void LCD_SetWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend);

void LCD_Init(void);
void LCD_SetBackLight(UWORD Value);

void LCD_Clear_12bitRGB(uint32_t color_12bit);
void LCD_ClearWindow_12bitRGB(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, uint32_t color_12bit);

#endif