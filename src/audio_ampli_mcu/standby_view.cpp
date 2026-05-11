#include "standby_view.h"

#include <algorithm>
#include <cmath>

namespace
{
  constexpr uint32_t SPAWN_INTERVAL_MS = 2000;
  constexpr uint32_t Z_LIFETIME_MS = 6000;
  constexpr uint32_t BREATH_CYCLE_MS = 4000;
  constexpr uint32_t BREATH_HOLD_MS = 1500;
  constexpr int16_t Z_SPAWN_OFFSET_X = 100;
  constexpr int16_t Z_SPAWN_OFFSET_Y = 5;
  constexpr float Z_RISE_SPEED_PPS = 18.0f;
  constexpr float Z_DRIFT_RIGHT_PPS = 8.0f;
  constexpr float Z_DRIFT_AMPLITUDE = 10.0f;
  constexpr float Z_DRIFT_FREQ = 2.5f;

  constexpr uint32_t grayscale_color(const uint32_t intensity_4bit)
  {
    return (intensity_4bit << 8) | (intensity_4bit << 4) | intensity_4bit;
  }

  // Simple pseudo-random using bit-mixed millis. Returns [0.0, 1.0).
  float random_float(const uint32_t seed)
  {
    // xorshift-ish bit mixing
    uint32_t x = seed * 747796405u + 2891336453u;
    x = ((x >> ((x >> 28) + 4)) ^ x) * 277803737u;
    x = (x >> 22) ^ x;
    return static_cast<float>(x) / 4294967296.0f;
  }
}

StandbyView::StandbyView(
  StateMachine& state_machine,
  Display& display,
  const LvFontWrapper& font,
  const lv_img_dsc_t& cat_image)
  : state_machine_(state_machine)
  , display_(display)
  , font_(font)
  , cat_image_(cat_image)
{
  // Cache glyph dimensions for 'Z'
  if (const auto maybe_glyph = font_.get_glyph('Z'); maybe_glyph)
  {
    z_glyph_width_ = static_cast<int16_t>(maybe_glyph.value()->width_px);
    z_glyph_height_ = static_cast<int16_t>(maybe_glyph.value()->height_px);
  }
  else
  {
    z_glyph_width_ = 16;
    z_glyph_height_ = 16;
  }
}

