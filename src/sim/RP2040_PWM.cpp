
#include "sim/RP2040_PWM.h"

#include <iostream>

RP2040_PWM::RP2040_PWM(const uint8_t& pin, const float& frequency, const float dutycycle, bool phaseCorrect)
  : duty_cycle_(dutycycle)
{
}
bool RP2040_PWM::setPWM(const uint8_t& pin, const float& frequency, const float dutycycle, bool phaseCorrect)
{
  if (dutycycle != duty_cycle_)
  {
    std::cout << "Changed duty cycle from " << duty_cycle_ << " to " << dutycycle << std::endl;
  }
  duty_cycle_ = dutycycle;
  return true;
}