/// Version of each external libraries (use Library Manager to install them):
/// - Raspberry Pi Pico/RP2040: 3.7.2
/// - rp2040-encoder-library: 0.1.2
/// - InputDebounce: 1.6.0
/// - MCP23S17: 0.5.1
/// - RP2040_PWM: 1.7.0
/// - IRemote: 4.4.1

#include "LCD_Driver.h"
#include "cat_sleep_img.h"
#include "digit_font.h"
#include "digit_font_droid_sans_mono.h"
#include "dm_sans_extrabold.h"
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

PioEncoder volume_encoder(18);            // GP18 and GP19 are the encoder's pins
PioEncoder menu_select_encoder(20);       // GP20 and GP21 are the encoder's pins
const pin_size_t mute_button_pin = 16;    // Button for the volume encoder
const pin_size_t select_button_pin = 17;  // Button for the menu select encoder
IoExpander io_expander(7);                // GP7 is the chip select of the IO expander
const pin_size_t set_mute_pin = 27;       // Output pin that mute / unmute
const pin_size_t bias_pwm_pin = 27;       // Output pin that mute / unmute
const pin_size_t power_enable_pin = 14;   // Output pin that power on / off the amplification part of the system
const pin_size_t latch_left_vol = 28;     // Apply volume to the left side
const pin_size_t latch_right_vol = 15;    // Apply volume to the right side

// 6bit output to control the volume
const std::array<pin_size_t, 6> volume_gpio_pins = {22, 4, 5, 9, 10, 11};
// IO expander pins for the audio input selection. BAL, RCA 1, RCA 2 and RCA 3   respectively
OptionContollerPins option_ctrl_pins{
  .iox_gpio_pin_audio_in_select = {0, 1, 2, 3},  // GPA0, GPA1, GPA2 & GPA3
  .in_out_unipolar_pin = 4,                      // GPA4
  .in_out_bal_unipolar_pin = 5,                  // GPA5
  .set_low_gain_pin = 8,                         // GPB0
  .out_bal_pin = 9,                              // GPB1
  .preamp_out_pin = 10,                          // GPB2
  .out_se_pin = 11,                              // GPB3
  .out_lfe_bal_pin = 12,                         // GPB4
  .out_lfe_se_pin = 13,                          // GPB5
  .trigger_12v = 15,                             // GPB7
};

PersistentData persistent_data{};
PersistentDataFlasher persistent_data_flasher;
StateMachine state_machine;
VolumeController volume_ctrl(
  &state_machine,
  &persistent_data,
  volume_gpio_pins,
  &volume_encoder,
  latch_left_vol,
  latch_right_vol,
  TOTAL_TICK_FOR_FULL_VOLUME);
OptionController option_ctrl(
  &state_machine, &persistent_data, &io_expander, &volume_ctrl, bias_pwm_pin, power_enable_pin, option_ctrl_pins);

Display display;

LvFontWrapper digit_droid_sans_font(&droid_sans_mono, true);
LvFontWrapper digit_light_font(&dmsans_36pt_light, true);
LvFontWrapper regular_bold_font(&dmsans_36pt_extrabold);

MainMenuView main_menu_view(
  &option_ctrl, &volume_ctrl, &persistent_data, &state_machine, regular_bold_font, digit_droid_sans_font);
OptionsView option_view(&option_ctrl, &volume_ctrl, &persistent_data, &state_machine, regular_bold_font);
InteractionHandler interaction_handler(
  &option_view,
  &main_menu_view,
  &volume_ctrl,
  &state_machine,
  &menu_select_encoder,
  select_button_pin,
  mute_button_pin);
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
        start_x, top_y, start_x + number_of_ZZZ * spacing_x, top_y + font.get_height_px(), BLACK_COLOR);
    }
    for (int i = 0; i < zzz_count; ++i)
    {
      draw_string_fast(display, "Z", start_x + i * spacing_x, top_y, start_x + (i + 1) * spacing_x, font);
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
  io_expander.begin();

  volume_encoder.begin();
  menu_select_encoder.begin();

  volume_ctrl.init();
  option_ctrl.init();
  remote_ctrl.init();
  interaction_handler.init();

  display.init();
  display.clear_screen(BLACK_COLOR);
  display.blip_framebuffer();

  option_ctrl.power_on();
}

void loop()
{
  const auto remote_change = remote_ctrl.decode_command();
  const auto option_change = interaction_handler.update();
  const bool has_volume_changed = volume_ctrl.update();

  const auto state_changed = state_machine.update();
  if (state_changed)
  {
    display.clear_screen(BLACK_COLOR);
  }
  switch (state_machine.get_state())
  {
    case State::main_menu:
      main_menu_view.draw(display);
      break;
    case State::option_menu:
      option_view.draw(display);
      break;
    case State::standby:
      draw_standby(state_changed);
      break;
  }

  // if (option_change || remote_change)
  // {
  //   Serial.println("update options");
  //   draw_options();
  //   volume_ctrl.on_option_change();
  // }
  // if (state_changed && state_machine.get_state() == State::main_menu)
  // {
  //   display.clear_screen(BLACK_COLOR);
  // }
  // draw_standby(state_changed);
  // const auto audio_input_change = false;  // audio_input_ctrl.update();
  // if (audio_input_change || remote_change)
  // {
  //   option_ctrl.on_audio_input_change();
  //   volume_ctrl.on_audio_input_change();
  // }
  // // if (audio_input_change || state_changed || remote_change)
  // // {
  // // Serial.println("update audio input");
  // draw_audio_inputs(state_changed);
  // // }
  // bool has_changed = volume_ctrl.update();
  // if (has_changed || state_changed || audio_input_change || option_change || remote_change)
  // {
  //   Serial.println("update volume");
  //   draw_volume(state_changed);
  // }

  persistent_data_flasher.save(persistent_data);
  display.blip_framebuffer();
}
