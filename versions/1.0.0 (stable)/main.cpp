#include <Arduino.h>
#include <Arduboy2.h>
// WIDTH = 128
// HEIGHT = 64
#define NUM_LIVES 3
#define PLAYER_SIZE 2
#define FAST_MOVE 3  // Move every 1 frame
#define SLOW_MOVE 4  // Move every 2 frames
#define MAX_VERTICES 50
#define DEBUG 0

class vertex {
  public:
    int x;
    int y;
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
  public:
    int lives = NUM_LIVES;
    int perimIndex[2] = {1, 2}; //index of current perimeter vertices (bottom edge)
    byte allowedMoves = 0x03; // bit 0-3 for left, right, up, down; bit 4 for fast move, bit 5 for slow move
    byte activeDir = 0; // current active direction
    byte prevInput = 0; // previous frame input
    byte lastTrailDir = 0; // last movement direction for trail corners
    int drawStartIndex[2] = {0, 0}; // perimeter index when draw mode began
    int drawEndIndex[2] = {0, 0}; // perimeter index when draw mode ended
    vertex position;
    vertex trail[MAX_VERTICES]; // store trail vertices for filling
    int trailCount = 0;
    player() {
      position = vertex(WIDTH/2, HEIGHT - 1);
    }

    void addTrailVertex(vertex v) {
      if (trailCount < MAX_VERTICES) {
        trail[trailCount] = v;
        trailCount++;
      }
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
    int speed = 1;
    qix() {
      position = vertex(WIDTH / 2, HEIGHT / 2);
    }
};

player p;
perimeter perim;
qix q;
Arduboy2 arduboy;
uint16_t frameCounter = 0;
void drawLine(vertex v1, vertex v2);
void drawPerimeter();
void drawPlayer();
byte getInput();
void updateActiveDirection(byte input);
void updateScreen();
void updatePlayer(byte input);
void movePlayer(byte allowedMoves);
void perimeterMove(byte input);
void updateDrawAllowance(byte input);
void updateCanMove();
void updatePerimIndex();
void drawDebug();
bool isInsidePerimeter(vertex point);
bool pointOnSegment(vertex point, vertex v1, vertex v2);
void drawMove(byte input, bool speed);
void updateTrail();
void drawTrail();
void updateCanDraw();
void updatePerim();
void fillArea();
void fillHLine(int x1, int x2, int y);
bool isInsidePolygon(vertex point, vertex* verts, int count);
void drawQix();

void setup() {
  // put your setup code here, to run once:
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.clear();
  updateCanMove();
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }
  frameCounter++;
  if (frameCounter >= 240) {
    frameCounter = 0;
  }
  arduboy.clear();
  fillArea();       // fill claimed territory (outside perimeter)
  drawPerimeter();
  drawPlayer();
  drawQix();
  drawDebug();
  updateScreen();
  arduboy.display();
}

void drawPlayer() {
  //draw diamond shape for player
  arduboy.drawLine(p.position.x, p.position.y - PLAYER_SIZE, p.position.x - PLAYER_SIZE, p.position.y);
  arduboy.drawLine(p.position.x - PLAYER_SIZE, p.position.y, p.position.x, p.position.y + PLAYER_SIZE);
  arduboy.drawLine(p.position.x, p.position.y + PLAYER_SIZE, p.position.x + PLAYER_SIZE, p.position.y);
  arduboy.drawLine(p.position.x + PLAYER_SIZE, p.position.y, p.position.x, p.position.y - PLAYER_SIZE);
}

void drawQix() {
  // Draw an X shape for the qix
  arduboy.drawLine(q.position.x - 2, q.position.y - 2, q.position.x + 2, q.position.y + 2);
  arduboy.drawLine(q.position.x + 2, q.position.y - 2, q.position.x - 2, q.position.y + 2);
}

void updateScreen() {
  byte input = getInput();
  updateActiveDirection(input);
  updatePlayer(input);
  //update qix
  //update sparx
  //update fill
  //check for death
}

