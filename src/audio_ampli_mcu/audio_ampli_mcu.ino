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
#include "consolas_regular_26.h"
#include "dm_sans_regular_40.h"
#include "interaction_handler.h"
#include "io_expander.h"
#include "main_menu_view.h"
#include "options_controller.h"
#include "options_view.h"
#include "persistent_data.h"
#include "standby_view.h"
#include "state_machine.h"
#include <optional>
#include <ratio>

#ifdef SIM
#include "sim/pio_encoder.h"
#else
#include "pio_encoder.h"
#endif

#include "config.h"
#include "remote_controller.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

// Convention for GPIO names:
// GPX -> X pin on the PI Pico
// GPAX -> X pin on port A of the IO expander
// GPBX -> X pin on port B of the IO expander

PioEncoder volume_encoder(pin_out::volume_encoder_b.pin);            // GP18 and GP19 are the encoder's pins
PioEncoder menu_select_encoder(pin_out::menu_select_encoder_b.pin);  // GP20 and GP21 are the encoder's pins
IoExpander io_expander_1(
  pin_out::io_expander_chip_select.pin);  // GP7 is the chip select of the IO expander address == 0x0
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
  &state_machine, &persistent_data, &volume_encoder, &gpio_handler);
OptionController option_ctrl(&state_machine, &persistent_data, &volume_ctrl, &gpio_handler);

Display display;

LvFontWrapper digit_droid_sans_font(&droid_sans_mono, true);
LvFontWrapper digit_light_font(&dmsans_36pt_light, true);
LvFontWrapper regular_bold_font(&dmsans_36pt_extrabold);//&dmsans_36pt_extrabold);
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
StandbyView standby_view(&state_machine, &display, &regular_bold_font, &cat_sleep_image);

InteractionHandler interaction_handler(
  &option_view, &main_menu_view, &volume_ctrl, &state_machine, &menu_select_encoder);
RemoteController remote_ctrl(&state_machine, &interaction_handler, &volume_ctrl);


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

/// Animated test that exercises the new Rect-based clipping.
/// Shapes and text are deliberately driven off all four screen edges
/// to prove that set_pixel_unsafe is never called with out-of-bounds coordinates.
void test_bounds_check()
{
  static uint32_t frame = 0;
  ++frame;

  const float t = frame * 0.02f;
  display.clear_screen(BLACK_COLOR);

  // --- 1. Image: wide ellipse, goes off ALL edges ---
  {
    const int32_t cx = static_cast<int32_t>(sinf(t) * (LCD_WIDTH * 0.65f));
    const int32_t cy = static_cast<int32_t>(cosf(t * 1.3f) * (LCD_HEIGHT * 0.65f));
    const int32_t img_x = LCD_WIDTH / 2 + cx - static_cast<int32_t>(cat_sleep_image.w_px / 2);
    const int32_t img_y = LCD_HEIGHT / 2 + cy - static_cast<int32_t>(cat_sleep_image.h_px / 2);
    draw_image_from_top_left(display, cat_sleep_image, img_x, img_y);
  }

  // --- 2. Rounded rectangle: diagonal sweep, goes off ALL edges ---
  {
    const int32_t rr_w = 110;
    const int32_t rr_h = 70;
    const int32_t rr_x = static_cast<int32_t>((frame * 2) % (LCD_WIDTH + rr_w + 40)) - (rr_w + 20);
    const int32_t rr_y = static_cast<int32_t>((frame * 3) % (LCD_HEIGHT + rr_h + 40)) - (rr_h + 20);
    draw_rounded_rectangle(display, rr_x, rr_y, rr_x + rr_w, rr_y + rr_h, true, true, true, 14);
  }

  // --- 3. Text: horizontal scroll, goes off right edge ---
  {
    const int32_t txt_x = static_cast<int32_t>((frame * 2) % (LCD_WIDTH + 120)) - 60;
    draw_string_fast(display, "RIGHT-CLIP", txt_x, 10, txt_x + 260, regular_bold_font, true, false, TextAlign::left);
  }

  // --- 4. Text: vertical scroll, goes off top & bottom ---
  {
    const int32_t txt_y = static_cast<int32_t>((frame * 3) % (LCD_HEIGHT + 80)) - 40;
    draw_string_fast(display, "BOTTOM", 10, txt_y, 200, regular_medium_font, true, false, TextAlign::left);
  }

  // --- 5. Centered text: oscillates past left & right edges ---
  {
    const int32_t offset = static_cast<int32_t>(sinf(t * 0.7f) * (LCD_WIDTH * 0.55f));
    const int32_t box_cx = LCD_WIDTH / 2 + offset;
    draw_string_fast(display, "CENTER", box_cx, LCD_HEIGHT / 2 - 20, box_cx + 160, regular_bold_font, true, false, TextAlign::center);
  }

  // --- 6. Small frame counter (always visible) ---
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", frame);
    draw_string_fast(display, buf, 5, LCD_HEIGHT - 35, 120, regular_medium_font, true, false, TextAlign::left);
  }

  display.blip_framebuffer();
  // delay(16);  // ~60 fps
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

  gpio_handler.cache_init_input(pin_out::power_detect);
}

void update_low_power_timer()
{
  if (state_machine.get_state() != State::main_menu)
  {
    return;
  }
  static std::optional<unsigned long> maybe_timer;
  static bool prev_power_detected = true;

  const bool is_power_detected = digitalRead(pin_out::power_detect.pin) == HIGH;

  // Detected -> not detected
  if (!is_power_detected && prev_power_detected)
  {
    Serial.println("Starting inactive timer...");
    maybe_timer.emplace(millis());
  } // not detected -> not detected
  else if (!is_power_detected && !prev_power_detected)
  {
    if (maybe_timer.has_value() && (millis() - maybe_timer.value()) > INACTIVITY_TIMER_THRESHOLD_MS)
    {
      Serial.println("Turning off...");
      option_view.power_off();
      state_machine.change_state(State::standby);
      maybe_timer = std::nullopt;
    }
  } // not detected -> detected
  else if (is_power_detected && !prev_power_detected)
  {
    Serial.println("End inactive timer...");
    maybe_timer = std::nullopt;
  } // detected -> detected
  else
  {
  }

  prev_power_detected = is_power_detected;
}

void loop()
{
  // test_bounds_check();
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
      standby_view.draw(has_state_changed);
      break;
  }

  update_low_power_timer();

  persistent_data_flasher.save(persistent_data);
  display.blip_framebuffer();
}
