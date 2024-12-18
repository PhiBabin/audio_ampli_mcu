/// Version of each external libraries (use Library Manager to install them):
/// - Raspberry Pi Pico/RP2040: 3.7.2
/// - rp2040-encoder-library: 0.1.2
/// - InputDebounce: 1.6.0
/// - MCP23S17: 0.5.1
/// - RP2040_PWM: 1.7.0
/// - IRemote: 4.4.1

#include "audio_input_controller.h"
#include "cat_sleep_img.h"
#include "digit_font.h"
#include "digit_font_droid_sans_mono.h"
#include "digit_font_lt_superior_mono.h"
#include "dm_sans_extrabold.h"
#include "io_expander.h"
#include "mute_image.h"
#include "options_controller.h"
#include "persistent_data.h"
#include "state_machine.h"

#ifdef SIM
#include "sim/LCD_Driver.h"
#include "sim/pio_encoder.h"
#else
#include "LCD_Driver.h"
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

// 6bit output to control the volume
const std::array<pin_size_t, 6> volume_gpio_pins = {22, 4, 5, 9, 10, 11};
// IO expander pins for the audio input selection. BAL, RCA 1, RCA 2 and RCA 3   respectively
const std::array<pin_size_t, 4> audio_input_iox_gpio_pins = {0, 1, 2, 3};  // GPA0, GPA1, GPA2 & GPA3

// Option pins
const int in_out_unipolar_pin = 4;      // GPA4
const int in_out_bal_unipolar_pin = 5;  // GPA5
const int set_low_gain_pin = 8;         // GPB0
const int out_bal_pin = 9;              // GPB1
const int preamp_out_pin = 10;          // GPB2

PersistentData persistent_data{};
PersistentDataFlasher persistent_data_flasher;
StateMachine state_machine;
VolumeController volume_ctrl(
  &state_machine,
  &persistent_data,
  volume_gpio_pins,
  &volume_encoder,
  mute_button_pin,
  set_mute_pin,
  TOTAL_TICK_FOR_FULL_VOLUME);
AudioInputController audio_input_ctrl(
  &state_machine, &persistent_data, &menu_select_encoder, &io_expander, audio_input_iox_gpio_pins, TICK_PER_AUDIO_IN);
OptionController option_ctrl(
  &state_machine,
  &persistent_data,
  &menu_select_encoder,
  &io_expander,
  &volume_ctrl,
  select_button_pin,
  TICK_PER_AUDIO_IN,
  in_out_unipolar_pin,
  in_out_bal_unipolar_pin,
  set_low_gain_pin,
  out_bal_pin,
  preamp_out_pin,
  bias_pwm_pin);
RemoteController remote_ctrl(&state_machine, &option_ctrl, &audio_input_ctrl, &volume_ctrl);

LvFontWrapper digit_lt_superior_font(&lt_superior_mono, true);
LvFontWrapper digit_droid_sans_font(&droid_sans_mono, true);
LvFontWrapper digit_light_font(&dmsans_36pt_light, true);
LvFontWrapper regular_bold_font(&dmsans_36pt_extrabold);