byte getInput() {
  byte input = 0;
  if (arduboy.pressed(LEFT_BUTTON)) {
    input |= 0x01; // set bit 0 for left
  }
  if (arduboy.pressed(RIGHT_BUTTON)) {
    input |= 0x02; // set bit 1 for right
  }
  if (arduboy.pressed(UP_BUTTON)) {
    input |= 0x04; // set bit 2 for up
  }
  if (arduboy.pressed(DOWN_BUTTON)) {
    input |= 0x08; // set bit 3 for down
  }
  if (arduboy.pressed(A_BUTTON)) {
    //fast move
    input |= 0x10; // set bit 4 for fast move
  }
  if (arduboy.pressed(B_BUTTON)) {
    //slow move
    input |= 0x20; // set bit 5 for slow move
  }
  return input; 
}

void updateActiveDirection(byte input) {
  byte currentDir = input & 0x0F;
  byte prevDir = p.prevInput & 0x0F;
  
  // Check if there's a newly pressed button
  byte newlyPressed = currentDir & ~prevDir;
  
  if (newlyPressed) {
    // A button was newly pressed, make it active
    p.activeDir = newlyPressed;
  } else if ((p.activeDir & currentDir) == 0) {
    // Active direction was released, switch to any remaining pressed button
    p.activeDir = currentDir;
  }
  // else: keep current activeDir
  
  p.prevInput = input;
}

void updatePlayer(byte input) {
  updateDrawAllowance(input); // Check if we should enter or stay in draw mode
  if (p.allowedMoves & 0x30) {
    // In draw mode — always draw trail
    drawTrail();
    if ((input & 0x10) && (p.allowedMoves & 0x10)) {
      // fast move
      drawMove(input, true);
    } else if ((input & 0x20) && (p.allowedMoves & 0x20)) {
      // slow move
      drawMove(input, false);
    }
  } else if (input & 0x0F) {
    //perimeter move
    perimeterMove(input);
  }
}

void drawMove(byte input, bool speed) {
  byte interval = speed ? FAST_MOVE : SLOW_MOVE;
  // Only move when frameCounter is divisible by the interval
  if (frameCounter % interval == 0) {
    // Only update trail direction and move if activeDir is actually allowed
    if (p.activeDir & p.allowedMoves) {
      // Check if direction changed BEFORE moving - add current position as corner
      if (p.lastTrailDir != 0 && p.activeDir != p.lastTrailDir) {
        p.addTrailVertex(p.position);
      }
      p.lastTrailDir = p.activeDir;
      movePlayer(p.allowedMoves);
    }
    
    // Check if player has returned to the perimeter (only after moving away from start)
    if (p.trailCount > 0 &&
        (p.position.x != p.trail[0].x || p.position.y != p.trail[0].y)) {
      for (int i = 0; i < perim.vertexCount; i++) {
      vertex v1 = perim.vertices[i];
      vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
      if (pointOnSegment(p.position, v1, v2)) {
        // Player is back on perimeter, store end indices
        // If we re-enter exactly at a corner, normalize indices to that single vertex.
        int endA = i;
        int endB = (i + 1) % perim.vertexCount;
        if (p.position.x == v1.x && p.position.y == v1.y) {
          endA = i;
          endB = i;
        } else if (p.position.x == v2.x && p.position.y == v2.y) {
          endA = endB;
          endB = endB;
        }
        p.drawEndIndex[0] = endA;
        p.drawEndIndex[1] = endB;
        // Add end position as final trail vertex
        p.addTrailVertex(p.position);
        // Update the perimeter shape with the trail
        updatePerim();
        // Re-find player position on the new perimeter
        for (int j = 0; j < perim.vertexCount; j++) {
          vertex nv1 = perim.vertices[j];
          vertex nv2 = perim.vertices[(j + 1) % perim.vertexCount];
          if (pointOnSegment(p.position, nv1, nv2)) {
            p.perimIndex[0] = j;
            p.perimIndex[1] = (j + 1) % perim.vertexCount;
            break;
          }
        }
        updatePerimIndex(); // Check if at a corner
        updateCanMove(); // This clears draw mode bits and sets perimeter movement
        return; // Exit drawMove entirely to avoid updateCanDraw overwriting allowedMoves
      }
      }
    }
    // Still in draw mode, update valid moves
    updateCanDraw();
  }
}

