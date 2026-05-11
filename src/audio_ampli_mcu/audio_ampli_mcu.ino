/// Version of each external libraries (use Library Manager to install them):
/// - Raspberry Pi Pico/RP2040: 3.7.2
/// - rp2040-encoder-library: 0.1.2
/// - InputDebounce: 1.6.0
/// - MCP23S17: 0.8.0
/// - RP2040_PWM: 1.7.0
/// - IRemote: 4.4.1

#include "app.h"

App app;

void setup()
{
  app.init();
}

void loop()
{
  app.tick();
}
