/// Library version:
/// Raspberry Pi Pico/RP2040: 3.7.2
/// rp2040-encoder-library: 0.1.1
/// InputDebounce: 1.6.0

#include "pio_encoder.h"
#include "volume_controller.h"
#include "audio_input_controller.h"
#include "digit_font.h"

#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

#include <SPI.h>

#define STARTUP_VOLUME_DB 42
#define ENCODER_TICK_PER_ROTATION 24
#define TOTAL_TICK_FOR_FULL_VOLUME (3 * ENCODER_TICK_PER_ROTATION)
#define TICK_PER_AUDIO_IN (ENCODER_TICK_PER_ROTATION / 4)

PioEncoder volume_encoder(18); // GP18 and GP19 are the encoder's pins
PioEncoder menu_select_encoder(20);  // GP20 and GP21 are the encoder's pins

// 6bit output to control the volume
// const std::array<pin_size_t, 6> volume_gpio_pins = {0, 1, 2, 3, 4, 5};
const std::array<pin_size_t, 6> volume_gpio_pins = {4, 5, 6, 7, 9, 10};
const pin_size_t mute_button_pin = 16;

VolumeController volume_ctrl(volume_gpio_pins, &volume_encoder, mute_button_pin, STARTUP_VOLUME_DB, TOTAL_TICK_FOR_FULL_VOLUME);
AudioInputController audio_input_ctrl(&menu_select_encoder, AudioInput::AUX_3, TICK_PER_AUDIO_IN);

const uint8_t BLANK_COLOR = 0;
const int32_t BLANK_DIGIT = 11;

struct Digit
{
  const uint32_t max_width{64};
  const uint32_t max_height{128};
  bool enabled{false};
  int32_t digit{0};
  uint32_t start_x{0};
  uint32_t start_y{0};
  const uint8_t* start_digit_bitmap_ptr{nullptr};
  uint32_t digit_width{64};

  void set_digit(int32_t digit_)
  {
    digit = digit_;
    /// Find the offset in the bitmap of where the digit started
    start_digit_bitmap_ptr = dmsans_80pt_thin_glyph_bitmap;
    for (size_t i = 0; i < digit_; ++i)
    {
      const auto width = dmsans_80pt_thin_width_px[i];
      const auto actual_width = width % 2 == 0 ? width : width + 1;
      start_digit_bitmap_ptr += (actual_width * max_height) * 8 / 4;
    }
    digit_width = dmsans_80pt_thin_width_px[digit_];
  }

  bool is_within_boundary(const uint32_t x, const uint32_t y) const
  {
    return start_x <= x && x < start_x + max_width &&  start_y <= y && y < start_y + max_height;
  }

  uint32_t get_color(const uint32_t x, const uint32_t y) const
  {
    if (digit == BLANK_DIGIT)
    {
      return BLANK_COLOR;
    }
    if (start_digit_bitmap_ptr == nullptr)
    {
      return BLANK_COLOR;
    }
    if (x - start_x < (max_width - digit_width) || x - start_x >= max_width)
    {
      return BLANK_COLOR;
    }
    const uint32_t bitmap_x_px = x - start_x - (max_width - digit_width);
    const uint32_t bitmap_y_px =  y - start_y;
    const uint32_t bitmap_y_offset_px =  digit_width % 2 == 0 ? digit_width * bitmap_y_px : (digit_width + 1) * bitmap_y_px; // Padding for odd width
    const uint32_t offset_px = bitmap_y_offset_px + bitmap_x_px;
    const auto two_pixels_byte = start_digit_bitmap_ptr[offset_px  * 8 / 4];
    if (offset_px % 2 == 0)
    {
      return two_pixels_byte >> 4;
    }
    return two_pixels_byte & 0x0F;
  }
};

