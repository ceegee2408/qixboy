#ifndef ENTITIES_H
#define ENTITIES_H

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "geometry.h"
#include "gamestate.h"
// Sprite frame counts
#include "sprites.h"

class player {
  private:
    // Packed data to save RAM
    byte directionData = 0; // lower 4 bits: activeDir, upper 4 bits: lastTrailDir
    uint16_t perimIndexPacked = (1 << 6) | 2; // lower 6 bits: index[0], next 6 bits: index[1]
    uint16_t drawStartIndexPacked = 0; // lower 6 bits: index[0], next 6 bits: index[1]
    uint16_t drawEndIndexPacked = 0; // lower 6 bits: index[0], next 6 bits: index[1]

  public:
    byte data = NUM_LIVES << 6; // upper 2 bits for lives, bit 2 for stationary flag
    byte allowedMoves = 0x03; // bit 0-3 for left, right, up, down; bit 4 for fast move, bit 5 for slow move
    byte prevInput = 0; // previous frame input
    vertex position;
    vertex trail[MAX_VERTICES / 4]; // store trail vertices for filling
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

    bool stationary() {
      return (data & 0x04) != 0;
    }

    void setStationary(bool stationary) {
      if (stationary) data |= 0x04;
      else data &= ~0x04;
    }

    byte getLives() {
      return data >> 6;
    }

    void loseLife() {
      initializeDeathAnimation();
      byte lives = getLives();
      if (lives > 0) {
        lives--;
        data = (data & 0x3F) | (lives << 6);
      } else {
        gameState = GAME_OVER;
      }
    }

    void addTrailVertex(vertex v) {
      if (trailCount < MAX_VERTICES / 4) {
        trail[trailCount] = v;
        trailCount++;
      }
    }

    // Frames since last movement. Increment with `tickFrame()` each game frame;
    // call `noteMoved()` from movement code when the player actually moves.
    uint16_t framesSinceMove = 0;
    inline void noteMoved() { framesSinceMove = 0; }
    inline void tickFrame() { framesSinceMove++; }

    // Helper: return true when currently in draw mode and have been idle
    // for at least `threshold` frames.
    inline bool isInDrawModeAndIdle(uint16_t threshold) {
      return ((allowedMoves & 0x30) != 0) && (framesSinceMove >= threshold);
    }

    void respawn() {
      position = vertex(WIDTH / 2, HEIGHT - 1);
      trailCount = 0;
      allowedMoves = 0x03; // reset to allow perimeter movement
      setStationary(false);
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

    // Remove redundant collinear vertices (axis-aligned geometry only).
    // A vertex is redundant when its predecessor and successor lie on the
    // same horizontal or vertical line as itself.  The minimum of 4
    // vertices (rectangle) is preserved.
    void removeCollinear() {
      int i = 0;
      while (i < vertexCount && vertexCount > 4) {
        int prev = (i - 1 + vertexCount) % vertexCount;
        int next = (i + 1) % vertexCount;
        vertex vPrev = vertices[prev];
        vertex vCurr = vertices[i];
        vertex vNext = vertices[next];
        bool collinear =
          (vPrev.getx() == vCurr.getx() && vCurr.getx() == vNext.getx()) ||
          (vPrev.gety() == vCurr.gety() && vCurr.gety() == vNext.gety());
        if (collinear) {
          removeVertex(i);
          // Re-check position i — it now holds the old i+1 vertex,
          // which may itself be collinear with its new neighbours.
        } else {
          i++;
        }
      }
    }
    void reset() {
      vertexCount = 4;
      vertices[0] = vertex(0, 0);
      vertices[1] = vertex(0, HEIGHT - 1);
      vertices[2] = vertex(WIDTH - 1, HEIGHT - 1);
      vertices[3] = vertex(WIDTH - 1, 0);
    }
};

class qix {
  public:
    vertex position;
    int speed = 1;
    
    qix() {
      position = vertex(WIDTH / 2, HEIGHT / 2);
    }
};

// Declare global player instance (defined in main.cpp)
extern player p;
// Forward-declare fuze so other translation units can reference the global
class fuze;
// Declare global fuze instance (defined in main.cpp)
extern fuze fz;

// Forward-declare rendering function needed inside fuze::update()
void restoreFuzeBackground();

class fuze {
  public:
    bool active = false;
    vertex position;
    byte trailIndex = 0;
    int speed = FUZE_MOVE_INTERVAL;
    byte frame = 0; // tick counter for animation
    byte moveTick = 0; // tick counter for movement gating
    fuze() {
      position = vertex(WIDTH / 2, HEIGHT / 2);
    }
    void begin() {
      active = true;
      frame = 0;
      if (hasResumePos) {
        position = resumePos;
        trailIndex = resumeTrailIndex;
        hasResumePos = false;
      } else {
        trailIndex = 0;
        if (p.trailCount > 0) position = p.trail[0];
      }
    }
    void update() {
      if (!active) return;
      if (!(p.allowedMoves & 0x30)) {
        active = false;
        return;
      }
      if (p.trailCount < 2) {
        active = false;
        return;
      }
      if (p.framesSinceMove < (uint16_t)SLOW_MOVE) {
        return;
      }
      frame++;
      if (frame >= (FUZE_FRAME_TICKS * FUZE_FRAME_COUNT)) frame = 0;
      moveTick++;
      bool doMove = false;
      if (moveTick >= (uint8_t)speed) { moveTick = 0; doMove = true; }
      bool hasNext = false;
      vertex nextTarget = position;
      if (trailIndex + 1 < p.trailCount) {
        nextTarget = p.trail[trailIndex + 1];
        hasNext = true;
      } else if (trailIndex + 1 == p.trailCount) {
        nextTarget = p.position;
        hasNext = true;
      }
      if (hasNext && compareVertices(position, nextTarget)) {
        trailIndex++;
        if (trailIndex >= p.trailCount) {
          active = false;
          p.loseLife();
          return;
        }
      }

      if (hasNext && doMove) {
        switch (getDirection(position, nextTarget)) {
          case 0x01: position.addx(-1); break; // left
          case 0x02: position.addx(1); break;  // right
          case 0x04: position.addy(-1); break; // up
          case 0x08: position.addy(1); break;  // down
        }
      }
    }
    void render();
    // If the fuze was interrupted by player movement, we store the
    // previous fuze position here so it can resume instantly later.
    vertex resumePos;
    bool hasResumePos = false;
    byte resumeTrailIndex = 0;
};

class sparx {
  public:
    vertex position;
    // data byte: bit 0 for normal/super sparx, bit 1 for clockwise/counterclockwise, 
    // bit 2 for horizontal/vertical, bit 3-4 for speed (00=slowest, 11=fastest)
    byte data = 0b0000;
    byte frame = 0; // animation frame index for rendering
    sparx() {
      position = vertex(WIDTH / 2, 0);
      data = 0;
      frame = 0;
    }
    sparx(bool isSuper, bool clockwise, bool horizontal, byte speedLevel) {
      position = vertex(WIDTH / 2, 0);
      data = (isSuper ? 0x01 : 0x00) |
             (clockwise ? 0x02 : 0x00) |
             (horizontal ? 0x04 : 0x00) |
             (speedLevel << 3);
      frame = 0;
    }
    int getSpeed() {
      switch ((data >> 3) & 0x03) {
        case 0: return SLOW_MOVE;
        case 1: return FAST_MOVE;
        case 2: return (int)(FAST_MOVE * 1.5 + 0.5);
        case 3: return FAST_MOVE * 2;
      }
    }
};
#endif // ENTITIES_H
