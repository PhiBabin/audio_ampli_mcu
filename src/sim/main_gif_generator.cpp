#include "audio_ampli_mcu/LCD_Driver.h"
#include "sim/external/gif.h"
#include "sim/lcd_simulator.h"
#include "sim/pio_encoder.h"
#include "sim/toggle_button.h"

#include <SDL.h>
#include <stdio.h>

void setup();
void loop();

constexpr int delay_between_frame_ms = 1;
int main(int argc, char* args[])
{
  if (argc != 2)
  {
    printf("Invalid arguments, usage: gif_generator path/to/my_output.gif");
  }
  const char* filename = args[1];
  // The window we'll be rendering to
  // SDL_Window* window = NULL;

  // The surface contained by the window
  SDL_Surface* screenSurface = SDL_CreateRGBSurface(0, LCD_WIDTH, LCD_HEIGHT, 32, 0, 0, 0, 0);
  if (screenSurface == nullptr)
  {
    return -1;
  }

  auto do_nothing = []() {};
  std::function<void(void)> foo = do_nothing;
  hook_sdl_surface_for_lcd_simulator(screenSurface, foo);

  // Fill the surface white
  SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

  // Call arduino's setup
  setup();

  GifWriter g;
  GifBegin(&g, filename, LCD_WIDTH, LCD_HEIGHT, delay_between_frame_ms * 5);

  auto write_frame = [&](const auto delay) {
    GifWriteFrame(&g, reinterpret_cast<const uint8_t*>(screenSurface->pixels), LCD_WIDTH, LCD_HEIGHT, delay);
  };
  for (size_t i = 0; i < 75; ++i)
  {
    loop();
    write_frame(delay_between_frame_ms);
    decrement_encoder(18, 3);
  }
  write_frame(delay_between_frame_ms * 5);
  for (size_t i = 0; i < 75; ++i)
  {
    loop();
    write_frame(delay_between_frame_ms);
    increment_encoder(18, 3);
  }
  GifEnd(&g);

  return 0;
}