void draw_volume(const bool has_state_changed = true)
{
  if (state_machine.get_state() == State::standby)
  {
    return;
  }
  const int max_time_since_last_change = 5000;
  static auto prev_volume = volume_ctrl.get_volume_db();
  static int time_since_last_change = -max_time_since_last_change;
  // static auto prev_state = state_machine.get_state();
  if (volume_ctrl.get_volume_db() != prev_volume)
  {
    time_since_last_change = millis();
    prev_volume = volume_ctrl.get_volume_db();
  }

  if (state_machine.get_state() == State::option_menu)
  {
    const uint32_t y_end = 50;
    const uint32_t y_text_top = (y_end - regular_bold_font.get_height_px()) / 2;
    if (has_state_changed)
    {
      LCD_ClearWindow_12bitRGB(0, 0, LCD_WIDTH, y_end, BLACK_COLOR);
    }
    // prev_state = state_machine.get_state();
    if (millis() - time_since_last_change >= max_time_since_last_change)
    {
      return;
    }

    char option_buffer[10];
    if (volume_ctrl.is_muted())
    {
      strcpy(option_buffer, "[MUTED]");
    }
    else
    {
      sprintf(option_buffer, "Vol: %ddB", volume_ctrl.get_volume_db());
    }
    draw_string_fast(option_buffer, 0, y_text_top, LCD_WIDTH, regular_bold_font);
    return;
  }
  static bool prev_mute_state = volume_ctrl.is_muted();

  const auto& font = digit_droid_sans_font;
  char buffer[5];
  sprintf(buffer, "%d", volume_ctrl.get_volume_db());

  const uint32_t min_x = 85;
  const uint32_t max_x = LCD_WIDTH - 10;
  const uint32_t middle_y = LCD_HEIGHT / 2;
  const uint32_t start_y = middle_y - font.get_height_px() / 2;

  if (prev_mute_state != volume_ctrl.is_muted())
  {
    LCD_ClearWindow_12bitRGB(min_x, 0, max_x, LCD_HEIGHT, BLACK_COLOR);
    prev_mute_state = volume_ctrl.is_muted();
  }

  if (volume_ctrl.is_muted())
  {
    draw_image(mute_image, (max_x - min_x) / 2 + min_x + 3, middle_y);
  }
  else
  {
    draw_string_fast(buffer, min_x, start_y, max_x, font);
  }
  // prev_state = state_machine.get_state();
}

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
    LCD_Clear_12bitRGB(BLACK_COLOR);
    draw_image_from_top_left(cat_sleep_image, LCD_WIDTH - cat_sleep_image.w_px - 1,  LCD_HEIGHT - cat_sleep_image.h_px -1);
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
      LCD_ClearWindow_12bitRGB(
        start_x, top_y, start_x + number_of_ZZZ * spacing_x, top_y + font.get_height_px(), BLACK_COLOR);
    }
    for (int i = 0; i < zzz_count; ++i)
    {
      draw_string_fast("Z", start_x + i * spacing_x, top_y, start_x + (i + 1) * spacing_x, font);
    }
    ++zzz_count;
  }
}

void draw_audio_inputs(const bool has_state_changed = true)
{
  static auto prev_audio_input = audio_input_ctrl.get_audio_input();
  if (state_machine.get_state() != State::main_menu)
  {
    return;
  }
  const auto& font = regular_bold_font;
  const auto max_enum_value = static_cast<uint8_t>(AudioInput::audio_input_enum_length);
  const uint32_t ver_spacing =
    LCD_HEIGHT / (max_enum_value + 1);  // + 1 is to have equal spacing between the top input and the screen's border

  const uint32_t tab_width_px = 82;
  const uint32_t tab_height_px = font.get_height_px() + 8;
  const uint32_t first_option_center_y = ver_spacing;

  if (has_state_changed)
  {
    LCD_ClearWindow_12bitRGB(0, 0, tab_width_px + 1, LCD_HEIGHT, BLACK_COLOR);
  }
  else if (prev_audio_input == audio_input_ctrl.get_audio_input())
  {
    prev_audio_input = audio_input_ctrl.get_audio_input();
    return;
  }

  for (uint8_t enum_value = 0; enum_value < max_enum_value; ++enum_value)
  {
    const auto audio_input = static_cast<AudioInput>(enum_value);
    const auto curr_option_center_y = first_option_center_y + enum_value * ver_spacing;
    const auto curr_option_box_top_y = curr_option_center_y - tab_height_px / 2;
    const auto curr_option_text_top_y = curr_option_center_y - font.get_height_px() / 2;

    const auto is_selected = audio_input_ctrl.get_audio_input() == audio_input;
    const auto was_previously_selected = prev_audio_input == audio_input;
    if (is_selected)
    {
      draw_rounded_rectangle(
        0,
        curr_option_box_top_y,
        tab_width_px,
        curr_option_box_top_y + tab_height_px + 1,
        /*is_white_on_black=*/true,
        /*rounded_left =*/false,
        /*rounded_right =*/true);
    }
    else if (was_previously_selected)
    {
      LCD_ClearWindow_12bitRGB(
        0, curr_option_box_top_y, tab_width_px, curr_option_box_top_y + tab_height_px + 1, BLACK_COLOR);
    }
    if (is_selected || was_previously_selected || has_state_changed)
    {
      draw_string_fast(
        audio_input_to_string(audio_input), 0, curr_option_text_top_y, tab_width_px, font, !is_selected, false);
    }
  }
  prev_audio_input = audio_input_ctrl.get_audio_input();
}

