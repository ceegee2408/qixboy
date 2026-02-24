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

void drawDotLine(vertex v1, vertex v2) {
  // Draw a dotted line by only drawing every other pixel
  int dx = v2.getx() - v1.getx();
  int dy = v2.gety() - v1.gety();
  int steps = max(abs(dx), abs(dy));
  float xInc = dx / (float) steps;
  float yInc = dy / (float) steps;
  float x = v1.getx();
  float y = v1.gety();
  
  for (int i = 0; i <= steps; i++) {
    if (i % 2 == 0) { // Draw every other pixel
      arduboy.drawPixel(round(x), round(y));
    }
    x += xInc;
    y += yInc;
  }
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
  arduboy.print(p.getPerimIndex(0));
  arduboy.print(F("-"));
  arduboy.print(p.getPerimIndex(1));
#endif
}

void drawFill() {
  // Fill claimed territory (everything outside the perimeter polygon)
  // Pre-compute polygon y-range so scanlines fully outside are filled quickly
  int polyMinY = HEIGHT, polyMaxY = 0;
  for (int i = 0; i < perim.vertexCount; i++) {
    int vy = perim.vertices[i].gety();
    if (vy < polyMinY) polyMinY = vy;
    if (vy > polyMaxY) polyMaxY = vy;
  }

  for (int y = 0; y < HEIGHT; y++) {
    if (y < polyMinY || y >= polyMaxY) {
      // Scanline is outside the polygon's vertical extent — fill entire line
      arduboy.drawFastHLine(0, y, WIDTH);
      continue;
    }

    int xs[16];
    int xCount = 0;
    for (int i = 0; i < perim.vertexCount; i++) {
      vertex v1 = perim.vertices[i];
      vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
      int minY = min(v1.gety(), v2.gety());
      int maxY = max(v1.gety(), v2.gety());
      if (y >= minY && y < maxY) {
        // Only vertical edges contribute; horizontals excluded by y-range
        if (xCount < 16) xs[xCount++] = v1.getx();
      }
    }

    // Sort (insertion sort, small n)
    for (int a = 1; a < xCount; a++) {
      int key = xs[a];
      int b = a - 1;
      while (b >= 0 && xs[b] > key) { xs[b + 1] = xs[b]; b--; }
      xs[b + 1] = key;
    }

    // Fill outside the polygon (complement of interior)
    // Even-odd rule: starts outside, toggles at each crossing
    // Outside spans: [0,xs[0]), [xs[1],xs[2]), ..., [xs[last],WIDTH)
    if (xCount >= 2) {
      if (xs[0] > 0)
        arduboy.drawFastHLine(0, y, xs[0]);
      for (int a = 1; a + 1 < xCount; a += 2) {
        if (xs[a + 1] > xs[a])
          arduboy.drawFastHLine(xs[a], y, xs[a + 1] - xs[a]);
      }
      if (xs[xCount - 1] < WIDTH)
        arduboy.drawFastHLine(xs[xCount - 1], y, WIDTH - xs[xCount - 1]);
    } else if (xCount == 0) {
      // Inside y-range but no crossings — treat as fully outside
      arduboy.drawFastHLine(0, y, WIDTH);
    }
  }
}

void scanlineFill(vertex* verts, int count) {
  for (int y = 0; y < HEIGHT; y++) {
    int xs[16];
    int xCount = 0;
    for (int i = 0; i < count; i++) {
      vertex v1 = verts[i];
      vertex v2 = verts[(i + 1) % count];
      // Find the x intersections of the horizontal ray at y with each edge
      int minY = min(v1.gety(), v2.gety());
      int maxY = max(v1.gety(), v2.gety());
      if (y < minY || y >= maxY) continue;  
      if (v1.gety() == v2.gety()) continue; // Skip horizontal edges
      int x = (v1.getx() == v2.getx()) ? v1.getx()
        : v1.getx() + (y - v1.gety()) * (v2.getx() - v1.getx()) / (v2.gety() - v1.gety());
      if (xCount < 16) {
        xs[xCount++] = x;
      }
    }
    // Sort xs (insertion sort, small n)
    for (int a = 1; a < xCount; a++) {
      int key = xs[a], b = a - 1;
      while (b >= 0 && xs[b] > key) { 
        xs[b + 1] = xs[b]; 
        b--; 
      }
      xs[b + 1] = key;
    }
    // Fill between pairs
    for (int a = 0; a + 1 < xCount; a += 2) {
      arduboy.drawFastHLine(xs[a], y, xs[a+1] - xs[a]);
    }
  }
}
