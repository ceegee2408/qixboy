#include "rendering.h"

// Sprite data moved to include/sprites.h and src/sprites.cpp
#include "sprites.h"

// Packs SPRITE_SIZE^2 (25) pixel states into a uint32_t.
static_assert(SPRITE_SIZE * SPRITE_SIZE <= 32,
              "SPRITE_SIZE too large for uint32_t background buffer");

// Array-based background buffers: one per BgSlot
static uint32_t bgBuffers[BG_SLOT_COUNT]  = {0, 0};
static vertex   bgSavedPositions[BG_SLOT_COUNT];
static bool     bgSavedFlags[BG_SLOT_COUNT] = {false, false};
// Death animation center (set when the player dies)
vertex deathAnimCenter;

static int deathAnimRadius = 0;
static int deathAnimTick = 0;
static int deathAnimPhase = 0;
static int radiusStep = 0;

#define DEATH_RADIUS_MAX    300
#define DEATH_TICK_INTERVAL  1
#define DEATH_PULSE_COUNT    2

void saveBackground(vertex pos, BgSlot slot) {
  bgSavedPositions[slot] = pos;
  bgSavedFlags[slot] = true;
  bgBuffers[slot] = 0;
  int sx = pos.x - PLAYER_SIZE;
  int sy = pos.y - PLAYER_SIZE;
  for (int row = 0; row < SPRITE_SIZE; row++) {
      for (int col = 0; col < SPRITE_SIZE; col++) {
          int px = sx + col, py = sy + row;
          if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
              if (arduboy.getPixel(px, py)) {
                  bgBuffers[slot] |= (1UL << (row * SPRITE_SIZE + col));
              }
          }
      }
  }
}

void restoreBackground(BgSlot slot) {
  if (!bgSavedFlags[slot]) return;
  int sx = bgSavedPositions[slot].x - PLAYER_SIZE;
  int sy = bgSavedPositions[slot].y - PLAYER_SIZE;
  for (int row = 0; row < SPRITE_SIZE; row++) {
      for (int col = 0; col < SPRITE_SIZE; col++) {
          int px = sx + col, py = sy + row;
          if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
              bool lit = (bgBuffers[slot] >> (row * SPRITE_SIZE + col)) & 1;
              arduboy.drawPixel(px, py, lit ? WHITE : BLACK);
          }
      }
  }
  bgSavedFlags[slot] = false;
}

void drawLine(vertex v1, vertex v2) {
    arduboy.drawLine(v1.x, v1.y, v2.x, v2.y);
}

void drawInvertedLine(vertex v1, vertex v2) {
    int x1 = v1.x, y1 = v1.y;
    int x2 = v2.x, y2 = v2.y;
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        bool cur = arduboy.getPixel(x1, y1);
        arduboy.drawPixel(x1, y1, cur ? BLACK : WHITE);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void drawPerimeter() {
  for (int i = 0; i < perim.vertexCount; i++) {
    vertex v1 = perim.vertices[i];
    vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
    if (v1.y == v2.y) {
      arduboy.drawFastHLine(min(v1.x, v2.x), v1.y, abs(v2.x - v1.x) + 1);
    } else if (v1.x == v2.x) {
      arduboy.drawFastVLine(v1.x, min(v1.y, v2.y), abs(v2.y - v1.y) + 1);
    }
  }
}

void drawPlayer() {
    int sx = p.position.x - PLAYER_SIZE;
    int sy = p.position.y - PLAYER_SIZE;
    for (int row = 0; row < SPRITE_SIZE; row++) {
        uint8_t bits = pgm_read_byte(&playerSpriteRows[row]);
        for (int col = 0; col < SPRITE_SIZE; col++) {
            if (bits & (0x80 >> col)) {
                int px = sx + col, py = sy + row;
                if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
          bool cur = arduboy.getPixel(px, py);
          arduboy.drawPixel(px, py, cur ? BLACK : WHITE);
                }
            }
        }
    }
}

void drawQix() {
  drawInvertedLine(q.p1, q.p2);
  q.recordHistory();
}

void eraseQixHistory() {
    drawInvertedLine(q.hist1[(q.histIdx) % q.QIX_HISTORY], q.hist2[(q.histIdx) % q.QIX_HISTORY]);
}

void drawTrail() {
  // Draw the player's trail as they move in draw mode.
  // Connect the vertices in p.trail to create the outline of the area being drawn.
  for (int i = 1; i < p.trailCount; i++) {
    drawLine(p.trail[i - 1], p.trail[i]);
  }
  if (p.trailCount > 0) {
    drawLine(p.trail[p.trailCount - 1], p.position); // Connect last trail vertex to current position
  }
}