void draw_options()
{
  static auto prev_state = state_machine.get_state();
  static auto prev_selected_option = option_ctrl.get_selected_option();
  const auto& font = regular_bold_font;
  if (state_machine.get_state() != State::option_menu)
  {
    prev_state = state_machine.get_state();
    prev_selected_option = option_ctrl.get_selected_option();
    return;
  }

  const auto max_enum_value = static_cast<uint8_t>(Option::option_enum_length);
  const uint32_t ver_spacing = LCD_HEIGHT / (max_enum_value + 2);

  if (prev_state != state_machine.get_state())
  {
    LCD_ClearWindow_12bitRGB(0, 0, LCD_WIDTH, LCD_HEIGHT, BLACK_COLOR);
  }

  for (uint8_t enum_value = 0; enum_value < max_enum_value; ++enum_value)
  {
    const auto option = static_cast<Option>(enum_value);
    const auto curr_option_center_y = (enum_value + 2) * ver_spacing;
    const auto curr_option_text_top_y = curr_option_center_y - font.get_height_px() / 2;

    const auto is_selected = option_ctrl.get_selected_option() == option;
    const auto is_prev_selected_option = option == prev_selected_option;

    if (is_selected || is_prev_selected_option)
    {
      const auto curr_option_box_top_y = curr_option_center_y - ver_spacing / 2;
      LCD_ClearWindow_12bitRGB(
        0,
        curr_option_box_top_y,
        LCD_WIDTH,
        curr_option_box_top_y + ver_spacing + 1,
        is_selected ? WHITE_COLOR : BLACK_COLOR);
    }

    // Back is centered
    if (option == Option::back)
    {
      draw_string_fast("BACK", 0, curr_option_text_top_y, LCD_WIDTH, font, !is_selected, false);
    }  // Other option have a field name and values
    else
    {
      draw_string_fast(option_to_string(option), 0, curr_option_text_top_y, LCD_WIDTH / 2, font, !is_selected, false);
      draw_string_fast(
        option_ctrl.get_option_value_string(option),
        LCD_WIDTH / 2,
        curr_option_text_top_y,
        LCD_WIDTH,
        font,
        !is_selected,
        false);
    }
  }
  prev_state = state_machine.get_state();
  prev_selected_option = option_ctrl.get_selected_option();
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

  LCD_GPIO_Init();
  io_expander.begin();

  volume_encoder.begin();
  menu_select_encoder.begin();

  volume_ctrl.init();
  audio_input_ctrl.init();
  option_ctrl.init();
  remote_ctrl.init();

  LCD_Init();
  LCD_Clear_12bitRGB(BLACK_COLOR);

  draw_options();
  draw_audio_inputs();
  draw_volume();
}

void loop()
{
  const auto remote_change = remote_ctrl.decode_command();
  const auto option_change = option_ctrl.update();
  if (option_change || remote_change)
  {
    Serial.println("update options");
    draw_options();
    volume_ctrl.on_option_change();
  }
  const auto state_changed = state_machine.update();
  if (state_changed && state_machine.get_state() == State::main_menu)
  {
    LCD_Clear_12bitRGB(BLACK_COLOR);
  }
  draw_standby(state_changed);
  const auto audio_input_change = audio_input_ctrl.update();
  if (audio_input_change || remote_change)
  {
    option_ctrl.on_audio_input_change();
    volume_ctrl.on_audio_input_change();
  }
  if (audio_input_change || state_changed || remote_change)
  {
    Serial.println("update audio input");
    draw_audio_inputs(state_changed);
  }
  bool has_changed = volume_ctrl.update();
  if (has_changed || state_changed || audio_input_change || option_change || remote_change)
  {
    Serial.println("update volume");
    draw_volume(state_changed);
  }

  persistent_data_flasher.save(persistent_data);
}
