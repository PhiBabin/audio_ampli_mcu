#ifndef VOL_CTRL_GUARD_H_
#define VOL_CTRL_GUARD_H_

#ifdef SIM
#include "sim/pio_encoder.h"
#include "sim/toggle_button.h"
#else
#include "pio_encoder.h"
#include "toggle_button.h"
#endif

#include "gpio_handler.h"
#include "persistent_data.h"
#include "state_machine.h"

#include <array>
#include <tuple>

class VolumeController
{
public:
  // Constructor
  VolumeController(
    StateMachine& state_machine,
    PersistentData& persistent_data,
    PioEncoder& vol_encoder,
    GpioHandler& gpio_handler);

  // Init GPIO pins
  void init();

  // Return current volume as a value in tenth-dB units
  int32_t get_volume_db() const;

  // Return the integer dB part (truncation toward zero)
  // e.g. -205 tenth-dB -> -20
  int32_t get_volume_db_int() const;

  // Return the tenth-dB remainder (0-9, for display)
  // e.g. -205 tenth-dB -> 5
  uint8_t get_volume_tenth_db_rem() const;

  // Update current volume in db
  void set_volume_db(const int32_t new_volume_db);

  // Add a certain amount of db to the current volume.
  void increase_volume_db(const int32_t delta_volume_db);

  // Return whether we're muted
  bool is_muted() const;

  // Toggle muted state
  void toggle_mute();

  // Toggle muted state
  void set_mute(const bool is_mute);

  // Read encoder, update state and set GPIO pin that set the volume.
  // return true on change in volume or mute status
  bool update();

  // When audio input change, the volume is changed
  void on_audio_input_change();

  // When selected options change, the volume is changed
  void on_option_change();

  // Set volume of the left and right stereo by triggering the GPIOs.
  void set_gpio_based_on_volume();

  // Get how much the compensation for the left and right speaker.
  std::tuple<int16_t, int16_t> get_left_right_bias_compensation();

private:
  // Read encoder, update state and set GPIO pin that set the volume.
  bool update_volume();

  // Update mute state
  bool update_mute();

  // Read current volume db on the current audio input and reset the volume tick count
  void reset_volume_tick_count_based_volume_db();

  // Update the volume of one stereo side for firmware version v1.
  // Since relays don't take the same time to change from 0 -> 1 than to 1 -> 0, so delay logic is applied.
  void latch_volume_gpio_one_side_v1(const uint8_t prev_vol_6bit, const uint8_t vol_6bit, const GpioPin& latch_pin, const std::array<GpioPin, 6U>& volume_pins);

  // Update the volume of one stereo side for firmware version v2.
  // Since relays don't take the same time to change from 0 -> 1 than to 1 -> 0, so delay logic is applied.
  void latch_volume_gpio_one_side_v2(
    const uint8_t prev_vol_7bit, const uint8_t vol_7bit, const std::array<GpioPin, 7U>& volume_pins);

  /// Update the GPIOs pins of the volume based on @c vol_7bit.
  /// @param[in] vol_7bit Values of the GPIOs
  /// @param[in] mask Only the (mask & vol_7bit) GPIO will be updated.
  void set_gpio_volume(const std::array<GpioPin, 7U>& volume_pins, const uint8_t vol_7bit, const uint8_t mask = 0xff);

  /// Update the GPIOs pins of the volume (6-bit variant for V1/V0).
  void set_gpio_volume(const std::array<GpioPin, 6U>& volume_pins, const uint8_t vol_6bit, const uint8_t mask = 0xff);

  // Determine the correct gain based on the left+right effective volume and set GPIOs
  void set_gain_based_on_volume(const int32_t left_vol_tenth, const int32_t right_vol_tenth);

  /// Update the volume of one stereo side for the V0 firmware.
  void latch_volume_gpio_one_side(const uint8_t prev_vol_6bit, const uint8_t vol_6bit, const GpioPin& latch_pin);

  // Reference to the state machine
  StateMachine& state_machine_;
  // Reference to the persistent data
  PersistentData& persistent_data_;
  // Handler to read/write to GPIO from the pico or to IO expander
  GpioHandler& gpio_handler_;
  /// Previous count of the encoder
  int32_t prev_encoder_count_;
  /// How many encoder tick per db of volume
  int32_t tick_per_db_;
  /// Reference to the quadrature encoder
  PioEncoder& vol_encoder_;
  /// Is the device muted?
  bool is_muted_{false};
  /// If the volume/mute is change outside of the update_XX(), this keep latch the update
  bool latched_volume_updated_{false};
  /// When set_gpio_based_on_volume() is called we need to know what was previous value of the GPIOs for each stereo
  /// side
  uint8_t prev_vol_set_on_left_{0};
  uint8_t prev_vol_set_on_right_{0};
};

#endif  // VOL_CTRL_GUARD_H_
