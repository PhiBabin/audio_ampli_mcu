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
#include <optional>
#include <unordered_map>

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



class LvFontWrapper
{
public:
  struct LvCharacter
  {
    uint8_t get_color(const uint32_t bitmap_x_px, const uint32_t bitmap_y_px) const
    {
      const uint32_t bitmap_y_offset_px =  width_px % 2 == 0 ? width_px * bitmap_y_px : (width_px + 1) * bitmap_y_px; // Padding for odd width
      const uint32_t offset_px = bitmap_y_offset_px + bitmap_x_px;
      const auto two_pixels_byte = raw_bytes[offset_px / 2];
      if (offset_px % 2 == 0)
      {
        return two_pixels_byte >> 4;
      }
      return two_pixels_byte & 0x0F;
    };
  
    uint32_t width_px;
    uint32_t height_px;
    const uint8_t* raw_bytes;
  };

  LvFontWrapper(const lv_font_t* font) : font_(font)
  {
    const uint32_t* unicodes_list = font_->unicode_list;
    size_t i = 0;
    while (unicodes_list[i] != 0)
    {
      LvCharacter character{
        .width_px = font_->glyph_dsc[i].w_px,
        .height_px = font_->h_px,
        .raw_bytes = font_->glyph_bitmap + font_->glyph_dsc[i].glyph_index
      };
      unicode_to_char_.emplace(unicodes_list[i], character);
      ++i;
    }
  }

  std::optional<const LvCharacter*> get_character(const char c) const
  {
    const auto iter = unicode_to_char_.find(static_cast<uint32_t>(c));
    if (iter == unicode_to_char_.end())
    {
      return {};
    }
    return std::optional<const LvCharacter*>(&iter->second);
  };




private:
  // Unicode to the character
  std::unordered_map<uint32_t, LvCharacter> unicode_to_char_;
  const lv_font_t* font_;
};


LvFontWrapper digit_thin_font(&dmsans_80pt_thin);

void draw_character_fast(const LvFontWrapper::LvCharacter* character, const uint32_t start_x, const uint32_t start_y, bool is_b_and_w = true)
{
  uint32_t end_x = start_x + character->width_px;
  const uint32_t end_y = start_y + character->height_px;
  
  // Make sure that we have an even number of columns, that way we don't have to worry about write call with only one column
  if (end_x - start_x % 2 != 0)
  {
    ++end_x;
  }

  // LCD will auto increment the row when we reach columns == end_x
	LCD_SetWindow(start_x, start_y + 1, end_x, end_y + 1);
  for (int32_t y = start_y; y < end_y; ++y)
  {
    uint8_t px_count = 0;
    uint32_t color_2pixels = 0;
    // LCD_SetCursor(start_x, y);
    for (int32_t x = start_x; x < end_x; ++x)
    {
      const auto color_4bit = character->get_color(start_x - x, y - start_y);
      
      // Convert 4bit grayscale to four 4bit RGB
      color_2pixels = (color_2pixels << 12) | (color_4bit << 8 | color_4bit << 4 | color_4bit);
      ++px_count;
      if (px_count == 2)
      {
        send_2pixel_color(color_2pixels);
        px_count = 0;
        color_2pixels = 0;
      }
    }
  }
}

