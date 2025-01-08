
#include "persistent_data.h"

#ifdef SIM
#include "sim/arduino.h"
#else
#include <Arduino.h>
#include <EEPROM.h>
#endif

#define DELAY_UNTIL_CHANGES_ARE_WRITTEN_TO_FLASH 15000  // [ms]

bool PersistentData::operator==(const PersistentData& rhs) const
{
  if (
    magic_num != rhs.magic_num || version_num != rhs.version_num || is_muted != rhs.is_muted ||
    selected_audio_input != rhs.selected_audio_input || output_mode_value != rhs.output_mode_value ||
    output_mode_value != rhs.output_mode_value || output_type_value != rhs.output_type_value ||
    sufwoofer_enable_value != rhs.sufwoofer_enable_value || bias != rhs.bias)
  {
    return false;
  }
  for (size_t i = 0; i < NUM_INPUT_OUTPUT_PERMUTATION; ++i)
  {
    if (
      per_audio_input_output_data[i].volume_db != rhs.per_audio_input_output_data[i].volume_db ||
      per_audio_input_output_data[i].gain_value != rhs.per_audio_input_output_data[i].gain_value)
    {
      return false;
    }
  }
  return true;
}

bool PersistentData::operator!=(const PersistentData& rhs) const
{
  return !(*this == rhs);
}

size_t PersistentData::current_input_output_pair_index() const
{
  return static_cast<size_t>(selected_audio_input) * NUM_OUTPUT_MODE * NUM_OUTPUT_TYPE +
         static_cast<size_t>(output_mode_value) * NUM_OUTPUT_TYPE + static_cast<size_t>(output_type_value);
}

PersistentData::PerAudioInputOutputData& PersistentData::get_per_audio_input_output_data_mutable()
{
  return per_audio_input_output_data[current_input_output_pair_index()];
}
const PersistentData::PerAudioInputOutputData& PersistentData::get_per_audio_input_output_data() const
{
  return per_audio_input_output_data[current_input_output_pair_index()];
}
int32_t& PersistentData::get_volume_db_mutable()
{
  return get_per_audio_input_output_data_mutable().volume_db;
}
GainOption& PersistentData::get_gain_mutable()
{
  return get_per_audio_input_output_data_mutable().gain_value;
}

void PersistentDataFlasher::init()
{
  EEPROM.begin(/*size = */ sizeof(PersistentData));
}

const int32_t& PersistentData::get_volume_db() const
{
  return get_per_audio_input_output_data().volume_db;
}
const GainOption& PersistentData::get_gain() const
{
  return get_per_audio_input_output_data().gain_value;
}

uint8_t PersistentData::compute_checksum() const
{
  uint8_t checksum = 0;
  const uint8_t* data_out_ptr = reinterpret_cast<const uint8_t*>(this);
  // Skip checksum bits
  for (size_t i = 2; i < sizeof(PersistentData); ++i)
  {
    checksum += data_out_ptr[i];
  }
  return checksum;
}

bool PersistentDataFlasher::maybe_load_data(PersistentData& data_out)
{
  EEPROM.get(0, data_out);
  if (data_out.checksum != data_out.compute_checksum())
  {
    Serial.println("Data in flash has the wrong checksum. It won't be restored.");
    return false;
  }
  if (data_out.magic_num != 0xCAFE || data_out.version_num != VERSION_NUMBER)
  {
    Serial.println("Data in flash is either corrupted or invalid. It won't be restored.");
    return false;
  }

  // Copy what we loaded
  maybe_last_saved_data_ = data_out;
  return true;
}

void PersistentDataFlasher::save(const PersistentData& curr_data)
{
  // If current data matches the last saved data. Reset everything.
  if (!maybe_last_saved_data_ || curr_data == static_cast<const PersistentData&>(*maybe_last_saved_data_))
  {
    maybe_changed_data_ = {};
    maybe_time_since_last_change_to_data_ = {};
    return;
  }
  // If the current data is new, start a timer. Everytime the data change, reset the timer
  if (!maybe_changed_data_ || curr_data != *maybe_changed_data_)
  {
    Serial.println("Persistent data changed.");
    maybe_changed_data_ = curr_data;
    maybe_time_since_last_change_to_data_ = millis();
    return;
  }

  // If the timer has elapse, write the data to flash
  if (millis() - *maybe_time_since_last_change_to_data_ > DELAY_UNTIL_CHANGES_ARE_WRITTEN_TO_FLASH)
  {
    force_save(curr_data);
  }
}

void PersistentDataFlasher::force_save(const PersistentData& curr_data)
{
  auto curr_data_copy = curr_data;
  Serial.println("Saving data to flash...");
  Serial.print("volume=");
  Serial.println(curr_data_copy.get_volume_db());

  // Update checksum
  curr_data_copy.checksum = curr_data_copy.compute_checksum();

  EEPROM.put(0, curr_data_copy);
  EEPROM.commit();
  maybe_last_saved_data_ = curr_data_copy;
  maybe_changed_data_ = {};
  maybe_time_since_last_change_to_data_ = {};
}