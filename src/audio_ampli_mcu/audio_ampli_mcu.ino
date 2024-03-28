/// Library version:
/// Raspberry Pi Pico/RP2040: 3.7.2
/// rp2040-encoder-library: 0.1.1
/// InputDebounce: 1.6.0

#include "pio_encoder.h"
#include "volume_controller.h"
#include "audio_input_controller.h"

#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

#include <SPI.h>

#define STARTUP_VOLUME_PERCENTAGE 50
#define ENCODER_TICK_PER_ROTATION 24
#define TOTAL_TICK_FOR_FULL_VOLUME (3 * ENCODER_TICK_PER_ROTATION)
#define TICK_PER_AUDIO_IN (ENCODER_TICK_PER_ROTATION / 4)

PioEncoder volume_encoder(18); // GP18 and GP19 are the encoder's pins
PioEncoder menu_select_encoder(20);  // GP20 and GP21 are the encoder's pins

// 6bit output to control the volume
// const std::array<pin_size_t, 6> volume_gpio_pins = {0, 1, 2, 3, 4, 5};
const std::array<pin_size_t, 6> volume_gpio_pins = {4, 5, 6, 7, 9, 10};
const pin_size_t mute_button_pin = 16;

VolumeController volume_ctrl(volume_gpio_pins, &volume_encoder, mute_button_pin, STARTUP_VOLUME_PERCENTAGE, TOTAL_TICK_FOR_FULL_VOLUME);
AudioInputController audio_input_ctrl(&menu_select_encoder, AudioInput::AUX_3, TICK_PER_AUDIO_IN);


void setup()
{
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Starting up...");

  volume_encoder.begin();
  menu_select_encoder.begin();

  volume_ctrl.init();
  audio_input_ctrl.init();

  // SPI.setRX(0);
  // SPI.setCS(1);
  // SPI.setSCK(2);
  // SPI.setTX(3);
  // SPI.begin(false);
  // SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3));
  Config_Init();
  LCD_Init();
  LCD_Clear(0xffff);
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, WHITE);
  
  // Paint_DrawRectangle(125, 10, 225, 58, RED,  DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
  // Paint_DrawLine(125, 10, 225, 58, MAGENTA,   DOT_PIXEL_2X2,LINE_STYLE_SOLID);
  // Paint_DrawLine(225, 10, 125, 58, MAGENTA,   DOT_PIXEL_2X2,LINE_STYLE_SOLID);
  
  // Paint_DrawCircle(150,100, 25, BLUE,   DOT_PIXEL_2X2,   DRAW_FILL_EMPTY);
  // Paint_DrawCircle(180,100, 25, BLACK,  DOT_PIXEL_2X2,   DRAW_FILL_EMPTY);
  // Paint_DrawCircle(210,100, 25, RED,    DOT_PIXEL_2X2,   DRAW_FILL_EMPTY);
  // Paint_DrawCircle(165,125, 25, YELLOW, DOT_PIXEL_2X2,   DRAW_FILL_EMPTY);
  // Paint_DrawCircle(195,125, 25, GREEN,  DOT_PIXEL_2X2,   DRAW_FILL_EMPTY);
  

  // Paint_DrawImage(gImage_70X70, 20, 80, 70, 70); 
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
    Serial.println(volume_ctrl.get_volume_percentage());
  }
  const auto audio_input_change = audio_input_ctrl.update();
  has_changed |= audio_input_change;
  if (audio_input_change)
  {
    Serial.println(audio_input_to_string(audio_input_ctrl.get_audio_input()));
  }
  if (has_changed)
  {
    
  //  Serial.println("test");
    LCD_Clear(0xffff);
    Paint_Clear(WHITE);

    char buffer[100];
    // sprintf(buffer, "Volume 1123123213123123123123213123123");
    sprintf(buffer, "%sVolume: %d%  Audio input: %s", volume_ctrl.is_muted() ? "[MUTED]" : "", volume_ctrl.get_volume_percentage(), audio_input_to_string(audio_input_ctrl.get_audio_input()));     
    Paint_DrawString_EN(30, 34, buffer, &Font24, BLUE, CYAN);
  }
}

