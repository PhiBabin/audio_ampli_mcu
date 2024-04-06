#ifndef __PIO_ENCODER_H__
#define __PIO_ENCODER_H__

#include "sim/arduino.h"

class PioEncoder{
    private:
        uint8_t pin;
        unsigned int sm;
        size_t pio;
        int max_step_rate;
        int flip_it;
    public:
        static unsigned int offset;
        static bool not_first_instance;
        
        PioEncoder(uint8_t _pin, size_t _pio = 0, unsigned int _sm = -1, int max_step_rate = 0, bool wflip=false);
        void begin();
        void reset();
        void flip(const bool x=true);
        int getCount();
};


#endif