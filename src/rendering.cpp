#include "rendering.h"

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
    drawLine(perim.vertices[i], perim.vertices[(i + 1) % perim.vertexCount]);
  }
}

void drawPlayer() {
  // Draw diamond shape for player
  arduboy.drawLine(p.position.getx(), p.position.gety() - PLAYER_SIZE, p.position.getx() - PLAYER_SIZE, p.position.gety());
  arduboy.drawLine(p.position.getx() - PLAYER_SIZE, p.position.gety(), p.position.getx(), p.position.gety() + PLAYER_SIZE);
  arduboy.drawLine(p.position.getx(), p.position.gety() + PLAYER_SIZE, p.position.getx() + PLAYER_SIZE, p.position.gety());
  arduboy.drawLine(p.position.getx() + PLAYER_SIZE, p.position.gety(), p.position.getx(), p.position.gety() - PLAYER_SIZE);
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
  arduboy.setCursor(WIDTH / 2, HEIGHT / 2);
  arduboy.print(F("P:"));
  arduboy.print(p.position.getx());
  arduboy.print(F(","));
  arduboy.print(p.position.gety());
  arduboy.setCursor(WIDTH / 2, HEIGHT / 2 + 8);
  arduboy.print(F("E:"));
  arduboy.print(p.getPerimIndex(0));
  arduboy.print(F("-"));
  arduboy.print(p.getPerimIndex(1));
  arduboy.setCursor(WIDTH / 2, HEIGHT / 2 + 16);
  arduboy.print(F("M:"));
  arduboy.print(p.allowedMoves, BIN);
#endif
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
