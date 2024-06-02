#ifndef CONFIG_OPTION_GUARD_H_
#define CONFIG_OPTION_GUARD_H_

#include "audio_input_enums.h"

// Default volume
#define STARTUP_VOLUME_DB -40

// Number of tick per rotation of the encoder
#define ENCODER_TICK_PER_ROTATION 24

// How many encoder tick is required to go from -63db to 0db
#define TOTAL_TICK_FOR_FULL_VOLUME (9 * ENCODER_TICK_PER_ROTATION)

// Number of encoder tick to change audio input
#define TICK_PER_AUDIO_IN (ENCODER_TICK_PER_ROTATION / NUM_AUDIO_INPUT)

// Changing the version will make previously saved settings unusable
#define VERSION_NUMBER 1

#endif  //CONFIG_OPTION_GUARD_H_