void draw_string_fast(const char* str, const uint32_t start_x, const uint32_t start_y, const uint32_t end_x, const LvFontWrapper& font, bool is_b_and_w = true)
{
  if (*str == '\0')
  {
    return;
  }
  if (start_x > end_x)
  {
    return;
  }
  uint32_t text_width_px = 0;
  const char* str_temp = str;
  while (*str_temp != '\0')
  {
    if (const auto maybe_char = font.get_character(*str); maybe_char)
    {
      text_width_px += maybe_char.value()->width_px;
    }
    ++str_temp;
  }
  const auto middle_x = (end_x - start_x) / 2 + start_x;
  const auto start_text_x = middle_x - text_width_px / 2;
  const auto end_text_x = middle_x + text_width_px / 2;
  if (start_x < start_text_x)
  {
    // whiteout
  }
  if (end_text_x < end_x)
  {
    // whiteout
  }
  str_temp = str;
  auto current_text_x = start_text_x;
  while (*str_temp != '\0')
  {
    if (const auto maybe_char = font.get_character(*str); maybe_char)
    {
      draw_character_fast(*maybe_char, start_text_x, start_y, is_b_and_w);
    }
    ++str_temp;
  }

}

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
    std::size_t offset = 0;
    for (size_t i = 0; i < digit_; ++i)
    {
      const auto width = dmsans_80pt_thin_width_px[i];
      Serial.println(width);
      const auto actual_width = width % 2 == 0 ? width : width + 1;
      start_digit_bitmap_ptr += (actual_width * max_height) / 2;
      offset += (actual_width * max_height) / 2;
    }
    digit_width = dmsans_80pt_thin_width_px[digit_];
    
    Serial.print("d=");
    Serial.print(digit_);
    Serial.print(" start=");
    Serial.print(offset);
    Serial.println("");
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
    // return 0xF;
    const uint32_t bitmap_x_px = x - start_x - (max_width - digit_width);
    const uint32_t bitmap_y_px =  y - start_y;
    const uint32_t bitmap_y_offset_px =  digit_width % 2 == 0 ? digit_width * bitmap_y_px : (digit_width + 1) * bitmap_y_px; // Padding for odd width
    const uint32_t offset_px = bitmap_y_offset_px + bitmap_x_px;
    const auto two_pixels_byte = start_digit_bitmap_ptr[offset_px  / 2];
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
  uint32_t start_x = 320;
  uint32_t start_y = 320;
  uint32_t end_x = 0;
  uint32_t end_y = 0;
  if (maybe_first_digit >= 0)
  {
    digits[0].set_digit(maybe_first_digit == 0 ? BLANK_DIGIT : maybe_first_digit);
    digits[0].enabled = true;
    digits[0].start_x = 68;
    digits[0].start_y = 38;
    start_x = digits[0].start_x;
    start_y = digits[0].start_y;
    end_x = digits[0].start_x + digits[0].max_width;
    end_y = digits[0].start_y +  digits[0].max_height;
  }
  if (maybe_second_digit >= 0)
  {
    digits[1].set_digit(maybe_second_digit);
    digits[1].enabled = true;
    digits[1].start_x = 172;
    digits[1].start_y = 38;
    start_x = min(digits[1].start_x, start_x);
    start_y = min(digits[1].start_y, start_y);
    end_x =  max(digits[1].start_x + digits[1].max_width, end_x);
    end_y =  max(digits[1].start_y +  digits[1].max_height, end_y);
  }

  // Make sure that we have an even number of columns, that way we don't have to worry about write call with only one column
  if (end_x - start_x % 2 != 0)
  {
    ++end_x;
  }

  // LCD will auto increment the row when we reach columns == end_x
	LCD_SetWindow(start_x, start_y + 1, end_x, end_y + 1);
  
  Serial.print(start_x);
  Serial.print(" ");
  Serial.print(start_y);
  Serial.print(" ");
  Serial.print(end_x);
  Serial.print(" ");
  Serial.print(end_y);
  Serial.println("foo");
  for (int32_t y = start_y; y < end_y; ++y)
  {
    uint8_t px_count = 0;
    uint32_t color_2pixels = 0;
    // LCD_SetCursor(start_x, y);
    for (int32_t x = start_x; x < end_x; ++x)
    {
      uint8_t color_4bit = BLANK_COLOR;
      for (const auto& digit : digits)
      {
        if (digit.enabled && digit.is_within_boundary(x, y))
        {
          // Serial.print(x);
          // Serial.print(" ");
          // Serial.println(y);
          color_4bit = digit.get_color(x, y);
        }
      }
      
      // Convert 4bit grayscale to four 4bit RGB
      color_2pixels = (color_2pixels << 12) | (color_4bit << 8 | color_4bit << 4 | color_4bit);
      ++px_count;
      if (px_count == 2)
      {
        send_2pixel_color(color_2pixels);
        px_count = 0;
        color_2pixels = 0;
      }
    }
  }
}

void draw_volume()
{
  static auto is_init = false;
  static auto prev_is_muted = volume_ctrl.is_muted();
  static auto prev_volume_db = volume_ctrl.get_volume_db();
  const auto curr_is_muted = volume_ctrl.is_muted();
  const auto curr_volume_db = volume_ctrl.get_volume_db();
  if (is_init && curr_volume_db == prev_volume_db  && curr_is_muted == prev_is_muted)
  {
    return;
  }
  const auto first_digit = curr_volume_db / 10;
  const auto second_digit = curr_volume_db % 10;
  const auto first_digit_changed = first_digit != (prev_volume_db / 10) || is_init;
  const auto second_digit_changed = second_digit != (prev_volume_db % 10) || is_init;

  fast_draw_digits(first_digit_changed ? first_digit : -1, second_digit_changed ? second_digit : -1);
  prev_is_muted = curr_is_muted;
  prev_volume_db = curr_volume_db;
  is_init = true;
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

  Config_Init();
  LCD_Init();
  LCD_Clear_12bitRGB(0x0000);

  draw_volume();
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
    // LCD_Clear(0xffff);
    // Paint_Clear(WHITE);

    char buffer[100];
    // sprintf(buffer, "%sVolume: %ddB  Audio input: %s", volume_ctrl.is_muted() ? "[MUTED]" : "", volume_ctrl.get_volume_db(), audio_input_to_string(audio_input_ctrl.get_audio_input()));     
    // draw_volume();
    sprintf(buffer, "%d", volume_ctrl.get_volume_db());
    draw_string_fast(buffer, 68, 38, 68 + 210, digit_thin_font);
  }
}

