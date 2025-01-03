#ifndef FLASH_DATA_GUARD_H_
#define FLASH_DATA_GUARD_H_

#include "audio_input_enums.h"
#include "config.h"
#include "option_enums.h"

#include <optional>

constexpr uint8_t NUM_INPUT_OUTPUT_PERMUTATION = NUM_AUDIO_INPUT * NUM_OUTPUT_MODE * NUM_OUTPUT_TYPE;

// Internal data that will be saved to the flash
struct PersistentData
{
public:
  // Each audio input and audio output pair have their own volume and gain setting
  struct PerAudioInputOutputData
  {
    int32_t volume_db{STARTUP_VOLUME_DB};
    GainOption gain_value{GainOption::low};
  };

  uint16_t checksum{0};
  uint16_t magic_num{0xCAFE};
  uint8_t version_num{VERSION_NUMBER};
  bool is_muted{false};
  AudioInput selected_audio_input{AudioInput::rca_1};
  PerAudioInputOutputData per_audio_input_output_data[NUM_INPUT_OUTPUT_PERMUTATION];
  OutputModeOption output_mode_value{OutputModeOption::phones};
  OutputTypeOption output_type_value{OutputTypeOption::se};
  OnOffOption sufwoofer_enable_value{OnOffOption::off};

  // Getters
  const PerAudioInputOutputData& get_per_audio_input_output_data() const;
  PerAudioInputOutputData& get_per_audio_input_output_data_mutable();
  const int32_t& get_volume_db() const;
  int32_t& get_volume_db_mutable();
  const GainOption& get_gain() const;
  GainOption& get_gain_mutable();

  /// Compute checksum of the persistent data.
  uint8_t compute_checksum() const;

  bool operator==(const PersistentData& rhs) const;
  bool operator!=(const PersistentData& rhs) const;

private:
  /// There are 4 audio inputs, 2 output modes and 2 output types, which mean 16 possible combinaisons. This helper
  /// function returns the current offset in the @c per_audio_input_output_data array.
  size_t current_input_output_pair_index() const;
};

class PersistentDataFlasher
{
public:
  void init();
  bool maybe_load_data(PersistentData& data_out);
  void save(const PersistentData& curr_data);
  void force_save(const PersistentData& curr_data);

private:
  // Data at the last call of save.
  std::optional<PersistentData> maybe_changed_data_;
  // Maybe the last saved data
  std::optional<PersistentData> maybe_last_saved_data_;
  // Time since last change in data
  std::optional<unsigned long> maybe_time_since_last_change_to_data_;
};

#endif  // FLASH_DATA_GUARD_H_