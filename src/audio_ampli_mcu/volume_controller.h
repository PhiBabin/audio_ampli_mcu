#ifndef VOL_CTRL_GUARD_H_
#define VOL_CTRL_GUARD_H_

#ifdef SIM
#include "sim/pio_encoder.h"
#include "sim/toggle_button.h"
#else
#include "pio_encoder.h"
#include "toggle_button.h"
#endif

#include "persistent_data.h"
#include "state_machine.h"

#include <array>

class VolumeController
{
public:
  // Construtor
  VolumeController(
    StateMachine* state_machine_ptr,
    PersistentData* persistent_data_ptr,
    const std::array<pin_size_t, 6> gpio_pin_vol_select,
    PioEncoder* vol_encoder_ptr,
    const int mute_button_pin,
    const int set_mute_pin,
    const int power_enable_pin,
    const int latch_left_vol,
    const int latch_right_vol,
    const int32_t total_tick_for_63db);

  // Init GPIO pins
  void init();

  // Return current volume as a value in the 0-63 db range
  int32_t get_volume_db() const;

  // Update current volume in db
  void set_volume_db(const int32_t new_volume_db);

  // Return whether we're muted
  bool is_muted() const;

  // Toggle muted state
  void toggle_mute();

  // Power on/off the amplificator and change to standy state
  void power_on();
  void power_off();

  // Read encoder, update state and set GPIO pin that set the volume.
  // return true on change in volume or mute status
  bool update();

  // When audio input change, the volume is changed
  void on_audio_input_change();

  // When selected options change, the volume is changed
  void on_option_change();

private:
  // Read encoder, update state and set GPIO pin that set the volume.
  bool update_volume();

  // Update mute state
  bool update_mute();

  // Set volume of the left and right stereo by triggering the GPIOs.
  void set_gpio_based_on_volume();

  // Read current volume db on the current audio input and reset the volume tick count
  void reset_volume_tick_count_based_volume_db();

  // Update the volume of one stereo side.
  // Since relays don't take the same time to change from 0 -> 1 than to 1 -> 0, so delay logic is applied.
  void latch_volume_gpio_one_side(const uint8_t prev_vol_6bit, const uint8_t vol_6bit, const pin_size_t latch_pin);

  /// Update the GPIOs pins of the volume based on @c vol_6bit.
  /// @param[in] vol_6bit Values of the GPIOs
  /// @param[in] mask Only the (mask & vol_6bit) GPIO will be updated.
  void set_gpio_volume(const uint8_t vol_6bit, const uint8_t mask = 0xff);

  // Non-owning pointer to the state machine
  StateMachine* state_machine_ptr_;
  // Non-owning pointer to the persistent data
  PersistentData* persistent_data_ptr_;
  // GPIO pin for each of the 6 bit of the volume
  std::array<pin_size_t, 6> gpio_pin_vol_select_;
  // Pin for the mute toggle button
  pin_size_t mute_button_pin_;
  // Output pin to mute / unmute
  pin_size_t set_mute_pin_;
  // Output pin to turn power on/off
  pin_size_t power_enable_pin_;
  // Latch the volume to the left stereo
  pin_size_t latch_left_vol_;
  // Latch the volume to the right stereo
  pin_size_t latch_right_vol_;
  /// Volume as a wrap around integer
  int32_t volume_;
  /// Previous count of the encoder
  int32_t prev_encoder_count_;
  /// How many encoder tick correspond to the full range of 0-100%
  int32_t total_tick_for_63db_;
  /// Pointer to the quadrature encoder
  PioEncoder* vol_encoder_ptr_;
  /// Toggle button for the mutting
  ToggleButton mute_button_;
  /// Is the device muted?
  bool is_muted_{false};
  /// If the volume/mute is change outside of the update_XX(), this keep latch the update
  bool latched_volume_updated_{false};
  /// When set_gpio_based_on_volume() is called we need to know what was previous value of the GPIOs for each stereo
  /// side
  uint8_t prev_vol_6bit_set_on_left_{0};
  uint8_t prev_vol_6bit_set_on_right_{0};
};

#endif  // VOL_CTRL_GUARD_H_