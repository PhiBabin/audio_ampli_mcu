#ifndef __LCD_SIM_H
#define __LCD_SIM_H

#include <cstdint>
#include <functional>

// forward declaration
class SDL_Surface;

void hook_sdl_surface_for_lcd_simulator(SDL_Surface* surface, std::function<void(void)> funct);

void LCD_process_spi_data(const uint8_t data);

#endif