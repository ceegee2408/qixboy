#include "rendering.h"

// Sprite data moved to include/sprites.h and src/sprites.cpp
#include "sprites.h"

// Packs SPRITE_SIZE^2 (25) pixel states into a uint32_t.
static_assert(SPRITE_SIZE * SPRITE_SIZE <= 32,
              "SPRITE_SIZE too large for uint32_t background buffer");

static uint32_t bgBuffer  = 0;
static vertex   bgSavedPos;
static bool     bgSaved   = false;

// Separate background buffer for fuze sprite to avoid smearing
static uint32_t fzBgBuffer = 0;
static vertex   fzBgSavedPos;
static bool     fzBgSaved = false;
// Death animation center (set when the player dies)
vertex deathAnimCenter;

static int deathAnimRadius = 0;
static int deathAnimTick = 0;
static int deathAnimPhase = 0;
static int radiusStep = 0;

#define DEATH_RADIUS_MAX    300
#define DEATH_TICK_INTERVAL  1
#define DEATH_PULSE_COUNT    2

void saveBackground(vertex pos) {
  bgSavedPos = pos;
  bgSaved    = true;
  bgBuffer   = 0;
  int sx = pos.getx() - PLAYER_SIZE;
  int sy = pos.gety() - PLAYER_SIZE;
  for (int row = 0; row < SPRITE_SIZE; row++) {
      for (int col = 0; col < SPRITE_SIZE; col++) {
          int px = sx + col, py = sy + row;
          if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
              if (arduboy.getPixel(px, py)) {
                  bgBuffer |= (1UL << (row * SPRITE_SIZE + col));
              }
          }
      }
  }
}

void restoreBackground() {
  if (!bgSaved) return;
  int sx = bgSavedPos.getx() - PLAYER_SIZE;
  int sy = bgSavedPos.gety() - PLAYER_SIZE;
  for (int row = 0; row < SPRITE_SIZE; row++) {
      for (int col = 0; col < SPRITE_SIZE; col++) {
          int px = sx + col, py = sy + row;
          if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
              bool lit = (bgBuffer >> (row * SPRITE_SIZE + col)) & 1;
              arduboy.drawPixel(px, py, lit ? WHITE : BLACK);
          }
      }
  }
  bgSaved = false;
}

void saveFuzeBackground(vertex pos) {
  fzBgSavedPos = pos;
  fzBgSaved = true;
  fzBgBuffer = 0;
  int sx = pos.getx() - PLAYER_SIZE;
  int sy = pos.gety() - PLAYER_SIZE;
  for (int row = 0; row < SPRITE_SIZE; row++) {
    for (int col = 0; col < SPRITE_SIZE; col++) {
      int px = sx + col, py = sy + row;
      if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
        if (arduboy.getPixel(px, py)) {
          fzBgBuffer |= (1UL << (row * SPRITE_SIZE + col));
        }
      }
    }
  }
}

void restoreFuzeBackground() {
  if (!fzBgSaved) return;
  int sx = fzBgSavedPos.getx() - PLAYER_SIZE;
  int sy = fzBgSavedPos.gety() - PLAYER_SIZE;
  for (int row = 0; row < SPRITE_SIZE; row++) {
    for (int col = 0; col < SPRITE_SIZE; col++) {
      int px = sx + col, py = sy + row;
      if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
        bool lit = (fzBgBuffer >> (row * SPRITE_SIZE + col)) & 1;
        arduboy.drawPixel(px, py, lit ? WHITE : BLACK);
      }
    }
  }
  fzBgSaved = false;
}

void drawLine(vertex v1, vertex v2) {
    arduboy.drawLine(v1.getx(), v1.gety(), v2.getx(), v2.gety());
}

void drawPerimeter() {
  for (int i = 0; i < perim.vertexCount; i++) {
    vertex v1 = perim.vertices[i];
    vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
    if (v1.gety() == v2.gety()) {
      arduboy.drawFastHLine(min(v1.getx(), v2.getx()), v1.gety(), abs(v2.getx() - v1.getx()));
    } else if (v1.getx() == v2.getx()) {
      arduboy.drawFastVLine(v1.getx(), min(v1.gety(), v2.gety()), abs(v2.gety() - v1.gety()));
    }
  }
}

void drawPlayer() {
    int sx = p.position.getx() - PLAYER_SIZE;
    int sy = p.position.gety() - PLAYER_SIZE;
    for (int row = 0; row < SPRITE_SIZE; row++) {
        uint8_t bits = pgm_read_byte(&playerSpriteRows[row]);
        for (int col = 0; col < SPRITE_SIZE; col++) {
            if (bits & (0x80 >> col)) {
                int px = sx + col, py = sy + row;
                if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                    arduboy.drawPixel(px, py, WHITE);
                }
            }
        }
    }
}

void drawQix() {
  // Draw an X shape for the qix
  arduboy.drawLine(q.position.getx() - 2, q.position.gety() - 2, q.position.getx() + 2, q.position.gety() + 2);
  arduboy.drawLine(q.position.getx() + 2, q.position.gety() - 2, q.position.getx() - 2, q.position.gety() + 2);
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
  // Clear a small area in the top-left corner
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
  restoreBackground();
}

// Prototype for animated horizontal line used by scanlineFill
void drawAnimatedHLine(int x, int y, int w, bool fast);

void scanlineFill(vertex* verts, int count, bool fast) {
  // Use global `fillAnimationFrame` as the current scanline index. Each call
  // advances the animation by one horizontal scanline so the animation can
  // progress across frames instead of blocking the loop.
  int y = fillAnimationFrame;
  if (y < 0) y = 0;
  if (y >= HEIGHT) {
    // Nothing to do; ensure state reset and background saved.
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
    int minY = min(v1.gety(), v2.gety());
    int maxY = max(v1.gety(), v2.gety());
    if (y < minY || y >= maxY) continue;
    if (v1.gety() == v2.gety()) continue; // Skip horizontal edges
    int x = (v1.getx() == v2.getx()) ? v1.getx()
      : v1.getx() + (y - v1.gety()) * (v2.getx() - v1.getx()) / (v2.gety() - v1.gety());
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
    if (x2 > x1 + 1) drawAnimatedHLine(x1 + 1, y, x2 - x1 - 1, fast);
  }

  // Advance to next scanline for next frame; when finished, finalize
  fillAnimationFrame++;
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
    //clear line to reduce ghosting
    arduboy.drawFastHLine(x, y, w, BLACK);
    for (int i = 0; i < w; i++) {
      if (((x + i + y) & 1) == 0) arduboy.drawPixel(x + i, y, WHITE);
    }
  } else {
    for (int i = 0; i < w; i++) {
      arduboy.drawPixel(x + i, y, WHITE);
    }
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
  int fx = position.getx() - PLAYER_SIZE;
  int fy = position.gety() - PLAYER_SIZE;
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
  restoreBackground();
}

void respawn() {
  p.respawn();
  perim.reset();
  q.position = vertex(WIDTH / 2, HEIGHT / 2);
  fz.active = false;
  fillAnimationFrame = 0;
  bgSaved = false;
  fzBgSaved = false;
  gameState = PLAYING;
}

int xreset = 0;
int yreset = 0;

void drawDeathAnimation() {
  if(deathAnimPhase < 2) {
    int cx = deathAnimCenter.getx();
    int cy = deathAnimCenter.gety();

    // Offsets from current radius — gaps shrink at larger distances
    // e.g. at r=14: draws at 14, 11, 7, 2 (gaps 3, 4, 5)
    // increase DEATH_RADIUS_MAX if you want more rings visible
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