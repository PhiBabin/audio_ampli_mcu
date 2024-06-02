#include "persistent_data.h"

PersistentData::PerAudioInputData& PersistentData::get_per_audio_input_data_mutable()
{
  return per_audio_input_data[static_cast<uint8_t>(selected_audio_input)];
}
const PersistentData::PerAudioInputData& PersistentData::get_per_audio_input_data() const
{
  return per_audio_input_data[static_cast<uint8_t>(selected_audio_input)];
}
int32_t& PersistentData::get_volume_db_mutable()
{
  return get_per_audio_input_data_mutable().volume_db;
}
GainOption& PersistentData::get_gain_mutable()
{
  return get_per_audio_input_data_mutable().gain_value;
}

const int32_t& PersistentData::get_volume_db() const
{
  return get_per_audio_input_data().volume_db;
}
const GainOption& PersistentData::get_gain() const
{
  return get_per_audio_input_data().gain_value;
}

bool PersistentDataFlasher::maybe_load_data(PersistentData& data_out)
{
  return false;
}

void PersistentDataFlasher::save(const PersistentData& curr_data)
{
}

void PersistentDataFlasher::force_save(const PersistentData& curr_data)
{
}