bool pointOnSegment(vertex point, vertex v1, vertex v2) {
  // Check if point is on the axis-aligned line segment from v1 to v2
  int minX = min(v1.x, v2.x);
  int maxX = max(v1.x, v2.x);
  int minY = min(v1.y, v2.y);
  int maxY = max(v1.y, v2.y);
  if (v1.x == v2.x) {
    return point.x == v1.x && point.y >= minY && point.y <= maxY;
  } else if (v1.y == v2.y) {
    return point.y == v1.y && point.x >= minX && point.x <= maxX;
  }
  return false;
}

void updateCanDraw() {
  byte allowedMoves = 0x0F;

  // Block the opposite of the current travel direction (not input direction)
  if (p.lastTrailDir & 0x01) allowedMoves &= ~0x02; // traveling left, block right
  else if (p.lastTrailDir & 0x02) allowedMoves &= ~0x01; // traveling right, block left
  if (p.lastTrailDir & 0x04) allowedMoves &= ~0x08; // traveling up, block down
  else if (p.lastTrailDir & 0x08) allowedMoves &= ~0x04; // traveling down, block up

  // For each of the 4 directions, check if moving there would land on a trail segment
  byte dirs[4] = {0x01, 0x02, 0x04, 0x08};
  int dx[4] = {-2, 2, 0, 0};
  int dy[4] = {0, 0, -2, 2};

  for (int d = 0; d < 4; d++) {
    if (!(allowedMoves & dirs[d])) continue; // already blocked
    vertex nextPos = vertex(p.position.x + dx[d], p.position.y + dy[d]);

    // Check against each trail segment (corner to corner)
    for (int i = 0; i < p.trailCount - 1; i++) {
      if (pointOnSegment(nextPos, p.trail[i], p.trail[i + 1])) {
        allowedMoves &= ~dirs[d];
        break;
      }
    }
    // Check the live segment (last corner to current position)
    if ((allowedMoves & dirs[d]) && p.trailCount > 0) {
      if (pointOnSegment(nextPos, p.trail[p.trailCount - 1], p.position)) {
        allowedMoves &= ~dirs[d];
      }
    }
  }

  p.allowedMoves = allowedMoves | (p.allowedMoves & 0x30); // Preserve draw mode bits
}

void movePlayer(byte allowedMoves) {
  // Only move if active direction is allowed
  if (p.activeDir & allowedMoves) {
    // Move player based on activeDir (only one direction per frame)
    if (p.activeDir & 0x01) {
      // Move left
      p.position.x -= 1;
    } else if (p.activeDir & 0x02) {
      // Move right
      p.position.x += 1;
    } else if (p.activeDir & 0x04) {
      // Move up
      p.position.y -= 1;
    } else if (p.activeDir & 0x08) {
      // Move down
      p.position.y += 1;
    }
  }
}

void perimeterMove(byte input) {
  // Only move when frameCounter is divisible by FAST_MOVE interval
  if (frameCounter % FAST_MOVE == 0) {
    // Call movePlayer with perimeter constraints
    vertex prevPos = p.position;
    movePlayer(p.allowedMoves);
    if (p.position.x != prevPos.x || p.position.y != prevPos.y) {
      updatePerimIndex();
      updateCanMove();
    }
  }
}

bool compareVertices(vertex v1, vertex v2) {
  if (v1.x == v2.x && v1.y == v2.y) {
    return true;
  }
  return false;
}

int vertexDistance(vertex v1, vertex v2) {
  // Manhattan distance — avoids float/sqrt on AVR
  return abs(v2.x - v1.x) + abs(v2.y - v1.y);
}

byte getDirection(vertex from, vertex to) {
  byte dir = 0;
  if (to.x < from.x) dir |= 0x01;      // left
  else if (to.x > from.x) dir |= 0x02; // right
  if (to.y < from.y) dir |= 0x04;      // up
  else if (to.y > from.y) dir |= 0x08; // down
  return dir;
}

