#ifndef ENTITIES_H
#define ENTITIES_H

#include <Arduboy2.h>
#include "geometry.h"
#include "config.h"
#include "gamestate.h"
#include "sprites.h"


extern Arduboy2 arduboy;
extern uint16_t frameCounter;

class vertex {
  public:
    byte x;
    byte y;
    
    vertex(int x, int y) {
      this->x = x;
      this->y = y;
    }
    
    vertex() {
      this->x = 0;
      this->y = 0;
    }
};

class player {
  private:
    // Packed data to save RAM
    byte directionData = 0; // lower 4 bits: activeDir, upper 4 bits: lastTrailDir
    uint16_t perimIndexPacked = (1 << 8) | 2; // lower 8 bits: index[0], upper 8 bits: index[1]
    uint16_t drawStartIndexPacked = 0; // lower 8 bits: index[0], upper 8 bits: index[1]
    uint16_t drawEndIndexPacked = 0; // lower 8 bits: index[0], upper 8 bits: index[1]

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
    inline byte getPerimIndex(byte i) { return (i == 0) ? (perimIndexPacked & 0x00FF) : ((perimIndexPacked >> 8) & 0x00FF); }
    inline void setPerimIndex(byte i, byte val) {
      if (i == 0) perimIndexPacked = (perimIndexPacked & 0xFF00) | (val & 0xFF);
      else perimIndexPacked = (perimIndexPacked & 0x00FF) | ((val & 0xFF) << 8);
    }

    // Draw start index accessors
    inline byte getDrawStartIndex(byte i) { return (i == 0) ? (drawStartIndexPacked & 0x00FF) : ((drawStartIndexPacked >> 8) & 0x00FF); }
    inline void setDrawStartIndex(byte i, byte val) {
      if (i == 0) drawStartIndexPacked = (drawStartIndexPacked & 0xFF00) | (val & 0xFF);
      else drawStartIndexPacked = (drawStartIndexPacked & 0x00FF) | ((val & 0xFF) << 8);
    }

    // Draw end index accessors
    inline byte getDrawEndIndex(byte i) { return (i == 0) ? (drawEndIndexPacked & 0x00FF) : ((drawEndIndexPacked >> 8) & 0x00FF); }
    inline void setDrawEndIndex(byte i, byte val) {
      if (i == 0) drawEndIndexPacked = (drawEndIndexPacked & 0xFF00) | (val & 0xFF);
      else drawEndIndexPacked = (drawEndIndexPacked & 0x00FF) | ((val & 0xFF) << 8);
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

    void eraseTrail() {
      // erase from buffer just to make collision checks work correctly during qix movement
      for (int i = 0; i < trailCount; i++) {
        int x = trail[i].x;
        int y = trail[i].y;
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
          arduboy.drawPixel(x, y, BLACK);
        }
      }
    }
    