void drawDebug() {
#if DEBUG
  arduboy.fillRect(WIDTH / 2, HEIGHT / 2, 40, 8, BLACK);
  arduboy.setCursor(WIDTH / 2, HEIGHT / 2);
  arduboy.setTextSize(1);
  arduboy.print(p.getPerimIndex(0));
  arduboy.print(F("-"));
  arduboy.print(p.getPerimIndex(1));
  arduboy.setCursor(WIDTH / 2, HEIGHT / 2 + 8);
  arduboy.print(F(" M:"));  
  for(int i = 0; i < 6; i++) {
    arduboy.print((p.allowedMoves & (1 << i)) ? 1 : 0);
  }
#endif
}

void initializeFill(bool speed) {
  fillAnimationFrame = 0;
  fillDith = speed;
  if (fz.active) {
    restoreBackground(BG_FUZE);
    fz.active = false;
  }
  restoreBackground(BG_PLAYER);
}

// Prototype for animated horizontal line used by scanlineFill
void drawAnimatedHLine(int x, int y, int w, bool fast);

void scanlineFill(vertex* verts, int count, bool fast) {
  //int pointMult = 0;
  int y = fillAnimationFrame;
  if (y < 0) y = 0;
  if (y >= HEIGHT) {
    gameState = PLAYING;
    fillAnimationFrame = 0;
    return;
  }

  // Compute intersections for this scanline
  int xs[32];
  int xCount = 0;
  for (int i = 0; i < count; i++) {
    vertex v1 = verts[i];
    vertex v2 = verts[(i + 1) % count];
    int minY = min(v1.y, v2.y);
    int maxY = max(v1.y, v2.y);
    if (y < minY || y >= maxY) continue;
    if (v1.y == v2.y) continue; // Skip horizontal edges
    int x = (v1.x == v2.x) ? v1.x
      : v1.x + (y - v1.y) * (v2.x - v1.x) / (v2.y - v1.y);
    if (xCount < (int)(sizeof(xs) / sizeof(xs[0]))) xs[xCount++] = x;
  }

  // Sort xs (insertion sort, small n)
  for (int a = 1; a < xCount; a++) {
    int key = xs[a];
    int b = a - 1;
    while (b >= 0 && xs[b] > key) {
      xs[b + 1] = xs[b];
      b--;
    }
    xs[b + 1] = key;
  }

  // Draw spans for this single scanline
  for (int i = 0; i + 1 < xCount; i += 2) {
    int x1 = xs[i];
    int x2 = xs[i + 1];
    if (x2 > x1 + 1) drawAnimatedHLine(x1 + 1, y, x2 - x1 - 1, fast);/*number of pixels filled (for scoring)*/;
  }

  // Advance to next scanline for next frame; when finished, finalize
  fillAnimationFrame++;
  //pointCounter(pointMult, fast);
  if (fillAnimationFrame >= HEIGHT) {
    gameState = PLAYING;
    fillAnimationFrame = 0;
    saveBackground(p.position);
  }
}

void drawAnimatedHLine(int x, int y, int w, bool fast) { 
  // Draw a horizontal span. For 'fast' fill use a dithering pattern (draw
  // alternating pixels) to produce a dashed appearance. Do not call
  // `display()` or `delay()` here — the main loop handles frame timing.
  if (w <= 0) return;
  //exceptions
  if (y == 0) return;
  if (fast) {
    // Draw dithered pattern, but do not overwrite existing white pixels
    for (int i = 0; i < w; i++) {
      if (((x + i + y) & 1) == 0) {
        int px = x + i;
        if (!arduboy.getPixel(px, y)) arduboy.drawPixel(px, y, WHITE);
      }
    }
  } else {
    for (int i = 0; i < w; i++) {
      int px = x + i;
      if (!arduboy.getPixel(px, y)) arduboy.drawPixel(px, y, WHITE);
    }
  }
}

void pointCounter(int points, bool fast) {
  if (fast) {
    score += points / 2; // Fast fill: half points
  } else {
    score += points;     // Slow fill: full points
  }
  
}

