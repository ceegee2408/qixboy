#include "rendering.h"

// Diamond outline, SPRITE_SIZE x SPRITE_SIZE (5x5), MSB = leftmost pixel.
//   Row 0:  00100  = 0x20
//   Row 1:  01010  = 0x50
//   Row 2:  10001  = 0x88
//   Row 3:  01010  = 0x50
//   Row 4:  00100  = 0x20
static const uint8_t PROGMEM playerSpriteRows[SPRITE_SIZE] = {
    0x20, 0x50, 0x88, 0x50, 0x20
};

// Packs SPRITE_SIZE^2 (25) pixel states into a uint32_t.
static_assert(SPRITE_SIZE * SPRITE_SIZE <= 32,
              "SPRITE_SIZE too large for uint32_t background buffer");

static uint32_t bgBuffer  = 0;
static vertex   bgSavedPos;
static bool     bgSaved   = false;

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
  arduboy.display(); // on for debugging
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
  arduboy.display(); // on for debugging
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
    drawPlayer();
  }
}

void drawAnimatedHLine(int x, int y, int w, bool fast) { 
  // Draw a horizontal span. For 'fast' fill use a dithering pattern (draw
  // alternating pixels) to produce a dashed appearance. Do not call
  // `display()` or `delay()` here — the main loop handles frame timing.
  if (w <= 0) return;
  if (fast) {
    //clear line to reduce ghosting
    arduboy.drawFastHLine(x, y, w, BLACK);
    for (int i = 0; i < w; i++) {
      if (((i + y) & 1) == 0) arduboy.drawPixel(x + i, y, WHITE);
    }
  } else {
    for (int i = 0; i < w; i++) {
      arduboy.drawPixel(x + i, y, WHITE);
    }
  }
}