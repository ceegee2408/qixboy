#ifndef ENTITIES_H
#define ENTITIES_H

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "geometry.h"
#include "gamestate.h"
// Sprite frame counts
#include "sprites.h"

// Access Arduboy framebuffer for collision check
extern Arduboy2 arduboy;

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
    vertex p1, p2;

    // Per-endpoint direction state (authentic Qix direction-selection algorithm)
    // dirSlot: 0-3 (indexes into SetA/SetB, rotated in steps of 1, wrapping at 3→0)
    // layout:  false = "negative" (UP-first), true = "positive" (DOWN-first, mirrored)
    // bounce:  increments each frame both endpoints are fully trapped; triggers layout swap
    byte   dir1 = 0;    byte   dir2 = 2;   // start in different slots for asymmetry
    int8_t bounce1 = 0; int8_t bounce2 = 0;
    bool   layout1 = false; bool layout2 = true; // opposite layouts for visual variety

    // FORCED_CHANGE_FLAG ($79 analog): set by bounce handler or frameHelper's 1/64 roll.
    // Cleared when a direction is successfully committed.
    // When true the starting slot is always perturbed (+2) regardless of PRNG low bits.
    bool forcedChange = false;

    // History for trailing effect
    static const int QIX_HISTORY = 4;
    vertex hist1[QIX_HISTORY];
    vertex hist2[QIX_HISTORY];
    int histIdx = 0;

    // Movement throttling
    int moveTick = 0;
    int moveInterval = 2;

    // Shared 16-bit LFSR state (seeded once; period 65535).
    // Advanced ONCE per frame in frameHelper() — both endpoints read the same
    // post-advance value, producing the correlated motion of the original arcade board.
    uint16_t rng = 0xACE1;
    uint8_t  framePhase = 0; // IRQ-style mixing counter (~$E014)

    // frameHelper — mirrors the per-frame helper at $E014 in the original ROM.
    // Call exactly once per frame before moving either endpoint.
    //   1. Advance LFSR.
    //   2. Mix in a frame-phase byte so timing has subtle variation.
    //   3. Occasionally (~1/64) assert forcedChange to break tied/trapped states.
    inline void frameHelper() {
      rng ^= rng << 7;
      rng ^= rng >> 9;
      rng ^= rng << 8;
      framePhase++;
      rng ^= ((uint16_t)framePhase << 8);
      if ((rng & 0x003F) == 0) forcedChange = true;
    }

    qix() {
      p1 = vertex(WIDTH / 3, HEIGHT / 3);
      p2 = vertex((WIDTH * 2) / 3, (HEIGHT * 2) / 3);
      for (int i = 0; i < QIX_HISTORY; i++) { hist1[i] = p1; hist2[i] = p2; }
      histIdx = 0;
    }

    // Return direction vector for slot (0-3), set 0=SetA cardinal / 1=SetB diagonal,
    // and the current layout flag.
    // Layout 0 (negative, UP-first):
    //   SetA: [UP(0,-1), RIGHT(1,0), DOWN(0,1), LEFT(-1,0)]
    //   SetB: [UL(-1,-1), UR(1,-1), DR(1,1), DL(-1,1)]
    // Layout 1 (positive, DOWN-first, mirrored):
    //   SetA: [DOWN(0,1), LEFT(-1,0), UP(0,-1), RIGHT(1,0)]
    //   SetB: [DR(1,1), DL(-1,1), UL(-1,-1), UR(1,-1)]
    static inline void getDir(byte slot, byte setIdx, bool layout,
                               int8_t &dx, int8_t &dy) {
      static const int8_t A0x[4] = { 0,  1, 0, -1};
      static const int8_t A0y[4] = {-1,  0, 1,  0};
      static const int8_t A1x[4] = { 0, -1, 0,  1};
      static const int8_t A1y[4] = { 1,  0,-1,  0};
      static const int8_t B0x[4] = {-1,  1, 1, -1};
      static const int8_t B0y[4] = {-1, -1, 1,  1};
      static const int8_t B1x[4] = { 1, -1,-1,  1};
      static const int8_t B1y[4] = { 1,  1,-1, -1};
      byte s = slot & 3;
      if (setIdx == 0) {
        dx = layout ? A1x[s] : A0x[s];
        dy = layout ? A1y[s] : A0y[s];
      } else {
        dx = layout ? B1x[s] : B0x[s];
        dy = layout ? B1y[s] : B0y[s];
      }
    }

    // Choose a direction and move one endpoint by one step.
    //
    // Pre-perturbation (slot rotate +2) before either pass:
    //   • Always when player is drawing (DIR_LOCK_FLAG analog — makes Qix aggressive).
    //   • Always when forcedChange is set.
    //   • Otherwise when low 2 bits of the already-advanced shared LFSR == 0 (~1/4).
    //
    // Pass 1 — SetB diagonals: first clear pixel wins.
    // Pass 2 — SetA cardinals: first clear pixel wins.
    // Fully blocked: increment bounce; at threshold swap layout + assert forcedChange.
    bool moveEndpoint(vertex &pos, byte &dirSlot, int8_t &bounce, bool &layout) {
      int8_t dx, dy;

      // DIR_LOCK_FLAG: player actively drawing → force perturbation every frame.
      bool playerDrawing = (p.allowedMoves & 0x30) != 0;
      byte startSlot = dirSlot;
      if (playerDrawing || forcedChange || (rng & 0x0003) == 0) {
        startSlot = (startSlot + 2) & 3;
      }

      // Pass 1 — SetB (diagonal)
      for (int i = 0; i < 4; i++) {
        byte slot = (startSlot + i) & 3;
        getDir(slot, 1, layout, dx, dy);
        int nx = pos.getx() + dx, ny = pos.gety() + dy;
        if (nx <= 1 || nx >= WIDTH-2 || ny <= 1 || ny >= HEIGHT-2) continue;
        if (!arduboy.getPixel(nx, ny)) {
          dirSlot = slot; bounce = 0; forcedChange = false;
          pos = vertex(nx, ny);
          return true;
        }
      }

      // Pass 2 — SetA (cardinal)
      // Original additionally skips pixels with PLAYER_TRAIL_MASK set; on 1-bit
      // Arduboy all white pixels are treated as blocked.
      for (int i = 0; i < 4; i++) {
        byte slot = (startSlot + i) & 3;
        getDir(slot, 0, layout, dx, dy);
        int nx = pos.getx() + dx, ny = pos.gety() + dy;
        if (nx <= 1 || nx >= WIDTH-2 || ny <= 1 || ny >= HEIGHT-2) continue;
        if (!arduboy.getPixel(nx, ny)) {
          dirSlot = slot; bounce = 0; forcedChange = false;
          pos = vertex(nx, ny);
          return true;
        }
      }

      // Fully blocked: update bounce counter; swap layout at threshold
      if (++bounce >= 4) {
        layout = !layout;
        dirSlot = (dirSlot + 1) & 3;
        bounce = 0;
        forcedChange = true; // FORCED_CHANGE_FLAG — nudge direction on next frame
      }
      return false;
    }

    void update() {
      moveTick++;
      if (moveTick < moveInterval) return;
      moveTick = 0;

      // Advance shared LFSR once — both endpoints read same post-advance state
      // (correlated, arcade-authentic). Mirrors $E014 pre-move helper.
      frameHelper();

      // Save positions to history before moving
      hist1[histIdx] = p1;
      hist2[histIdx] = p2;
      histIdx = (histIdx + 1) % QIX_HISTORY;

      moveEndpoint(p1, dir1, bounce1, layout1);
      moveEndpoint(p2, dir2, bounce2, layout2);
    }

    vertex center() const {
      return vertex((p1.getx() + p2.getx()) / 2, (p1.gety() + p2.gety()) / 2);
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
