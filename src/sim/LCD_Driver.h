#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include <cstdint>
#include <functional>

#define LCD_WIDTH 320   // LCD width
#define LCD_HEIGHT 240  // LCD height

#define DEV_SPI_BEGIN_TRANS
#define DEV_SPI_END_TRANS

// forward declaration
class SDL_Surface;

void LCD_hook_sdl(SDL_Surface* surface, std::function<void(void)> funct);

void LCD_GPIO_Init();

void LCD_write_2pixel_color(const uint32_t color_2pixels);

void LCD_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);

void LCD_Init(void);

void LCD_Clear_12bitRGB(uint32_t color_12bit);
void LCD_ClearWindow_12bitRGB(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint32_t color_12bit);

#endif