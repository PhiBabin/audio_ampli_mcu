#include "app.h"

#include "cat_sleep_img.h"
#include "digit_font.h"
// #include "digit_font_droid_sans_mono.h"
#include "digit_font_droid_sans_mono_130.h"
#include "dm_sans_bold_62.h"
#include "dm_sans_extrabold.h"

#include "dm_sans_regular_40.h"

#include <math.h>

App::App()
  : volume_encoder_(pin_out::volume_encoder_b.pin)
  , menu_select_encoder_(pin_out::menu_select_encoder_b.pin)
  , io_expander_1_(pin_out::io_expander_chip_select.pin)
  , phono_io_expander_(pin_out::phono_io_expander_chip_select.pin)
#if defined(USE_V2_PCB)
  , io_expander_2_(pin_out::io_expander_chip_select.pin, /*address=*/1)
#endif
  , io_expanders_{
#if defined(USE_V2_PCB)
      std::make_pair(GpioModule::io_expander_1, &io_expander_1_),
      std::make_pair(GpioModule::io_expander_phono, &phono_io_expander_),
      std::make_pair(GpioModule::io_expander_2, &io_expander_2_)
#else
      std::make_pair(GpioModule::io_expander_1, &io_expander_1_),
      std::make_pair(GpioModule::io_expander_phono, &phono_io_expander_)
#endif
  }
  , gpio_handler_(io_expanders_)
  , persistent_data_{}
  , persistent_data_flasher_{}
  , state_machine_{}
  , volume_ctrl_(state_machine_, persistent_data_, volume_encoder_, gpio_handler_)
  , option_ctrl_(state_machine_, persistent_data_, volume_ctrl_, gpio_handler_)
  , display_{}
  , digit_droid_sans_font_(&digit_font_droid_sans_mono_130, true) // droid_sans_mono
  , digit_light_font_(&dmsans_36pt_light, true)
  , regular_bold_font_(&dmsans_36pt_extrabold)
  , regular_medium_font_(&dmsans_36pt_regular_40)
  , regular_large_font_(&dm_sans_bold_62)
  , main_menu_view_(option_ctrl_, volume_ctrl_, persistent_data_, state_machine_, regular_bold_font_, digit_droid_sans_font_, regular_medium_font_)
  , option_view_(option_ctrl_, volume_ctrl_, persistent_data_, state_machine_, regular_bold_font_, regular_medium_font_, regular_large_font_)
  , standby_view_(state_machine_, display_, regular_bold_font_, cat_sleep_image)
  , interaction_handler_(option_view_, main_menu_view_, volume_ctrl_, state_machine_, menu_select_encoder_)
  , remote_ctrl_(state_machine_, interaction_handler_, volume_ctrl_)
{
}

void App::init()
{
  Serial.begin(115200);
  Serial.println("Starting up...");

  persistent_data_flasher_.init();
  if (persistent_data_flasher_.maybe_load_data(persistent_data_))
  {
    Serial.println("Data from flash loaded");
  }
  else
  {
    Serial.println("No data found in flash, using default settings.");
    persistent_data_ = PersistentData{};
    persistent_data_flasher_.force_save(persistent_data_);
  }

  display_.gpio_init();

  // Init all IO expanders
  for (const auto& [_, io_expander_ptr] : io_expanders_)
  {
    io_expander_ptr->begin();
  }

  volume_encoder_.begin();
  menu_select_encoder_.begin();

  volume_ctrl_.init();
  option_ctrl_.init();
  remote_ctrl_.init();
  interaction_handler_.init();

  display_.init();
  display_.clear_screen(BLACK_COLOR);
  display_.blip_framebuffer();

  main_menu_view_.init();
  option_view_.init();

  option_ctrl_.power_on();

#ifdef USE_V2_PCB
  gpio_handler_.cache_init_input(pin_out::power_detect);
#endif
}

void App::update_low_power_timer()
{
#ifdef USE_V2_PCB
  if (state_machine_.get_state() != State::main_menu)
  {
    return;
  }

  const auto timer_option = persistent_data_.inactivity_timer_option;
  if (timer_option == InactivityTimerOption::off)
  {
    return;
  }

  uint32_t threshold_ms;
  switch (timer_option)
  {
    case InactivityTimerOption::_15min:
      threshold_ms = 900000;
      break;
    case InactivityTimerOption::_1h:
      threshold_ms = 3600000;
      break;
    case InactivityTimerOption::_3h:
      threshold_ms = 10800000;
      break;
    default:
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
    if (maybe_timer.has_value() && (millis() - maybe_timer.value()) > threshold_ms)
    {
      Serial.println("Turning off...");
      option_view_.power_off();
      state_machine_.change_state(State::standby);
      maybe_timer = std::nullopt;
    }
  } // not detected -> detected
  else if (is_power_detected && !prev_power_detected)
  {
    Serial.println("End inactive timer...");
    maybe_timer = std::nullopt;
  }

  prev_power_detected = is_power_detected;
#endif
}

