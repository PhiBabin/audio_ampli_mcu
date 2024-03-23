#ifndef VOL_CTRL_GUARD_H_
#define VOL_CTRL_GUARD_H_

#include "pio_encoder.h"
#include "toggle_button.h"

#include <array>

class VolumeController
{
public:
  // Construtor
  VolumeController(const std::array<pin_size_t, 6> gpio_pin_vol_select, 
                   PioEncoder* vol_encoder_ptr,
                   const int mute_button_pin,
                   const int32_t startup_volume_percentage,
                   const int32_t total_tick_for_100percent);

  // Init GPIO pins
  void init();
  
  // Return current volume as a value in the 0-100% range
  int32_t get_volume_percentage() const;

  // Return whether we're muted
  bool is_muted() const;

  // Toggle muted state
  void toggle_mute(uint8_t);

  // Read encoder, update state and set GPIO pin that set the volume.
  // return true on change in volume or mute status
  bool update();

private:
  // Read encoder, update state and set GPIO pin that set the volume.
  bool update_volume();

  // Update mute state
  bool update_mute();

  // Set GPIO based on current volume
  void set_gpio_based_on_volume();

  // GPIO pin for each of the 6 bit of the volume
  std::array<pin_size_t, 6> gpio_pin_vol_select_;
  // Pin for the mute toggle button
  pin_size_t mute_button_pin_;
  /// Volume as a wrap arround integer
  int32_t volume_;
  /// Previous count of the encoder
  int32_t prev_encoder_count_;
  /// How many encoder tick correspond to the full range of 0-100%
  int32_t total_tick_for_100percent_;
  /// Pointer to the quadrature encoder
  PioEncoder* vol_encoder_ptr_;
  /// Toggle button for the mutting
  ToggleButton mute_button_;
};

#endif // VOL_CTRL_GUARD_H_