void send_2pixel_color(const uint32_t color_2pixels)
{
	DEV_Digital_Write(DEV_CS_PIN, 0);
	DEV_Digital_Write(DEV_DC_PIN, 1);
	DEV_SPI_WRITE((color_2pixels >> 16) & 0xff);
	DEV_SPI_WRITE((color_2pixels >> 8) & 0xff);
	DEV_SPI_WRITE(color_2pixels & 0xff);
	DEV_Digital_Write(DEV_CS_PIN, 1);
}

void fast_draw_digits(const int32_t maybe_first_digit, const int32_t maybe_second_digit)
{
  Digit digits[2] = {Digit{}, Digit{}};
  uint32_t start_x = 0;
  uint32_t start_y = 0;
  uint32_t end_x = 0;
  uint32_t end_y = 0;
  if (maybe_first_digit >= 0)
  {
    digits[0].set_digit(maybe_first_digit == 0 ? BLANK_DIGIT : maybe_first_digit);
    digits[0].enabled = true;
    digits[0].start_x = 68;
    digits[0].start_y = 38;
    start_x = digits[0].start_x;
  }
  if (maybe_second_digit >= 0)
  {
    digits[1].set_digit(maybe_second_digit);
    digits[1].enabled = true;
    digits[1].start_x = 172;
    digits[1].start_y = 38;
    start_x = min(digits[1].start_x, start_x);
  }

  // Make sure that we have an even number of columns, that way we don't have to worry about write call with only one column
  if (end_x - start_x % 2 != 0)
  {
    ++end_x;
  }

  // LCD will auto increment the row when we reach columns == end_x
	LCD_SetWindow(start_x, start_y, end_x, end_y);
  for (int32_t y = start_y; y < end_y; ++y)
  {
    uint8_t px_count = 0;
    uint32_t color_2pixels = 0;
    for (int32_t x = start_x; x < end_x; ++x)
    {
      uint8_t color_4bit = BLANK_COLOR;
      for (const auto& digit : digits)
      {
        if (digit.enabled && digit.is_within_boundary(x, y))
        {
          color_4bit = digit.get_color(x, y);
        }
      }
      // Convert 4bit grayscale to four 4bit RGB
      color_2pixels = (color_2pixels << 12) | (color_4bit << 8 | color_4bit << 4 | color_4bit);
      ++px_count;
      if (px_count == 2)
      {
        px_count = 0;
        color_2pixels = 0;
        send_2pixel_color(color_2pixels);
      }
    }
  }
}

void draw_volume()
{
  static const auto prev_is_muted = volume_ctrl.is_muted();
  static const auto prev_volume_db = volume_ctrl.get_volume_db();
  const auto curr_is_muted = volume_ctrl.is_muted();
  const auto curr_volume_db = volume_ctrl.get_volume_db();
  if (curr_volume_db == prev_volume_db  && curr_is_muted == prev_is_muted)
  {
    return;
  }
  const auto first_digit = curr_volume_db / 10;
  const auto second_digit = curr_volume_db % 10;
  const auto first_digit_changed = first_digit != (prev_volume_db / 10);
  const auto second_digit_changed = second_digit != (prev_volume_db % 10);

  fast_draw_digits(first_digit_changed ? first_digit : -1, second_digit_changed ? second_digit : -1);

}

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
  LCD_Clear_12bitRGB(0x0000);
  // Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, BLACK);
  
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
    Serial.println(volume_ctrl.get_volume_db());
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
    // LCD_Clear(0xffff);
    // Paint_Clear(WHITE);

    char buffer[100];
    // sprintf(buffer, "Volume 1123123213123123123123213123123");
    sprintf(buffer, "%sVolume: %ddB  Audio input: %s", volume_ctrl.is_muted() ? "[MUTED]" : "", volume_ctrl.get_volume_db(), audio_input_to_string(audio_input_ctrl.get_audio_input()));     
    // Paint_DrawString_EN(30, 34, buffer, &Font24, BLUE, CYAN);
    draw_volume();
  }
}

