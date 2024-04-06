/// Library version:
/// Raspberry Pi Pico/RP2040: 3.7.2
/// rp2040-encoder-library: 0.1.1
/// InputDebounce: 1.6.0

#include "audio_input_controller.h"
#include "digit_font.h"
#include "dm_sans_extrabold.h"
#include "volume_controller.h"

#ifdef SIM
#include "sim/LCD_Driver.h"
#include "sim/pio_encoder.h"
#else
#include "LCD_Driver.h"
#include "pio_encoder.h"
#endif

#include <algorithm>

#define STARTUP_VOLUME_DB -42
#define ENCODER_TICK_PER_ROTATION 24
#define TOTAL_TICK_FOR_FULL_VOLUME (9 * ENCODER_TICK_PER_ROTATION)
#define TICK_PER_AUDIO_IN (ENCODER_TICK_PER_ROTATION / 4)

PioEncoder volume_encoder(18);       // GP18 and GP19 are the encoder's pins
PioEncoder menu_select_encoder(20);  // GP20 and GP21 are the encoder's pins

// 6bit output to control the volume
// const std::array<pin_size_t, 6> volume_gpio_pins = {0, 1, 2, 3, 4, 5};
const std::array<pin_size_t, 6> volume_gpio_pins = {4, 5, 6, 7, 9, 10};
const pin_size_t mute_button_pin = 16;

VolumeController volume_ctrl(
  volume_gpio_pins, &volume_encoder, mute_button_pin, STARTUP_VOLUME_DB, TOTAL_TICK_FOR_FULL_VOLUME);
AudioInputController audio_input_ctrl(&menu_select_encoder, AudioInput::AUX_3, TICK_PER_AUDIO_IN);

LvFontWrapper digit_light_font(&dmsans_36pt_light);
LvFontWrapper regular_bold_font(&dmsans_36pt_extrabold);

void draw_volume()
{
  char buffer[5];
  // sprintf(buffer, "%sVolume: %ddB  Audio input: %s", volume_ctrl.is_muted() ? "[MUTED]" : "",
  // volume_ctrl.get_volume_db(), audio_input_to_string(audio_input_ctrl.get_audio_input()));
  sprintf(buffer, "%d", volume_ctrl.get_volume_db());
  draw_string_fast(buffer, 95, 64, 95 + 222, digit_light_font);
}

void draw_audio_inputs()
{
  const auto& font = regular_bold_font;
  const auto max_enum_value = static_cast<uint8_t>(AudioInput::audio_input_enum_length);
  const uint32_t ver_spacing =
    LCD_HEIGHT / (max_enum_value + 1);  // + 1 is to have equal spacing between the top input and the screen's border

  const uint32_t tab_width_px = 80;
  const uint32_t tab_height_px = font.get_height_px() + 8;
  const uint32_t first_option_center_y = ver_spacing;

  LCD_ClearWindow_12bitRGB(0, 0, tab_width_px + 1, LCD_HEIGHT, BLACK_COLOR);

  for (uint8_t enum_value = 0; enum_value < max_enum_value; ++enum_value)
  {
    const auto audio_input = static_cast<AudioInput>(enum_value);
    const auto curr_option_center_y = first_option_center_y + enum_value * ver_spacing;
    const auto curr_option_box_top_y = curr_option_center_y - tab_height_px / 2;
    const auto curr_option_text_top_y = curr_option_center_y - font.get_height_px() / 2;

    const auto is_selected = audio_input_ctrl.get_audio_input() == audio_input;
    if (is_selected)
    {
      LCD_ClearWindow_12bitRGB(
        0, curr_option_box_top_y, tab_width_px + 1, curr_option_box_top_y + tab_height_px + 1, WHITE_COLOR);
    }
    draw_string_fast(
      audio_input_to_string(audio_input), 0, curr_option_text_top_y, tab_width_px, font, !is_selected, false);
  }
}

void setup()
{
  Serial.begin(115200);
  // while(!Serial);
  Serial.println("Starting up...");

  volume_encoder.begin();
  menu_select_encoder.begin();

  volume_ctrl.init();
  audio_input_ctrl.init();

  Config_Init();
  LCD_Init();
  LCD_Clear_12bitRGB(0x0000);

  draw_volume();
  draw_audio_inputs();
}

void loop()
{
  bool has_changed = volume_ctrl.update();
  if (has_changed)
  {
    if (volume_ctrl.is_muted())
    {
      Serial.print("[MUTED]");
    }
    Serial.print("Volume %: ");
    Serial.println(volume_ctrl.get_volume_db());

    // char buffer[100];
    // sprintf(buffer, "%sVolume: %ddB  Audio input: %s", volume_ctrl.is_muted() ? "[MUTED]" : "",
    // volume_ctrl.get_volume_db(), audio_input_to_string(audio_input_ctrl.get_audio_input()));
    draw_volume();
  }
  const auto audio_input_change = audio_input_ctrl.update();
  has_changed |= audio_input_change;
  if (audio_input_change)
  {
    Serial.println(audio_input_to_string(audio_input_ctrl.get_audio_input()));
    draw_audio_inputs();
  }
  if (has_changed)
  {
    // LCD_Clear(0xffff);
    // Paint_Clear(WHITE);
  }
}
