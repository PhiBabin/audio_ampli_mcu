#include "sim/arduino.h"

#include <iostream>

 SerialObject Serial;

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void pinMode(int pin, int input_output)
{}

void digitalWrite(int pin, int input_output)
{}


unsigned long millis()
{
    return 0;
}


bool SerialObject::begin(int baudrate)
{
    return true;
}
void SerialObject::print(const char* text)
{
    std::cout << text;
}
void SerialObject::print(const uint32_t number)
{
    std::cout << std::to_string(number);
}

void SerialObject::print(const int32_t number)
{
    std::cout << std::to_string(number);
}
void SerialObject::println(const uint32_t number)
{
    std::cout << std::to_string(number) << std::endl;
}

void SerialObject::println(const int32_t number)
{
    std::cout << std::to_string(number) << std::endl;
}

void SerialObject::println(const char* text)
{
    std::cout << text << std::endl;
}