void StandbyView::draw(const bool has_state_changed)
{
  if (state_machine_.get_state() != State::standby)
  {
    return;
  }

  const uint32_t now = millis();

  // Initialize when entering standby
  if (has_state_changed)
  {
    display_.clear_screen(BLACK_COLOR);

    cat_x_ = static_cast<int16_t>(LCD_WIDTH - cat_image_.w_px);
    cat_y_ = static_cast<int16_t>(LCD_HEIGHT - cat_image_.h_px + 1);
    breath_offset_ = 0;
    breath_phase_start_ = now;

    draw_image_from_top_left(display_, cat_image_, cat_x_, cat_y_);

    for (int i = 0; i < MAX_Z_COUNT; ++i)
    {
      z_particles_[i].active = false;
      z_particles_[i].has_been_drawn = false;
    }
    next_spawn_time_ = now + SPAWN_INTERVAL_MS / 2;
    return;
  }

  // --- Breathing animation ---
  const uint32_t breath_age = now - breath_phase_start_;
  if (breath_age >= BREATH_CYCLE_MS)
  {
    breath_phase_start_ = now;
  }
  else
  {
    const bool want_breath_up = breath_age < BREATH_HOLD_MS;
    if (want_breath_up && breath_offset_ == 0)
    {
      // Inhale: shift cat up by 1 pixel
      breath_offset_ = -1;
      // Clear the bottom row exposed by the shift
      const int16_t clear_y = cat_y_ + static_cast<int16_t>(cat_image_.h_px) - 1;
      if (clear_y >= 0 && clear_y < LCD_HEIGHT)
      {
        display_.draw_rectangle(
          static_cast<uint16_t>(cat_x_),
          static_cast<uint16_t>(clear_y),
          static_cast<uint16_t>(cat_x_ + cat_image_.w_px),
          static_cast<uint16_t>(clear_y + 1),
          BLACK_COLOR);
      }
      draw_image_from_top_left(display_, cat_image_, cat_x_, cat_y_ - 1);
    }
    else if (!want_breath_up && breath_offset_ == -1)
    {
      // Exhale: return cat to original position
      breath_offset_ = 0;
      // Clear the top row exposed by the shift
      const int16_t clear_y = cat_y_ - 1;
      if (clear_y >= 0 && clear_y < LCD_HEIGHT)
      {
        display_.draw_rectangle(
          static_cast<uint16_t>(cat_x_),
          static_cast<uint16_t>(clear_y),
          static_cast<uint16_t>(cat_x_ + cat_image_.w_px),
          static_cast<uint16_t>(clear_y + 1),
          BLACK_COLOR);
      }
      draw_image_from_top_left(display_, cat_image_, cat_x_, cat_y_);
    }
  }

  // --- Spawn new Z particles ---
  if (now >= next_spawn_time_)
  {
    for (int i = 0; i < MAX_Z_COUNT; ++i)
    {
      if (!z_particles_[i].active)
      {
        auto& z = z_particles_[i];
        z.active = true;
        z.has_been_drawn = false;
        z.birth_time = now;
        z.spawn_x = cat_x_ + Z_SPAWN_OFFSET_X;
        z.spawn_y = cat_y_ + Z_SPAWN_OFFSET_Y + breath_offset_;
        z.x = z.spawn_x;
        z.y = z.spawn_y;

        // Randomize this particle's personality
        const uint32_t seed = now + static_cast<uint32_t>(i * 137);
        z.drift_phase = random_float(seed) * 6.283f;           // 0 to 2π
        z.drift_amp = 0.5f + random_float(seed + 1) * 1.0f;    // 0.5 to 1.5
        z.rise_speed_mult = 1.0f; //0.7f + random_float(seed + 2) * 0.6f;  // 0.7 to 1.3
        z.right_drift_mult = 0.5f + random_float(seed + 3) * 1.0f;   // 0.5 to 1.5
        break;
      }
    }
    next_spawn_time_ = now + SPAWN_INTERVAL_MS;
  }

  // --- Update and draw Z particles ---
  for (int i = 0; i < MAX_Z_COUNT; ++i)
  {
    auto& z = z_particles_[i];
    if (!z.active)
    {
      continue;
    }

    const uint32_t age = now - z.birth_time;

    // Kill expired Zs
    if (age >= Z_LIFETIME_MS)
    {
      if (z.has_been_drawn)
      {
        display_.draw_rectangle(z.drawn_x, z.drawn_y, z.drawn_x + z_glyph_width_, z.drawn_y + z_glyph_height_, BLACK_COLOR);
      }
      z.active = false;
      z.has_been_drawn = false;
      continue;
    }

    // Compute new position with per-particle variation
    const float age_sec = static_cast<float>(age) / 1000.0f;
    const float rise = Z_RISE_SPEED_PPS * z.rise_speed_mult * age_sec;
    const float right = Z_DRIFT_RIGHT_PPS * z.right_drift_mult * age_sec;
    const float sway = Z_DRIFT_AMPLITUDE * z.drift_amp * sinf(age_sec * Z_DRIFT_FREQ + z.drift_phase);
    z.x = z.spawn_x + static_cast<int16_t>(right + sway);
    z.y = z.spawn_y - static_cast<int16_t>(rise);

    // Compute fade color: white -> gray -> black over second half of life
    uint32_t z_color = WHITE_COLOR;
    if (age > Z_LIFETIME_MS / 2)
    {
      const uint32_t fade_start = Z_LIFETIME_MS / 2;
      const uint32_t fade_range = Z_LIFETIME_MS - fade_start;
      const uint32_t fade_age = age - fade_start;
      uint32_t intensity = (0xf * (fade_range - fade_age)) / fade_range;
      z_color = grayscale_color(intensity);
    }

    // Clear only where we *actually* drew last frame
    if (z.has_been_drawn)
    {
      display_.draw_rectangle(z.drawn_x, z.drawn_y, z.drawn_x + z_glyph_width_, z.drawn_y + z_glyph_height_, BLACK_COLOR);
    }

    if (z_color != BLACK_COLOR)
    {
      draw_string_fast(
        display_,
        "Z",
        z.x,
        z.y,
        z.x + z_glyph_width_ + 10,
        font_,
        /*is_white_on_black=*/ true,
        /*clear_side=*/ false,
        TextAlign::left,
        z_color);

      z.drawn_x = z.x;
      z.drawn_y = z.y;
      z.has_been_drawn = true;
    }
    else
    {
      // Fully faded to black — nothing to draw, and next frame we won't clear
      // because there's nothing visible to erase.
      z.has_been_drawn = false;
    }
  }
}