void updatePerimIndex() {
  int idxA = p.perimIndex[0];
  int idxB = p.perimIndex[1];
  vertex v1 = perim.vertices[idxA];
  vertex v2 = perim.vertices[idxB];
  int prevIdx = (idxA - 1 + perim.vertexCount) % perim.vertexCount;
  int nextIdx = (idxB + 1) % perim.vertexCount;

  if (p.position.x == v1.x && p.position.y == v1.y) {
    p.perimIndex[0] = idxA;
    p.perimIndex[1] = idxA;
    return;
  }
  if (p.position.x == v2.x && p.position.y == v2.y) {
    p.perimIndex[0] = idxB;
    p.perimIndex[1] = idxB;
    return;
  }

  // If player moved off the current edge (or corner), pick the adjacent edge.
  if (!pointOnSegment(p.position, v1, v2)) {
    vertex prevV = perim.vertices[prevIdx];
    if (pointOnSegment(p.position, v1, prevV)) {
      p.perimIndex[0] = idxA;
      p.perimIndex[1] = prevIdx;
    } else {
      vertex nextV = perim.vertices[nextIdx];
      if (pointOnSegment(p.position, v2, nextV)) {
        p.perimIndex[0] = nextIdx;
        p.perimIndex[1] = idxB;
      } else {
        // Fallback: full scan after perimeter restructure
        for (int i = 0; i < perim.vertexCount; i++) {
          vertex a = perim.vertices[i];
          vertex b = perim.vertices[(i + 1) % perim.vertexCount];
          if (pointOnSegment(p.position, a, b)) {
            p.perimIndex[0] = i;
            p.perimIndex[1] = (i + 1) % perim.vertexCount;
            return;
          }
        }
      }
    }
  }
}

void updateCanMove() {
  p.allowedMoves = 0;
  vertex v1 = perim.vertices[p.perimIndex[0]];
  vertex v2 = perim.vertices[p.perimIndex[1]];
  
  // Get adjacent vertices for corner handling
  int prevIdx = (p.perimIndex[0] - 1 + perim.vertexCount) % perim.vertexCount;
  int nextIdx = (p.perimIndex[1] + 1) % perim.vertexCount;
  vertex prevV = perim.vertices[prevIdx];
  vertex nextV = perim.vertices[nextIdx];

  if (p.perimIndex[0] == p.perimIndex[1]) {
    // At a corner, allow movement along both adjacent edges
    p.allowedMoves = getDirection(v1, prevV) | getDirection(v1, nextV);
  } else {
    // On edge between v1 and v2 - can move toward either endpoint
    p.allowedMoves = getDirection(p.position, v1) | getDirection(p.position, v2);
  }
}

void updateDrawAllowance(byte input) {
  // If already in draw mode, keep the draw mode bit that's currently active
  if (p.allowedMoves & 0x30) {
    // Preserve the existing draw mode bit (fast or slow)
    return;
  }
  
  // Not in draw mode: only enter if a draw button is actually pressed
  if (!(input & 0x30)) return;
  
  // Check if active direction is not allowed by perimeter movement
  if (p.activeDir != 0 && (p.activeDir & p.allowedMoves) == 0) {
    // Calculate next position based on the active direction
    vertex nextPos = p.position;
    
    // Check each individual direction bit
    if (p.activeDir & 0x01) nextPos.x -= 1;  // left
    else if (p.activeDir & 0x02) nextPos.x += 1;  // right
    
    if (p.activeDir & 0x04) nextPos.y -= 1;  // up
    else if (p.activeDir & 0x08) nextPos.y += 1;  // down
    
    // Only allow draw mode if next position is inside the perimeter
    if (isInsidePerimeter(nextPos)) {
      // Set only the bit corresponding to the button being pressed
      p.allowedMoves |= (input & 0x30);
      // Initialize trail when entering draw mode (one-shot)
      // Determine the directed perimeter edge (i -> i+1) that contains the player
      // so arc cutting is consistent even if perimIndex ordering flips.
      for (int i = 0; i < perim.vertexCount; i++) {
        vertex v1 = perim.vertices[i];
        vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
        if (pointOnSegment(p.position, v1, v2)) {
          p.drawStartIndex[0] = i;
          p.drawStartIndex[1] = (i + 1) % perim.vertexCount;
          break;
        }
      }
      p.trailCount = 0;
      p.addTrailVertex(p.position);
      p.lastTrailDir = p.activeDir; // Set initial direction
    }
  }
}

void drawLine(vertex v1, vertex v2) {
  arduboy.drawLine(v1.x, v1.y, v2.x, v2.y);
}

