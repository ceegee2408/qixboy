#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include <Arduboy2.h>
#include "config.h"

class vertex {
  public:
    // We only need 7 bits (0-127) for x and 6 bits (0-63) for y, 
    // so we pack them into 2 bytes for memory efficiency
    int position;
    
    vertex(int x, int y) {
      this->position = x * 64 + y;
    }
    
    vertex() {
      this->position = 0;
    }
    
    int getx() const {
      return position / 64;
    }

    int gety() const {
      return position % 64;
    }
    
    
    void addx(int x) {
      // Check if x movement would go out of bounds before applying
      int newX = getx() + x;
      if (newX >= 0 && newX < WIDTH) {
        position = newX * 64 + gety();
      } else {
        position = getx() * 64 + gety(); // No change if out of bounds
      }
    }
    
    void addy(int y) {
      // Check if y movement would go out of bounds before applying
      int newY = gety() + y;
      if (newY >= 0 && newY < HEIGHT) {
        position = getx() * 64 + newY;
      } else {
        position = getx() * 64 + gety(); // No change if out of bounds
      }
    }
};


#endif // TYPES_H