void drawSpriteFrame_P(const uint8_t *frames, int frameIdx, int frameW, int frameH, int x, int y) {
  if (frameW <= 0 || frameH <= 0) return;
  // frames is stored as contiguous [frame][row]
  const uint8_t *base = frames + frameIdx * frameH;
  for (int r = 0; r < frameH; r++) {
    uint8_t bits = pgm_read_byte(base + r);
    for (int c = 0; c < frameW; c++) {
      bool lit = false;
      if (frameW <= 5) {
        lit = bits & (0x80 >> c); // 5-wide sprites use MSB-left
      } else if (frameW == 7) {
        lit = bits & (0x40 >> c); // 7-wide sprites stored in low 7 bits (MSB at 0x40)
      } else {
        lit = bits & (0x80 >> c);
      }
      if (lit) {
        int px = x + c;
        int py = y + r;
        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT)
          arduboy.drawPixel(px, py, WHITE);
      }
    }
  }
}

// Implement fuze rendering here to keep drawing code centralized and avoid
// flicker. Uses the PROGMEM frames in `fuzeSpriteFrames`.
void fuze::render() {
  if (!active) return;
  int fx = position.x - PLAYER_SIZE;
  int fy = position.y - PLAYER_SIZE;
  // fuze.frame may run 0..7; frameset is 4 frames, so modulo 4
  int fidx = (frame / FUZE_FRAME_TICKS) % FUZE_FRAME_COUNT;
  drawSpriteFrame_P((const uint8_t*)fuzeSpriteFrames, fidx, SPRITE_SIZE, SPRITE_SIZE, fx, fy);
}

void initializeDeathAnimation() {
  gameState = DEATH_ANIMATION;
  deathAnimCenter = p.position;
  deathAnimRadius = 1;
  deathAnimTick   = 0;
  deathAnimPhase  = 0;
  radiusStep = 10;
  restoreBackground(BG_PLAYER);
}

void respawn() {
  p.respawn();
  perim.reset();
  q.p1 = vertex(WIDTH / 3, HEIGHT / 3);
  q.p2 = vertex((WIDTH * 2) / 3, (HEIGHT * 2) / 3);
  q.hist1[0] = q.p1;
  q.hist2[0] = q.p2;
  q.histIdx = 1;
  fz.active = false;
  fillAnimationFrame = 0;
  bgSavedFlags[BG_PLAYER] = false;
  bgSavedFlags[BG_FUZE] = false;
  gameState = PLAYING;
}

int xreset = 0;
int yreset = 0;

void drawDeathAnimation() {
  if(deathAnimPhase < 2) {
    int cx = deathAnimCenter.x;
    int cy = deathAnimCenter.y;
    int offsets[] = { 0, 3, 7, 11, 15, 17, 19 };

    for (int o = 0; o < 4; o++) {
      int r = deathAnimRadius - offsets[o];
      if (r < 1) continue;
      for (int i = 0; i <= r; i++) {
        int coords[4][2] = {
          { cx + i, cy - (r - i) },
          { cx + i, cy + (r - i) },
          { cx - i, cy + (r - i) },
          { cx - i, cy - (r - i) }
        };
        for (int c = 0; c < 4; c++) {
          int x = coords[c][0], y = coords[c][1];
          if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            if (deathAnimPhase == 1) {
              arduboy.drawPixel(x, y, BLACK);
            } else {
              bool co = (arduboy.getPixel(x, y) ? BLACK : WHITE);
              arduboy.drawPixel(x, y, co);
            }
          }
        }
      }
    }
    deathAnimTick++;
    if (deathAnimTick < DEATH_TICK_INTERVAL) return;
    deathAnimTick = 0;
    deathAnimRadius += radiusStep;

    if (deathAnimRadius > DEATH_RADIUS_MAX) {
    deathAnimRadius = 1;
    deathAnimPhase++;
    }
  } else {
    arduboy.drawFastHLine(0, 0, WIDTH, WHITE);
    arduboy.display();
    for (int y = 1; y < HEIGHT-1; y+=2) {
      arduboy.drawPixel(0, y, WHITE);
      arduboy.drawFastHLine(1, y, WIDTH-2, BLACK);
      arduboy.drawPixel(WIDTH-1, y, WHITE);
      arduboy.display();
    }
    for (int y = 2; y < HEIGHT-1; y+=2) {
      arduboy.drawPixel(0, y, WHITE);
      arduboy.drawFastHLine(1, y, WIDTH-2, BLACK);
      arduboy.drawPixel(WIDTH-1, y, WHITE);
      arduboy.display();
    }
    arduboy.drawFastHLine(0, HEIGHT-1, WIDTH, WHITE);
    arduboy.display();
    delay(1000);
    respawn();
    drawPerimeter();          // redraw the perimeter on the clean screen
    //gameState = GAME_OVER;
    //while(true){}
  }
}