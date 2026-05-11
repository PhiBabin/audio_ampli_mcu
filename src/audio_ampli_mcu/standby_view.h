#ifndef STANDBY_VIEW_GUARD_H_
#define STANDBY_VIEW_GUARD_H_

#include "draw_primitives.h"
#include "state_machine.h"

class StandbyView
{
public:
  StandbyView(
    StateMachine& state_machine,
    Display& display,
    const LvFontWrapper& font,
    const lv_img_dsc_t& cat_image);

  void draw(const bool has_state_changed);

private:
  struct SleepZ
  {
    int16_t spawn_x{0};
    int16_t spawn_y{0};
    int16_t x{0};
    int16_t y{0};
    int16_t drawn_x{0};
    int16_t drawn_y{0};
    uint32_t birth_time{0};
    bool active{false};
    bool has_been_drawn{false};

    // Per-particle random variation
    float drift_phase{0.0f};      // Random sine phase offset
    float drift_amp{1.0f};        // Random amplitude multiplier
    float rise_speed_mult{1.0f};  // Random rise speed multiplier
    float right_drift_mult{1.0f}; // Random rightward drift multiplier
  };

  static constexpr int MAX_Z_COUNT = 6;

  StateMachine& state_machine_;
  Display& display_;

  const LvFontWrapper& font_;
  const lv_img_dsc_t& cat_image_;

  int16_t cat_x_ = 0;
  int16_t cat_y_ = 0;
  int8_t breath_offset_ = 0;
  uint32_t breath_phase_start_ = 0;

  SleepZ z_particles_[MAX_Z_COUNT];
  uint32_t next_spawn_time_ = 0;

  int16_t z_glyph_width_ = 16;
  int16_t z_glyph_height_ = 16;
};

#endif  // STANDBY_VIEW_GUARD_H_
