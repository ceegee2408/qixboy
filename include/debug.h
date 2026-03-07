#ifndef DEBUG_H
#define DEBUG_H

#include "Arduboy2.h"

extern Arduboy2 arduboy;

class vertex;

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
  void critical(const char *message, int value = NULL, debugType messageType = ERROR)
  {
    Serial.println("CRITICAL:");
    Serial.println(message);
    if (value != NULL)
    {
      Serial.print("Value: ");
      Serial.println(value);
    }
    while (true)
    {
      arduboy.clear();
      arduboy.setCursor(0, 0);
      arduboy.print(message);
      if (value != NULL)
      {
        arduboy.setCursor(0, 10);
        arduboy.print("Value: ");
        arduboy.print(value);
      }
      arduboy.display();
    }
  }
  void raise(const char *message, int value = NULL, debugType messageType = WARNING)
  {   
    if (set != NONE && set >= messageType)
    {
      Serial.println("################################");
      Serial.println("RAISE:");
      Serial.println(message);
      if (value != NULL)
      {
        Serial.print("Value: ");
        Serial.println(value);
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
  void log(const char *type, int value, debugType messageType = INFO)
  {
    if (set != NONE && set >= messageType)
    {
      Serial.print(type);
      Serial.print(": ");
      Serial.println(value);
    }
  }
  void printVertex(const vertex &v);
  void log (const char *type, const vertex &v, debugType messageType = INFO)
  {
    if (set != NONE && set >= messageType)
    {
    Serial.print(type);
    Serial.print(": ");
    printVertex(v);
    }
  }
};
Debug debug;
#endif