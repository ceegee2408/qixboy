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

void initializeFill(){
  fillAnimationFrame = 0;
}

// Prototype for animated horizontal line used by scanlineFill
void drawAnimatedHLine(int x, int y, int w, int fast, int frames);

void scanlineFill(vertex* verts, int count, bool fast) {
  fillAnimationFrame++;
  int currentFrame = fillAnimationFrame;
  for (int y = 0; y < HEIGHT; y++) {
    byte xs[16];
    byte xCount = 0;
    // Compute intersections for this scanline
    for (int i = 0; i < count; i++) {
      vertex v1 = verts[i];
      vertex v2 = verts[(i + 1) % count];
      int minY = min(v1.gety(), v2.gety());
      int maxY = max(v1.gety(), v2.gety());
      if (y < minY || y >= maxY) continue;
      if (v1.gety() == v2.gety()) continue; // Skip horizontal edges
      int x = (v1.getx() == v2.getx()) ? v1.getx()
        : v1.getx() + (y - v1.gety()) * (v2.getx() - v1.getx()) / (v2.gety() - v1.gety());
      if (xCount < 16) xs[xCount++] = x;
    }
    // Sort xs (insertion sort, small n)
    for (int a = 1; a < xCount; a++) {
      byte key = xs[a], b = a - 1;
      while (b >= 0 && xs[b] > key) { 
        xs[b + 1] = xs[b]; 
        b--; 
      }
      xs[b + 1] = key;
    }
    // check if fill was fast or slow and draw accordingly

    if (fast) {
      for (int a = 0; a + 1 < xCount; a += 2) {
        drawAnimatedHLine(xs[a], y, xs[a+1] - xs[a], 1, currentFrame);
        currentFrame -= xs[a+1] - xs[a]; 
      }
    } else {
      for (int a = 0; a + 1 < xCount; a += 2) {
        drawAnimatedHLine(xs[a], y, xs[a+1] - xs[a], 0, currentFrame);
        currentFrame -= xs[a+1] - xs[a]; 
      }
    }
  }
}

void drawAnimatedHLine(int x, int y, int w, int fast, int _frames) { 
  //check if line will be drawn
  if (w < _frames) return;
  // check if fast or slow and draw accordingly
  x += _frames; // skip to current animation frame
  if (fast) {
    bool color = (y+x) % 2 == 0;
    arduboy.drawPixel(x, y, color);
  } else {
    arduboy.drawPixel(x, y, WHITE);
  }
}