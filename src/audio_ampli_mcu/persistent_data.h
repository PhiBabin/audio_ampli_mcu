#ifndef FLASH_DATA_GUARD_H_
#define FLASH_DATA_GUARD_H_

#include "audio_input_enums.h"
#include "option_enums.h"
#include "config.h"

#include <optional>

// Internal data that will be saved to the flash
struct PersistentData
{
  // Each audio input have their own volume and gain setting
  struct PerAudioInputData
  {
    int32_t volume_db{STARTUP_VOLUME_DB};
    GainOption gain_value{GainOption::low};  
  };

  uint16_t magic_num{0xCAFE};
  uint8_t version_num{VERSION_NUMBER};
  bool is_muted{false};
  AudioInput selected_audio_input{AudioInput::rca_1};
  PerAudioInputData per_audio_input_data[NUM_AUDIO_INPUT];
  OutputModeOption output_mode_value{OutputModeOption::phones};
  OutputTypeOption output_type_value{OutputTypeOption::se};

  // Getters
  const PerAudioInputData& get_per_audio_input_data() const;
  PerAudioInputData& get_per_audio_input_data_mutable();
  const int32_t& get_volume_db() const;
  int32_t& get_volume_db_mutable();
  const GainOption& get_gain() const;
  GainOption& get_gain_mutable();
};

class PersistentDataFlasher
{
public:
  bool maybe_load_data(PersistentData& data_out);
  void save(const PersistentData& curr_data);
  void force_save(const PersistentData& curr_data);

private:
  // Maybe the last saved data
  std::optional<PersistentData> maybe_last_saved_data_;
  // Time since last change in data
  std::optional<unsigned long> maybe_millis_when_data_has_last_changed_;
};

#endif  // FLASH_DATA_GUARD_H_