void drawDotLine(vertex v1, vertex v2) {
  // Draw a dotted line by only drawing every other pixel
  int dx = v2.x - v1.x;
  int dy = v2.y - v1.y;
  int steps = max(abs(dx), abs(dy));
  float xInc = dx / (float) steps;
  float yInc = dy / (float) steps;
  float x = v1.x;
  float y = v1.y;
  
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

void drawDebug() {
#if DEBUG
  arduboy.setCursor(WIDTH/2, HEIGHT/2);
  arduboy.print(F("P:"));
  arduboy.print(p.position.x);
  arduboy.print(F(","));
  arduboy.print(p.position.y);
  arduboy.setCursor(WIDTH/2, HEIGHT/2 + 8);
  arduboy.print(F("E:"));
  arduboy.print(p.perimIndex[0]);
  arduboy.print(F("-"));
  arduboy.print(p.perimIndex[1]);
  arduboy.setCursor(WIDTH/2, HEIGHT/2 + 16);
  arduboy.print(F("M:"));
  arduboy.print(p.allowedMoves, BIN);
#endif
}

int crossProduct(vertex a, vertex b, vertex c) {
  // Calculate the z component of the cross product of vectors AB and AC
  // Returns: positive if c is left of line ab, negative if right, 0 if collinear
  int ax = b.x - a.x;
  int ay = b.y - a.y;
  int bx = c.x - a.x;
  int by = c.y - a.y;
  return ax * by - ay * bx;
}

bool isInsidePerimeter(vertex point) {

  // Check if the point is inside the polygon using the winding number method
  int windingNumber = 0;
  for (int i = 0; i < perim.vertexCount; i++) {
    vertex v1 = perim.vertices[i];
    vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
    if (v1.y <= point.y) {
      if (v2.y > point.y && crossProduct(v1, v2, point) > 0) {
        windingNumber++;
      }
    } else {
      if (v2.y <= point.y && crossProduct(v1, v2, point) < 0) {
        windingNumber--;
      }
    }
  }
  return windingNumber != 0;
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

bool isInsidePolygon(vertex point, vertex* verts, int count) {
  // Winding number method for arbitrary polygon
  int windingNumber = 0;
  for (int i = 0; i < count; i++) {
    vertex v1 = verts[i];
    vertex v2 = verts[(i + 1) % count];
    if (v1.y <= point.y) {
      if (v2.y > point.y && crossProduct(v1, v2, point) > 0) {
        windingNumber++;
      }
    } else {
      if (v2.y <= point.y && crossProduct(v1, v2, point) < 0) {
        windingNumber--;
      }
    }
  }
  return windingNumber != 0;
}

// Single static scratch buffer for updatePerim — avoids stack overflow on AVR
static vertex scratch[MAX_VERTICES];

void updatePerim() {
  if (p.trailCount <= 0) return;

  // Build two candidate polygons:
  // Option A: trail + arc from drawEndIndex[1] forward to drawStartIndex[0]
  // Option B: trail (reversed) + arc from drawStartIndex[1] forward to drawEndIndex[0]
  // The polygon that contains the qix becomes the new perimeter.

  // --- Option A: trail forward + kept arc ---
  int countA = 0;
  for (int i = 0; i < p.trailCount && countA < MAX_VERTICES; i++) {
    if (countA > 0 && compareVertices(scratch[countA - 1], p.trail[i])) continue;
    scratch[countA++] = p.trail[i];
  }
  int idx = p.drawEndIndex[1];
  int stop = p.drawStartIndex[0];
  for (int safety = 0; safety < perim.vertexCount && countA < MAX_VERTICES; safety++) {
    if (countA == 0 || !compareVertices(scratch[countA - 1], perim.vertices[idx])) {
      scratch[countA++] = perim.vertices[idx];
    }
    if (idx == stop) break;
    idx = (idx + 1) % perim.vertexCount;
  }
  if (countA > 1 && compareVertices(scratch[0], scratch[countA - 1])) {
    countA--; // avoid duplicate closing vertex
  }

  bool qixInA = (countA >= 3) ? isInsidePolygon(q.position, scratch, countA) : false;

  if (!qixInA) {
    // --- Option B: trail reversed + other arc ---
    int countB = 0;
    for (int i = p.trailCount - 1; i >= 0 && countB < MAX_VERTICES; i--) {
      if (countB > 0 && compareVertices(scratch[countB - 1], p.trail[i])) continue;
      scratch[countB++] = p.trail[i];
    }
    idx = p.drawStartIndex[1];
    stop = p.drawEndIndex[0];
    for (int safety = 0; safety < perim.vertexCount && countB < MAX_VERTICES; safety++) {
      if (countB == 0 || !compareVertices(scratch[countB - 1], perim.vertices[idx])) {
        scratch[countB++] = perim.vertices[idx];
      }
      if (idx == stop) break;
      idx = (idx + 1) % perim.vertexCount;
    }
    if (countB > 1 && compareVertices(scratch[0], scratch[countB - 1])) {
      countB--;
    }
    countA = countB; // reuse countA for the final copy
  }

  if (countA < 3) return; // defensive: don't corrupt perimeter with degenerate polygon

  // Copy the chosen polygon back as the new perimeter
  perim.vertexCount = countA;
  for (int i = 0; i < countA; i++) {
    perim.vertices[i] = scratch[i];
  }
}

// Fast horizontal span fill — writes directly to Arduboy screen buffer
void fillHLine(int x1, int x2, int y) {
  if (x1 > x2 || y < 0 || y >= HEIGHT) return;
  if (x1 < 0) x1 = 0;
  if (x2 >= WIDTH) x2 = WIDTH - 1;
  uint8_t mask = 1 << (y & 7);
  int bufRow = (y >> 3) * WIDTH;
  uint8_t* buf = Arduboy2::sBuffer;
  for (int x = x1; x <= x2; x++) {
    buf[bufRow + x] |= mask;
  }
}

// Scanline fill of claimed territory (everything outside the perimeter polygon).
// Processes one row at a time — only ~32 bytes of stack for the intersection buffer.
void fillArea() {
  int xBuf[16]; // intersection buffer per scanline — plenty for axis-aligned perimeters

  for (int y = 0; y < HEIGHT; y++) {
    int count = 0;

    // Find x-intersections of perimeter edges with this scanline
    for (int i = 0; i < perim.vertexCount && count < 16; i++) {
      int j = (i + 1) % perim.vertexCount;
      int y1 = perim.vertices[i].y;
      int y2 = perim.vertices[j].y;

      if (y1 == y2) continue; // horizontal edge — no intersection

      int yMin, yMax;
      if (y1 < y2) { yMin = y1; yMax = y2; }
      else          { yMin = y2; yMax = y1; }

      if (y >= yMin && y < yMax) { // half-open interval avoids double-count at vertices
        int x1 = perim.vertices[i].x;
        int x2 = perim.vertices[j].x;
        if (x1 == x2) {
          xBuf[count++] = x1; // vertical edge
        } else {
          // General case (safety for non-axis-aligned edges)
          xBuf[count++] = x1 + (int)((long)(x2 - x1) * (y - y1) / (y2 - y1));
        }
      }
    }

    // Insertion sort (count is very small for typical perimeters)
    for (int i = 1; i < count; i++) {
      int key = xBuf[i];
      int j = i - 1;
      while (j >= 0 && xBuf[j] > key) {
        xBuf[j + 1] = xBuf[j];
        j--;
      }
      xBuf[j + 1] = key;
    }

    // Even-odd rule: fill spans OUTSIDE the polygon
    // Inside spans:  [xBuf[0]..xBuf[1]], [xBuf[2]..xBuf[3]], ...
    // Outside spans: [0..xBuf[0]-1], [xBuf[1]+1..xBuf[2]-1], ..., [xBuf[last]+1..127]
    int x = 0;
    for (int i = 0; i < count; i += 2) {
      // Fill outside span before this inside span
      if (xBuf[i] > x) {
        fillHLine(x, xBuf[i] - 1, y);
      }
      // Advance past the inside span
      if (i + 1 < count) {
        x = xBuf[i + 1] + 1;
      } else {
        x = xBuf[i] + 1;
      }
    }
    // Fill remaining outside span to screen edge
    if (x < WIDTH) {
      fillHLine(x, WIDTH - 1, y);
    }
  }
}