void App::tick()
{
  remote_ctrl_.decode_command();
  interaction_handler_.update();
  volume_ctrl_.update();

  const auto has_state_changed = state_machine_.update();
  if (has_state_changed)
  {
    Serial.println("state changed!");
    // Checkerboard dissolve before transitioning to the new state
    display_.checkerboard_dissolve();
    display_.blip_framebuffer();
    display_.clear_screen(BLACK_COLOR);
  }

  switch (state_machine_.get_state())
  {
    case State::main_menu:
      main_menu_view_.draw(display_, has_state_changed);
      break;
    case State::option_menu:
      option_view_.draw(display_, has_state_changed);
      break;
    case State::standby:
      standby_view_.draw(has_state_changed);
      break;
  }

  update_low_power_timer();

  persistent_data_flasher_.save(persistent_data_);
  display_.blip_framebuffer();
}



void App::test_draw_speed()
{
  const auto N = 10;
  display_.clear_screen(BLACK_COLOR);
  const auto start = millis();
  for (int i = 0; i < N; ++i)
  {
    for (uint16_t y = 0; y < LCD_HEIGHT; ++y)
    {
      for (uint16_t x = 0; x < LCD_WIDTH; ++x)
      {
        display_.set_pixel(x, y, (x ^ y) % 9 == 0 ? WHITE_COLOR : BLACK_COLOR);
      }
    }

    // LCD_Clear_12bitRGB_async(i % 2 == 0 ? BLACK_COLOR : WHITE_COLOR);
    display_.blip_framebuffer();
  }
  const auto end = millis();
  Serial.print("Took total: ");
  Serial.print(end - start);
  Serial.print("ms or ");
  Serial.print(static_cast<float>(end - start) / static_cast<float>(N));
  Serial.println("ms/frame");
}

void App::test_clear_rectangle()
{
  const auto N = 5;
  display_.clear_screen(BLACK_COLOR);

  Serial.println("---------------------------------------");
  // display_.draw_rectangle(10, 20, 20, 30, WHITE_COLOR);
  for (int i = 0; i < N; ++i)
  {
    Serial.println("+++++++++++++++++++++++++++++++++++++");
    int x_start = 10 + i;
    for (int width = 1; width < 10; ++width)
    {
      display_.draw_rectangle(x_start, 10 + 34 * i, x_start + width, 30 + 34 * i, WHITE_COLOR);
      x_start += width + 5;
    }
  }
  display_.blip_framebuffer();
  Serial.println("ms/frame");
  while (true)
  {
  }
}

/// Animated test that exercises the new Rect-based clipping.
/// Shapes and text are deliberately driven off all four screen edges
/// to prove that set_pixel_unsafe is never called with out-of-bounds coordinates.
void App::test_bounds_check()
{
  static uint32_t frame = 0;
  ++frame;

  const float t = frame * 0.02f;
  display_.clear_screen(BLACK_COLOR);

  // --- 1. Image: wide ellipse, goes off ALL edges ---
  {
    const int32_t cx = static_cast<int32_t>(sinf(t) * (LCD_WIDTH * 0.65f));
    const int32_t cy = static_cast<int32_t>(cosf(t * 1.3f) * (LCD_HEIGHT * 0.65f));
    const int32_t img_x = LCD_WIDTH / 2 + cx - static_cast<int32_t>(cat_sleep_image.w_px / 2);
    const int32_t img_y = LCD_HEIGHT / 2 + cy - static_cast<int32_t>(cat_sleep_image.h_px / 2);
    draw_image_from_top_left(display_, cat_sleep_image, img_x, img_y);
  }

  // --- 2. Rounded rectangle: diagonal sweep, goes off ALL edges ---
  {
    const int32_t rr_w = 110;
    const int32_t rr_h = 70;
    const int32_t rr_x = static_cast<int32_t>((frame * 2) % (LCD_WIDTH + rr_w + 40)) - (rr_w + 20);
    const int32_t rr_y = static_cast<int32_t>((frame * 3) % (LCD_HEIGHT + rr_h + 40)) - (rr_h + 20);
    draw_rounded_rectangle(display_, rr_x, rr_y, rr_x + rr_w, rr_y + rr_h, true, true, true, 14);
  }

  // --- 3. Text: horizontal scroll, goes off right edge ---
  {
    const int32_t txt_x = static_cast<int32_t>((frame * 2) % (LCD_WIDTH + 120)) - 60;
    draw_string_fast(display_, "RIGHT-CLIP", txt_x, 10, txt_x + 260, regular_bold_font_, true, false, TextAlign::left);
  }

  // --- 4. Text: vertical scroll, goes off top & bottom ---
  {
    const int32_t txt_y = static_cast<int32_t>((frame * 3) % (LCD_HEIGHT + 80)) - 40;
    draw_string_fast(display_, "BOTTOM", 10, txt_y, 200, regular_medium_font_, true, false, TextAlign::left);
  }

  // --- 5. Centered text: oscillates past left & right edges ---
  {
    const int32_t offset = static_cast<int32_t>(sinf(t * 0.7f) * (LCD_WIDTH * 0.55f));
    const int32_t box_cx = LCD_WIDTH / 2 + offset;
    draw_string_fast(display_, "CENTER", box_cx, LCD_HEIGHT / 2 - 20, box_cx + 160, regular_bold_font_, true, false, TextAlign::center);
  }

  // --- 6. Small frame counter (always visible) ---
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", frame);
    draw_string_fast(display_, buf, 5, LCD_HEIGHT - 35, 120, regular_medium_font_, true, false, TextAlign::left);
  }

  display_.blip_framebuffer();
  // delay(16);  // ~60 fps
}
