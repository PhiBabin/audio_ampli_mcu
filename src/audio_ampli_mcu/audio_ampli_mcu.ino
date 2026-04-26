/// Version of each external libraries (use Library Manager to install them):
/// - Raspberry Pi Pico/RP2040: 3.7.2
/// - rp2040-encoder-library: 0.1.2
/// - InputDebounce: 1.6.0
/// - MCP23S17: 0.8.0
/// - RP2040_PWM: 1.7.0
/// - IRemote: 4.4.1

#include "LCD_Driver.h"
#include "cat_sleep_img.h"
#include "digit_font.h"
#include "digit_font_droid_sans_mono.h"
#include "dm_sans_bold_62.h"
#include "dm_sans_extrabold.h"
#include "dm_sans_regular_40.h"
#include "interaction_handler.h"
#include "io_expander.h"
#include "main_menu_view.h"
#include "options_controller.h"
#include "options_view.h"
#include "persistent_data.h"
#include "state_machine.h"

#ifdef SIM
#include "sim/pio_encoder.h"
#else
#include "pio_encoder.h"
#endif

#include "config.h"
#include "remote_controller.h"

#include <algorithm>

// Convention for GPIO names:
// GPX -> X pin on the PI Pico
// GPAX -> X pin on port A of the IO expander
// GPBX -> X pin on port B of the IO expander

PioEncoder volume_encoder(pin_out::volume_encoder_b.pin);            // GP18 and GP19 are the encoder's pins
PioEncoder menu_select_encoder(pin_out::menu_select_encoder_b.pin);  // GP20 and GP21 are the encoder's pins
IoExpander io_expander_1(
  pin_out::io_expander_chip_select.pin, /*address=*/0);  // GP7 is the chip select of the IO expander
IoExpander phono_io_expander(
  pin_out::phono_io_expander_chip_select.pin);  // GP26 is the chip select of the IO expander on the phono board

#if defined(USE_V2_PCB)
// Same chip select as io expander 1, but with a different hardware address
IoExpander io_expander_2(pin_out::io_expander_chip_select.pin, /*address=*/1);

std::vector<GpioHandler::ModuleEnumExpanderPair> io_expanders{
  std::make_pair(GpioModule::io_expander_1, &io_expander_1),
  std::make_pair(GpioModule::io_expander_phono, &phono_io_expander),
  std::make_pair(GpioModule::io_expander_2, &io_expander_2)};
#else
std::vector<GpioHandler::ModuleEnumExpanderPair> io_expanders{
  std::make_pair(GpioModule::io_expander_1, &io_expander_1),
  std::make_pair(GpioModule::io_expander_phono, &phono_io_expander)};
#endif

GpioHandler gpio_handler(io_expanders);

PersistentData persistent_data{};
PersistentDataFlasher persistent_data_flasher;
StateMachine state_machine;
VolumeController volume_ctrl(
  &state_machine, &persistent_data, &volume_encoder, &gpio_handler, TOTAL_TICK_FOR_FULL_VOLUME);
OptionController option_ctrl(&state_machine, &persistent_data, &volume_ctrl, &gpio_handler);

Display display;

LvFontWrapper digit_droid_sans_font(&droid_sans_mono, true);
LvFontWrapper digit_light_font(&dmsans_36pt_light, true);
LvFontWrapper regular_bold_font(&dmsans_36pt_extrabold);
LvFontWrapper regular_medium_font(&dmsans_36pt_regular_40);
LvFontWrapper regular_large_font(&dm_sans_bold_62);

MainMenuView main_menu_view(
  &option_ctrl, &volume_ctrl, &persistent_data, &state_machine, regular_bold_font, digit_droid_sans_font);
OptionsView option_view(
  &option_ctrl,
  &volume_ctrl,
  &persistent_data,
  &state_machine,
  regular_bold_font,
  regular_medium_font,
  regular_large_font);
InteractionHandler interaction_handler(
  &option_view, &main_menu_view, &volume_ctrl, &state_machine, &menu_select_encoder);
RemoteController remote_ctrl(&state_machine, &interaction_handler, &volume_ctrl);

