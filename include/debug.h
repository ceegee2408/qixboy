#ifndef DEBUG_H
#define DEBUG_H

#include <Arduboy2.h>

extern Arduboy2 arduboy;

class vector;

enum debugType
{
  NONE,
  ERROR,
  WARNING,
  INFO,
  DETAILED
};

class Debug
{
public:
  debugType set = INFO; 
  void critical(const char *message, uint32_t *value = nullptr, debugType messageType = ERROR)
  {
    Serial.println("CRITICAL:");
    Serial.println(message);
    if (value != nullptr)
    {
      Serial.print("Value: ");
      Serial.println(*value);
    }
    while (true)
    {
      arduboy.clear();
      arduboy.setCursor(0, 0);
      arduboy.print(message);
      if (value != nullptr)
      {
        arduboy.setCursor(0, 10);
        arduboy.print("Value: ");
        arduboy.print(*value);
      }
      arduboy.display();
    }
  }
  void raise(const char *message, uint32_t *value = nullptr, debugType messageType = WARNING)
  {   
    if (set != NONE && set >= messageType)
    {
      Serial.println("################################");
      Serial.println("RAISE:");
      Serial.println(message);
      if (value != nullptr)
      {
        Serial.print("Value: ");
        Serial.println(*value);
      }
      Serial.println("################################");

    }
  }
  void log(const char *type, const char *message, debugType messageType = INFO)
  {
    if (set != NONE && set >= messageType)
    {
      Serial.print(type);
      Serial.print(": ");
      Serial.println(message);
    }
  }
  void log(const char *type, uint32_t value, debugType messageType = INFO)
  {
    if (set != NONE && set >= messageType)
    {
      Serial.print(type);
      Serial.print(": ");
      Serial.println(value);
    }
  }
  void logBitmap(const char *type, const byte value, debugType messageType = INFO) 
  {
    if (set != NONE && set >= messageType){
      Serial.print(type);
      Serial.print(": ");
      Serial.println(String(value, BIN));
    }
  }
  void printVector(const vector &v);
  void log (const char *type, const vector &v, debugType messageType = INFO)
  {
    if (set != NONE && set >= messageType)
    {
    Serial.print(type);
    Serial.print(": ");
    printVector(v);
    }
  }
};
Debug debug;
#endif