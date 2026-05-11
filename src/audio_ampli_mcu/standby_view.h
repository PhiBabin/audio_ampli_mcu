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
  StateMachine& state_machine_;
  Display& display_;
  const LvFontWrapper& font_;
  const lv_img_dsc_t& cat_image_;

  int timer_{0};
  int zzz_count_{0};
};

#endif  // STANDBY_VIEW_GUARD_H_