    void drawTrail() {
      for (int i = 0; i < trailCount; i++) {
        int x = trail[i].x;
        int y = trail[i].y;
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
          arduboy.drawPixel(x, y, WHITE);
        }
      }
    }

    bool isInDrawMode() {
      return (allowedMoves & 0x30) != 0;
    }

    // Frame number when the player last moved. Used with the global
    // frameCounter to compute idle time without a per-frame tick call.
    uint16_t lastMoveFrame = 0;
    inline void noteMoved() { lastMoveFrame = frameCounter; }

    // Compute frames elapsed since the player last moved, handling
    // the 0-254 wraparound of frameCounter.
    inline uint16_t getFramesSinceMove() {
      if (frameCounter >= lastMoveFrame)
        return frameCounter - lastMoveFrame;
      return (255 - lastMoveFrame) + frameCounter;
    }

    // Helper: return true when currently in draw mode and have been idle
    // for at least `threshold` frames.
    inline bool isInDrawModeAndIdle(uint16_t threshold) {
      return ((allowedMoves & 0x30) != 0) && (getFramesSinceMove() >= threshold);
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
          (vPrev.x == vCurr.x && vCurr.x == vNext.x) ||
          (vPrev.y == vCurr.y && vCurr.y == vNext.y);
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
// Declare global player instance (defined in main.cpp)
extern player p;
// Forward-declare fuze so other translation units can reference the global
class fuze;
// Declare global fuze instance (defined in main.cpp)
extern fuze fz;

extern bool doLinesIntersect(vertex p1, vertex p2, vertex p3, vertex p4);

class qix {
  public:
    // Current endpoints
    vertex p1, p2;
    // Velocities for each endpoint
    int v1x, v1y;
    int v2x, v2y;

    // History for trailing effect
    static const int QIX_HISTORY = 4;
    vertex hist1[QIX_HISTORY];
    vertex hist2[QIX_HISTORY];
    int histIdx = 0;
    // Movement throttling     
    const int maxSpeed = 3;
    int moveTick = 0;
    int moveInterval = 2;

    // Rendering throttling: how often the qix is actually redrawn
    // (movement can still update more frequently). Adjustable.
    int renderTick = 0;
    int renderInterval = 4; // draw every N frames

    // Return true when it's time to render (increments internal tick)
    inline bool shouldRender() {
      renderTick++;
      if (renderTick >= renderInterval) {
        renderTick = 0;
        return true;
      }
      return false;
    }

    qix() {
      p1 = vertex(WIDTH / 3, HEIGHT / 3);
      p2 = vertex((WIDTH * 2) / 3, (HEIGHT * 2) / 3);
      // Deliberately asymmetric so the pattern is never periodic/boring
      v1x = constrain(v1x + (int)random(-1, 2), -maxSpeed, maxSpeed); v1y = constrain(v1y + (int)random(-1, 2), -maxSpeed, maxSpeed);
      v2x = constrain(v2x + (int)random(-1, 2), -maxSpeed, maxSpeed); v2y = constrain(v2y + (int)random(-1, 2), -maxSpeed, maxSpeed);
      // Initialize history
      for (int i = 1; i < QIX_HISTORY; i++) {
        hist1[i] = vertex(0, 0);
        hist2[i] = vertex(0, 0);
      }
      hist1[0] = p1;
      hist2[0] = p2;
      histIdx = 1;
    }

    void update() {
      // Slow movement by only updating positions every `moveInterval` frames
      moveTick++;
      if (moveTick < moveInterval) return;
      moveTick = 0;
      const int maxDist = 20; // soft maximum Manhattan distance between endpoints

      // Randomly nudge velocities within a max speed
      v1x = constrain(v1x + (int)random(-1, 2), -maxSpeed, maxSpeed);
      v1y = constrain(v1y + (int)random(-1, 2), -maxSpeed, maxSpeed);
      v2x = constrain(v2x + (int)random(-1, 2), -maxSpeed, maxSpeed);
      v2y = constrain(v2y + (int)random(-1, 2), -maxSpeed, maxSpeed);

      // Avoid zero velocity on both axes (Qix gets stuck)
      if (v1x == 0 && v1y == 0) v1x = 1;
      if (v2x == 0 && v2y == 0) v2y = 1;

      // Soft pull to keep endpoints from drifting too far apart
      int dx = p2.x - p1.x;
      int dy = p2.y - p1.y;
      int dist = abs(dx) + abs(dy);
      if (dist > maxDist) {
        if (dx > 0) { v1x++; v2x--; } else { v1x--; v2x++; }
        if (dy > 0) { v1y++; v2y--; } else { v1y--; v2y++; }
        v1x = constrain(v1x, -maxSpeed, maxSpeed);
        v1y = constrain(v1y, -maxSpeed, maxSpeed);
        v2x = constrain(v2x, -maxSpeed, maxSpeed);
        v2y = constrain(v2y, -maxSpeed, maxSpeed);
      }

      // Compute tentative next positions
      int nx1 = p1.x + v1x;
      int ny1 = p1.y + v1y;
      int nx2 = p2.x + v2x;
      int ny2 = p2.y + v2y;

      // If an endpoint would move onto a white pixel, invert its velocity
      if (nx1 >= 0 && nx1 < WIDTH && ny1 >= 0 && ny1 < HEIGHT) {
        if (arduboy.getPixel(nx1, ny1)) {
          v1x = -v1x;
          v1y = -v1y;
          nx1 = p1.x + v1x;
          ny1 = p1.y + v1y;
        }
      }
      if (nx2 >= 0 && nx2 < WIDTH && ny2 >= 0 && ny2 < HEIGHT) {
        if (arduboy.getPixel(nx2, ny2)) {
          v2x = -v2x;
          v2y = -v2y;
          nx2 = p2.x + v2x;
          ny2 = p2.y + v2y;
        }
      }

      // Bounce on playfield bounds (simple AABB)
      if (nx1 <= 1 || nx1 >= WIDTH - 2) v1x = -v1x;
      if (ny1 <= 1 || ny1 >= HEIGHT - 2) v1y = -v1y;
      if (nx2 <= 1 || nx2 >= WIDTH - 2) v2x = -v2x;
      if (ny2 <= 1 || ny2 >= HEIGHT - 2) v2y = -v2y;

      // Apply (clamped) moves after any bounce adjustments
      nx1 = constrain(p1.x + v1x, 1, WIDTH - 2);
      ny1 = constrain(p1.y + v1y, 1, HEIGHT - 2);
      nx2 = constrain(p2.x + v2x, 1, WIDTH - 2);
      ny2 = constrain(p2.y + v2y, 1, HEIGHT - 2);

      p1 = vertex(nx1, ny1);
      p2 = vertex(nx2, ny2);
    }

    void recordHistory() {
      hist1[histIdx] = p1;
      hist2[histIdx] = p2;
      histIdx = (histIdx + 1) % QIX_HISTORY;
    }

    bool isCollidingWithPlayer() {

      // Check each consecutive trail segment
      if (p.trailCount >= 2) {
        for (int i = 0; i + 1 < p.trailCount; i++) {
          if (doLinesIntersect(p.trail[i], p.trail[i + 1], p1, p2)) return true;
        }
      }
      // Also check the open segment from the last trail vertex to the
      // player's current position (this is the in-progress segment).
      if (doLinesIntersect(p.trail[p.trailCount - 1], p.position, p1, p2)) return true;

      return false;
    }

    vertex center() const {
      return vertex((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
    }
};



// Forward-declare rendering functions needed inside fuze
enum BgSlot : byte;
void restoreBackground(BgSlot slot);
bool compareVertices(vertex v1, vertex v2);
byte getDirection(vertex from, vertex to);

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
      if (p.getFramesSinceMove() < (uint16_t)SLOW_MOVE) {
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
          case 0x01: position.x = -1; break; // left
          case 0x02: position.x = 1; break;  // right
          case 0x04: position.y = -1; break; // up
          case 0x08: position.y = 1; break;  // down
        }
      }
    }
    void render();
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
