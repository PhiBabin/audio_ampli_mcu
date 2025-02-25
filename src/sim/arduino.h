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
int digitalRead(int pin);
void digitalWrite(int pin, int input_output);
void delayMicroseconds(const unsigned us);

unsigned long millis();
void delay(const int ms);

#define HIGH 0x1
#define LOW 0x0

#define INPUT 0x0
#define OUTPUT 0x1

class SerialObject
{
public:
  bool begin(int baudrate);
  void print(const char* text);
  void print(const int number);
  // void print(const uint32_t number);
  void println(const char* text);
  void println(const int number);
  // void println(const uint32_t number);
};

class EEPROMClass
{
public:
  void begin(size_t size);
  uint8_t read(int const address);
  bool commit();
  ~EEPROMClass();

  template <typename T>
  T& get(int const address, T& t)
  {
    if (address < 0 || address + sizeof(T) > _size)
    {
      return t;
    }

    memcpy((uint8_t*)&t, _data + address, sizeof(T));
    return t;
  }

  template <typename T>
  const T& put(int const address, const T& t)
  {
    if (address < 0 || address + sizeof(T) > _size)
    {
      return t;
    }
    if (memcmp(_data + address, (const uint8_t*)&t, sizeof(T)) != 0)
    {
      memcpy(_data + address, (const uint8_t*)&t, sizeof(T));
    }

    return t;
  }

private:
  uint8_t* _data = nullptr;
  size_t _size = 0;
};

extern SerialObject Serial;
extern EEPROMClass EEPROM;
#endif