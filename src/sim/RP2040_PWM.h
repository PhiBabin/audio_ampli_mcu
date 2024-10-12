#ifndef __RP2040_PWM_H__
#define __RP2040_PWM_H__

#include "sim/arduino.h"

class RP2040_PWM
{
public:
  RP2040_PWM(const uint8_t& pin, const float& frequency, const float dutycycle, bool phaseCorrect = false);
  bool setPWM(const uint8_t& pin, const float& frequency, const float dutycycle, bool phaseCorrect = false);

private:
  float duty_cycle_;
};
#endif