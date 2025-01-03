#include "sim/lcd_simulator.h"

#include "audio_ampli_mcu/LCD_Driver.h"
#include "sim/arduino.h"

#include <SDL.h>
#include <cassert>
#include <iostream>
#include <optional>
#include <tuple>

/// It's around 1.33us/px in theory with 20MHz, 10bit per bytes and 2 px per 3 bytes, in practive it's 1us/px
constexpr uint64_t pixel_per_ms = 997;
uint64_t pixel_count = 0;

SDL_Surface* global_surface = nullptr;
uint32_t win_start_x = 0;
uint32_t win_start_y = 0;
uint32_t win_end_x = 0;
uint32_t win_end_y = 0;
uint32_t win_curr_x = 0;
uint32_t win_curr_y = 0;

std::function<void(void)> blip_sdl_window_callback = nullptr;

void hook_sdl_surface_for_lcd_simulator(SDL_Surface* surface, std::function<void(void)> funct)
{
  global_surface = surface;
  blip_sdl_window_callback = funct;
}

std::tuple<uint8_t, uint8_t, uint8_t> rgb444_to_rgb888(const uint32_t color_12bit)
{
  auto map_4b_to_8bit = [](const uint32_t v) -> uint8_t {
    if (v == 0xf)
    {
      return 0xff;
    }
    return v << 4;
  };
  const auto r = map_4b_to_8bit((color_12bit >> 8) & 0xf);
  const auto g = map_4b_to_8bit((color_12bit >> 4) & 0xf);
  const auto b = map_4b_to_8bit((color_12bit >> 0) & 0xf);
  return std::make_tuple(r, g, b);
}

void write_2pixel_color_to_sdl_surface(const uint32_t color_2pixels)
{
  if (win_curr_y >= win_end_y)
  {
    return;
  }
  const uint32_t left_pixel_color = (color_2pixels >> 12) & 0xFFF;
  const uint32_t right_pixel_color = (color_2pixels >> 0) & 0xFFF;

  auto sdl_draw_pixel = [](const auto x, const auto y, const uint32_t color12bit) {
    uint8_t* const target_pixel =
      ((uint8_t*)global_surface->pixels + y * global_surface->pitch + x * global_surface->format->BytesPerPixel);

    const auto [r, g, b] = rgb444_to_rgb888(color12bit);
    // Due to little endianness it's BGR, not RGB
    target_pixel[0] = b;
    target_pixel[1] = g;
    target_pixel[2] = r;
  };
  sdl_draw_pixel(win_curr_x, win_curr_y, left_pixel_color);
  sdl_draw_pixel(win_curr_x + 1, win_curr_y, right_pixel_color);
  win_curr_x += 2;
  if (win_curr_x >= win_end_x)
  {
    win_curr_x = win_start_x;
    ++win_curr_y;
  }

  pixel_count += 2;
  if (pixel_count > pixel_per_ms)
  {
    SDL_Delay(pixel_count / pixel_per_ms);
    pixel_count = 0;
    blip_sdl_window_callback();
  }
}

// State of the SPI processing
std::optional<uint8_t> maybe_command;
std::vector<uint8_t> spi_data;

void LCD_set_column_address()
{
  if (spi_data.size() != 4)
  {
    return;
  }
  win_start_x = (spi_data[0] << 8) | spi_data[1];
  win_end_x = (spi_data[2] << 8) | spi_data[3];
  ++win_end_x;
  win_curr_x = win_start_x;

  assert(win_start_x <= win_end_x);
  assert(win_end_x <= LCD_WIDTH);
}

void LCD_set_row_address()
{
  if (spi_data.size() != 4)
  {
    return;
  }
  win_start_y = (spi_data[0] << 8) | spi_data[1];
  win_end_y = (spi_data[2] << 8) | spi_data[3];
  ++win_end_y;
  win_curr_y = win_start_y;

  assert(win_start_y <= win_end_y);
  assert(win_end_y <= LCD_HEIGHT);
}

void LCD_write_mem()
{
  if (spi_data.size() != 3)
  {
    return;
  }

  const uint32_t color_2pixel = (spi_data[0] << 16) | (spi_data[1] << 8) | (spi_data[2] << 0);
  write_2pixel_color_to_sdl_surface(color_2pixel);
  spi_data.clear();
}

void LCD_process_spi_data(const uint8_t data)
{
  // Skip if no chip select
  if (digitalRead(DEV_CS_PIN) == 1)
  {
    return;
  }

  // Is command?
  if (digitalRead(DEV_DC_PIN) == 0)
  {
    if (maybe_command && maybe_command.value() != data)
    {
      spi_data.clear();
    }
    maybe_command = data;
    return;
  }
  if (!maybe_command)
  {
    return;
  }
  spi_data.push_back(data);
  // Only process set window and write to screen commands
  switch (maybe_command.value())
  {
    case LCD_REG_COL_ADDR_SET:
      LCD_set_column_address();
      break;
    case LCD_REG_ROW_ADDR_SET:
      LCD_set_row_address();
      break;
    case LCD_REG_MEM_WRITE:
      LCD_write_mem();
      break;
  }
}