void draw_standby(const bool has_state_changed = true)
{
  constexpr int wait_time_between_drawing_ZZZ = 2000;
  constexpr int number_of_ZZZ = 4;
  static int timer = 0;
  static int zzz_count = 0;
  if (state_machine.get_state() != State::standby)
  {
    return;
  }
  // We just switch to standby
  if (has_state_changed)
  {
    display.clear_screen(BLACK_COLOR);
    draw_image_from_top_left(
      display, cat_sleep_image, LCD_WIDTH - cat_sleep_image.w_px - 1, LCD_HEIGHT - cat_sleep_image.h_px - 1);
    timer = millis();
    zzz_count = 1;
  }

  if (has_state_changed || millis() - timer > wait_time_between_drawing_ZZZ)
  {
    timer = millis();

    const auto& font = regular_bold_font;
    constexpr int16_t start_x = 220;
    constexpr int16_t top_y = 120;
    const auto maybe_z_glyph = font.get_glyph('Z');
    const int16_t spacing_x = maybe_z_glyph ? maybe_z_glyph.value()->width_px + font.get_spacing_px() + 2 : 20;

    if (zzz_count > number_of_ZZZ)
    {
      zzz_count = 1;
      display.draw_rectangle(
        start_x,
        top_y - 3 * number_of_ZZZ,
        start_x + number_of_ZZZ * spacing_x,
        top_y + font.get_height_px(),
        BLACK_COLOR);
    }
    for (int i = 0; i < zzz_count; ++i)
    {
      const auto height = top_y - 2 * i;
      draw_string_fast(display, "Z", start_x + i * spacing_x, height, start_x + (i + 1) * spacing_x, font);
    }
    ++zzz_count;
  }
}

void test_draw_speed()
{
  const auto N = 10;
  display.clear_screen(BLACK_COLOR);
  const auto start = millis();
  for (int i = 0; i < N; ++i)
  {
    for (uint16_t y = 0; y < LCD_HEIGHT; ++y)
    {
      for (uint16_t x = 0; x < LCD_WIDTH; ++x)
      {
        display.set_pixel(x, y, (x ^ y) % 9 == 0 ? WHITE_COLOR : BLACK_COLOR);
      }
    }

    // LCD_Clear_12bitRGB_async(i % 2 == 0 ? BLACK_COLOR : WHITE_COLOR);
    display.blip_framebuffer();
  }
  const auto end = millis();
  Serial.print("Took total: ");
  Serial.print(end - start);
  Serial.print("ms or ");
  Serial.print(static_cast<float>(end - start) / static_cast<float>(N));
  Serial.println("ms/frame");
}

void test_clear_rectangle()
{
  const auto N = 5;
  display.clear_screen(BLACK_COLOR);

  Serial.println("---------------------------------------");
  // display.draw_rectangle(10, 20, 20, 30, WHITE_COLOR);
  for (int i = 0; i < N; ++i)
  {
    Serial.println("+++++++++++++++++++++++++++++++++++++");
    int x_start = 10 + i;
    for (int width = 1; width < 10; ++width)
    {
      display.draw_rectangle(x_start, 10 + 34 * i, x_start + width, 30 + 34 * i, WHITE_COLOR);
      x_start += width + 5;
    }
  }
  display.blip_framebuffer();
  Serial.println("ms/frame");
  while (true)
  {
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting up...");

  persistent_data_flasher.init();
  if (persistent_data_flasher.maybe_load_data(persistent_data))
  {
    Serial.println("Data from flash loaded");
  }
  else
  {
    Serial.println("No data found in flash, using default settings.");
    persistent_data = PersistentData{};
    persistent_data_flasher.force_save(persistent_data);
  }

  display.gpio_init();

  // Init all IO expander
  for (const auto& [_, io_expander_ptr] : io_expanders)
  {
    io_expander_ptr->begin();
  }

  volume_encoder.begin();
  menu_select_encoder.begin();

  volume_ctrl.init();
  option_ctrl.init();
  remote_ctrl.init();
  interaction_handler.init();

  display.init();
  display.clear_screen(BLACK_COLOR);
  display.blip_framebuffer();

  main_menu_view.init();
  option_view.init();

  option_ctrl.power_on();
}

void loop()
{
  remote_ctrl.decode_command();
  interaction_handler.update();
  volume_ctrl.update();

  const auto has_state_changed = state_machine.update();
  if (has_state_changed)
  {
    Serial.println("state changed!");
    display.clear_screen(BLACK_COLOR);
  }
  switch (state_machine.get_state())
  {
    case State::main_menu:
      main_menu_view.draw(display, has_state_changed);
      break;
    case State::option_menu:
      option_view.draw(display, has_state_changed);
      break;
    case State::standby:
      draw_standby(has_state_changed);
      break;
  }

  persistent_data_flasher.save(persistent_data);
  display.blip_framebuffer();
}
