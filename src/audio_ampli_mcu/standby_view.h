#ifndef STANDBY_VIEW_GUARD_H_
#define STANDBY_VIEW_GUARD_H_

#include "draw_primitives.h"
#include "state_machine.h"

class StandbyView
{
public:
  StandbyView(
    StateMachine* state_machine_ptr,
    Display* display_ptr,
    const LvFontWrapper* font_ptr,
    const lv_img_dsc_t* cat_image_ptr);

  void draw(const bool has_state_changed);

private:
  StateMachine* state_machine_ptr_;
  Display* display_ptr_;
  const LvFontWrapper* font_ptr_;
  const lv_img_dsc_t* cat_image_ptr_;

  int timer_{0};
  int zzz_count_{0};
};

#endif  // STANDBY_VIEW_GUARD_H_