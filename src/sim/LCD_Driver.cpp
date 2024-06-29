#include "sim/LCD_Driver.h"

#include <SDL.h>
#include <cassert>
#include <tuple>

/// It's around 1333ms/px in theory with 20MHz, 10bit per bytes and 2 px per 3 bytes
constexpr uint64_t ms_per_pixel = 1333 / 2;
uint64_t pixel_count = 0;

SDL_Surface* global_surface = nullptr;
uint32_t win_start_x = 0;
uint32_t win_start_y = 0;
uint32_t win_end_x = 0;
uint32_t win_end_y = 0;
uint32_t win_curr_x = 0;
uint32_t win_curr_y = 0;

std::function<void(void)> blip_sdl_window_callback = nullptr;

void LCD_hook_sdl(SDL_Surface* surface, std::function<void(void)> funct)
{
  global_surface = surface;
  blip_sdl_window_callback = funct;
}

void LCD_GPIO_Init()
{
}

void LCD_Init(void)
{
}

void LCD_SetBackLight(uint16_t Value)
{
}

void LCD_Clear_12bitRGB(uint32_t color_12bit)
{
  LCD_ClearWindow_12bitRGB(0, 0, LCD_WIDTH, LCD_HEIGHT, color_12bit);
}

void LCD_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
  assert(Xstart <= Xend);
  assert(Ystart <= Yend);
  assert(Xstart <= LCD_WIDTH);
  assert(Xend <= LCD_WIDTH);
  assert(Ystart <= LCD_HEIGHT);
  assert(Yend <= LCD_HEIGHT);

  win_start_x = Xstart;
  win_start_y = Ystart;
  win_end_x = Xend;
  win_end_y = Yend;
  win_curr_x = Xstart;
  win_curr_y = Ystart;
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

void LCD_ClearWindow_12bitRGB(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint32_t color_12bit)
{
  assert(global_surface != nullptr);
  const auto [r, g, b] = rgb444_to_rgb888(color_12bit);
  SDL_Rect rect;
  rect.x = Xstart;
  rect.y = Ystart;
  rect.w = Xend - Xstart;
  rect.h = Yend - Ystart;
  SDL_FillRect(global_surface, &rect, SDL_MapRGB(global_surface->format, r, g, b));

  pixel_count += rect.w * rect.h;
  if (pixel_count > ms_per_pixel)
  {
    SDL_Delay(pixel_count / ms_per_pixel);
    pixel_count = 0;
    blip_sdl_window_callback();
  }
}

void LCD_write_2pixel_color(const uint32_t color_2pixels)
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
  if (pixel_count > ms_per_pixel)
  {
    SDL_Delay(pixel_count / ms_per_pixel);
    pixel_count = 0;
    blip_sdl_window_callback();
  }
}