#ifndef ARDUINO_GUARD_H_
#define ARDUINO_GUARD_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stddef.h>

using pin_size_t = size_t;

long map(long x, long in_min, long in_max, long out_min, long out_max);

// #define min(a,b) ((a)<(b)?(a):(b))
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

void pinMode(int pin, int input_output);
void digitalWrite(int pin, int input_output);

unsigned long millis();

#define HIGH 0x1
#define LOW 0x0

#define INPUT 0x0
#define OUTPUT 0x1

class SerialObject
{
public:
  bool begin(int baudrate);
  void print(const char* text);
  void print(const int32_t number);
  void print(const uint32_t number);
  void println(const char* text);
  void println(const int32_t number);
  void println(const uint32_t number);
};
extern SerialObject Serial;
#endif