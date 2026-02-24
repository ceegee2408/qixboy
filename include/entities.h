#ifndef ENTITIES_H
#define ENTITIES_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

class player {
  private:
    // Packed data to save RAM
    byte directionData = 0; // lower 4 bits: activeDir, upper 4 bits: lastTrailDir
    uint16_t perimIndexPacked = (1 << 6) | 2; // lower 6 bits: index[0], next 6 bits: index[1]
    uint16_t drawStartIndexPacked = 0; // lower 6 bits: index[0], next 6 bits: index[1]
    uint16_t drawEndIndexPacked = 0; // lower 6 bits: index[0], next 6 bits: index[1]

  public:
    byte data = NUM_LIVES * 4; // upper 2 bits for lives
    byte allowedMoves = 0x03; // bit 0-3 for left, right, up, down; bit 4 for fast move, bit 5 for slow move
    byte prevInput = 0; // previous frame input
    vertex position;
    vertex trail[MAX_VERTICES / 2]; // store trail vertices for filling
    byte trailCount = 0;

    player() {
      // Starting position
      position = vertex(WIDTH / 2, HEIGHT - 1);
    }

    // Direction data accessors
    inline byte getActiveDir() { return directionData & 0x0F; }
    inline void setActiveDir(byte dir) { directionData = (directionData & 0xF0) | (dir & 0x0F); }
    inline byte getLastTrailDir() { return (directionData >> 4) & 0x0F; }
    inline void setLastTrailDir(byte dir) { directionData = (directionData & 0x0F) | ((dir & 0x0F) << 4); }

    // Perimeter index accessors
    inline byte getPerimIndex(byte i) { return (i == 0) ? (perimIndexPacked & 0x3F) : ((perimIndexPacked >> 6) & 0x3F); }
    inline void setPerimIndex(byte i, byte val) {
      if (i == 0) perimIndexPacked = (perimIndexPacked & 0xFFC0) | (val & 0x3F);
      else perimIndexPacked = (perimIndexPacked & 0x003F) | ((val & 0x3F) << 6);
    }

    // Draw start index accessors
    inline byte getDrawStartIndex(byte i) { return (i == 0) ? (drawStartIndexPacked & 0x3F) : ((drawStartIndexPacked >> 6) & 0x3F); }
    inline void setDrawStartIndex(byte i, byte val) {
      if (i == 0) drawStartIndexPacked = (drawStartIndexPacked & 0xFFC0) | (val & 0x3F);
      else drawStartIndexPacked = (drawStartIndexPacked & 0x003F) | ((val & 0x3F) << 6);
    }

    // Draw end index accessors
    inline byte getDrawEndIndex(byte i) { return (i == 0) ? (drawEndIndexPacked & 0x3F) : ((drawEndIndexPacked >> 6) & 0x3F); }
    inline void setDrawEndIndex(byte i, byte val) {
      if (i == 0) drawEndIndexPacked = (drawEndIndexPacked & 0xFFC0) | (val & 0x3F);
      else drawEndIndexPacked = (drawEndIndexPacked & 0x003F) | ((val & 0x3F) << 6);
    }

    void addTrailVertex(vertex v) {
      if (trailCount < MAX_VERTICES / 2) {
        trail[trailCount] = v;
        trailCount++;
      }
    }
};

class sparx {
  public:
    vertex position;
    // data byte: bit 0 for normal/super sparx, bit 1 for clockwise/counterclockwise, 
    // bit 2 for horizontal/vertical, bit 3-4 for speed (00=slowest, 11=fastest)
    byte data = 1;
    
    sparx() {
      position = vertex(WIDTH / 2, HEIGHT / 2);
    }
};

class perimeter {
  public:
    vertex vertices[MAX_VERTICES];
    int vertexCount = 4;
    
    perimeter() {
      vertices[0] = vertex(0, 0);
      vertices[1] = vertex(0, HEIGHT - 1);
      vertices[2] = vertex(WIDTH - 1, HEIGHT - 1);
      vertices[3] = vertex(WIDTH - 1, 0);
    }
    
    bool addVertex(vertex v, int index) {
      if (vertexCount < MAX_VERTICES && index >= 0 && index <= vertexCount) {
        // Shift vertices forward from index
        for (int i = vertexCount; i > index; i--) {
          vertices[i] = vertices[i - 1];
        }
        vertices[index] = v;
        vertexCount++;
        return true;
      }
      return false;  // array full or invalid index
    }
    
    void removeVertex(int index) {
      if (index >= 0 && index < vertexCount) {
        // Shift vertices backward from index
        for (int i = index; i < vertexCount - 1; i++) {
          vertices[i] = vertices[i + 1];
        }
        vertexCount--;
      }
    }
};

class qix {
  public:
    vertex position;
    vertex segmentA;
    vertex segmentB;
    int speed = 2;
    uint16_t phase = 0;
    
    qix() {
      position = vertex(WIDTH / 2, HEIGHT / 2);
      segmentA = vertex(WIDTH / 2 - 5, HEIGHT / 2 - 3);
      segmentB = vertex(WIDTH / 2 + 5, HEIGHT / 2 + 3);
    }
};

#endif // ENTITIES_H
