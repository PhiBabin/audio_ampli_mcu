#ifndef CONFIG_OPTION_GUARD_H_
#define CONFIG_OPTION_GUARD_H_

#include "audio_input_enums.h"

// Default volume
#define STARTUP_VOLUME_DB -20

// Number of tick per rotation of the encoder
#define ENCODER_TICK_PER_ROTATION 24

// How many encoder tick is required to go from -63db to 0db
#define TOTAL_TICK_FOR_FULL_VOLUME (9 * ENCODER_TICK_PER_ROTATION)

// Number of encoder tick to change audio input
#define TICK_PER_AUDIO_IN (ENCODER_TICK_PER_ROTATION / NUM_AUDIO_INPUT)

// Changing the version will make previously saved settings unusable
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
// Change to the patch will not impact save settings
#define PATCH_VERSION 1

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define VERSION_STRING "V" STR(MAJOR_VERSION) "." STR(MINOR_VERSION) "." STR(PATCH_VERSION)

// V2 PCB has some hence features (such as power on/off, subwoofer and different volume for left/right)
#define USE_V2_PCB

// IR remote pin, the IRremote library requires this to be set via a #define
#define IR_RECEIVE_PIN 6  // GP6

#define BUTTON_DEBOUNCE_DELAY 20  // [ms]

#endif  // CONFIG_OPTION_GUARD_H_