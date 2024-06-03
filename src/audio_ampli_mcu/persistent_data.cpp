
#include "persistent_data.h"

#ifdef SIM
#include "sim/arduino.h"
#else
#include <EEPROM.h>
#include <Arduino.h>
#endif


#define DELAY_UNTIL_CHANGES_ARE_WRITTEN_TO_FLASH 15000  // [ms]


bool PersistentData::operator==(const PersistentData& rhs) const
{
  if (magic_num != rhs.magic_num
  || version_num != rhs.version_num
  || is_muted != rhs.is_muted
  || selected_audio_input != rhs.selected_audio_input
  || output_mode_value != rhs.output_mode_value
  || output_mode_value != rhs.output_mode_value
  )
  {
    return false;
  }
  for (size_t i = 0; i < NUM_AUDIO_INPUT; ++i)
  {
    if (per_audio_input_data[i].volume_db != rhs.per_audio_input_data[i].volume_db
    || per_audio_input_data[i].gain_value != rhs.per_audio_input_data[i].gain_value
    )
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

void PersistentDataFlasher::init()
{
  EEPROM.begin(/*size = */ sizeof(PersistentData));
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
  if (EEPROM.read(0) != 0xFE || EEPROM.read(1) != 0xCA || EEPROM.read(2) != VERSION_NUMBER)
  {
    Serial.println("Data in flash is either corrupted or invalid. It won't be restored.");
    return false;
  }
  EEPROM.get(0, data_out);

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
  Serial.println("Saving data to flash...");
  Serial.print("volume=");
  Serial.println(curr_data.get_volume_db());
  EEPROM.put(0, curr_data);
  EEPROM.commit();
  maybe_last_saved_data_ = curr_data;
  maybe_changed_data_ = {};
  maybe_time_since_last_change_to_data_ = {};
}