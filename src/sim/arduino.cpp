#include "sim/arduino.h"

#include <SDL.h>
#include <fstream>
#include <iostream>

SerialObject Serial;
EEPROMClass EEPROM;

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void pinMode(int pin, int input_output)
{
}

void digitalWrite(int pin, int input_output)
{
}

unsigned long millis()
{
  return static_cast<unsigned long>(SDL_GetTicks());
}

bool SerialObject::begin(int baudrate)
{
  return true;
}
void SerialObject::print(const char* text)
{
  std::cout << text << std::flush;
}
void SerialObject::print(const uint32_t number)
{
  std::cout << std::to_string(number) << std::flush;
}

void SerialObject::print(const int32_t number)
{
  std::cout << std::to_string(number) << std::flush;
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

void EEPROMClass::begin(size_t size)
{
  if (_data != nullptr)
  {
    delete[] _data;
  }
  _data = new uint8_t[size];
  _size = size;

  Serial.println("Reading to flash_data.bin...");
  std::ifstream fs("flash_data.bin", std::ios::in | std::ios::binary);
  fs.read(reinterpret_cast<char*>(_data), _size);
  fs.close();
}

uint8_t EEPROMClass::read(int const address)
{
  if (address < 0 || (size_t)address >= _size)
  {
    return 0;
  }
  if (!_data)
  {
    return 0;
  }

  return _data[address];
}

bool EEPROMClass::commit()
{
  Serial.println("Writting to flash_data.bin...");
  std::ofstream fs("flash_data.bin", std::ios::out | std::ios::binary);
  fs.write(reinterpret_cast<const char*>(_data), _size);
  fs.close();
  return true;
}

EEPROMClass::~EEPROMClass()
{
  delete[] _data;
}