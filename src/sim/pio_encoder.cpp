#include "sim/pio_encoder.h"


int count[40] = {0};


void increment_encoder(const uint8_t encoder_pin, const int increment)
{
    if (encoder_pin < 40)
    {
        count[encoder_pin] += increment;
    }
}

void decrement_encoder(const uint8_t encoder_pin, const int increment)
{
    if (encoder_pin < 40)
    {
        count[encoder_pin] -= increment;
    }
}

PioEncoder::PioEncoder(uint8_t _pin, size_t _pio, unsigned int _sm, int max_step_rate, bool wflip): pin(_pin)
{
}

void PioEncoder::begin()
{
}

void PioEncoder::reset()
{
}

void PioEncoder::flip(const bool x)
{
}

int PioEncoder::getCount(){
    return count[pin];
}