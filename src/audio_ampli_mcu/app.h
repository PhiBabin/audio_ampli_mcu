#ifndef APP_GUARD_H_
#define APP_GUARD_H_

#include "LCD_Driver.h"
#include "config.h"
#include "gpio_handler.h"
#include "interaction_handler.h"
#include "io_expander.h"
#include "main_menu_view.h"
#include "options_controller.h"
#include "options_view.h"
#include "persistent_data.h"
#include "remote_controller.h"
#include "standby_view.h"
#include "state_machine.h"
#include "volume_controller.h"

#ifdef SIM
#include "sim/pio_encoder.h"
#else
#include "pio_encoder.h"
#endif

#include <vector>

/// Top-level application class that owns all subsystems.
/// Replaces the scattered global variables in the .ino file.
class App
{
public:
  App();

  // Initialize all hardware and subsystems
  void init();

  // Single application tick (called repeatedly from loop())
  void tick();

private:
  void update_low_power_timer();


  void test_draw_speed();
  void test_clear_rectangle();
  void test_bounds_check();

  // --- Hardware / low-level ---
  PioEncoder volume_encoder_;
  PioEncoder menu_select_encoder_;
  IoExpander io_expander_1_;
  IoExpander phono_io_expander_;
#if defined(USE_V2_PCB)
  IoExpander io_expander_2_;
#endif

  std::vector<GpioHandler::ModuleEnumExpanderPair> io_expanders_;
  GpioHandler gpio_handler_;

  // --- Data / state ---
  PersistentData persistent_data_;
  PersistentDataFlasher persistent_data_flasher_;
  StateMachine state_machine_;

  // --- Controllers ---
  VolumeController volume_ctrl_;
  OptionController option_ctrl_;

  // --- Display / fonts ---
  Display display_;
  LvFontWrapper digit_droid_sans_font_;
  LvFontWrapper digit_light_font_;
  LvFontWrapper regular_bold_font_;
  LvFontWrapper regular_medium_font_;
  LvFontWrapper regular_large_font_;

  // --- Views ---
  MainMenuView main_menu_view_;
  OptionsView option_view_;
  StandbyView standby_view_;

  // --- Input ---
  InteractionHandler interaction_handler_;
  RemoteController remote_ctrl_;
};

#endif  // APP_GUARD_H_
