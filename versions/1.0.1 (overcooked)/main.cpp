#include <Arduino.h>
#include <Arduboy2.h>
// WIDTH = 128
// HEIGHT = 64
#define NUM_LIVES 3
#define PLAYER_SIZE 2
#define FAST_MOVE 3  // Move every 1 frame
#define SLOW_MOVE 4  // Move every 2 frames
#define MAX_VERTICES 32
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

#define QIX_HISTORY_SIZE 6   // Store history for trailing ribbon effect
#define MAX_SPEED 2          // Maximum velocity magnitude

class qix {
  public:
    vertex p1, p2;       // two endpoints of the Qix line
    int v1x, v1y;        // velocity of endpoint 1
    int v2x, v2y;        // velocity of endpoint 2
    
    // Position history for trailing ribbon effect
    vertex historyP1[QIX_HISTORY_SIZE];
    vertex historyP2[QIX_HISTORY_SIZE];
    int historyIndex = 0;
    
    // Dynamic bounding box (changes shape over time)
    int boxLeft, boxRight, boxTop, boxBottom;
    int boxVelLeft, boxVelRight, boxVelTop, boxVelBottom;
    int boxPhase = 0;
    
    // Tuneable parameters
    int maxDist = 50;    // Maximum manhattan distance between endpoints
    int minDist = 15;    // Minimum manhattan distance between endpoints
    
    qix() {
      p1 = vertex(40, 20);
      p2 = vertex(70, 40);
      v1x = 2; v1y = 1;
      v2x = -1; v2y = 2;
      
      // Initialize history with current position
      for (int i = 0; i < QIX_HISTORY_SIZE; i++) {
        historyP1[i] = p1;
        historyP2[i] = p2;
      }
      
      // Initialize dynamic bounding box
      boxLeft = 10;
      boxRight = WIDTH - 10;
      boxTop = 6;
      boxBottom = HEIGHT - 6;
      boxVelLeft = 1;
      boxVelRight = -1;
      boxVelTop = 1;
      boxVelBottom = -1;
    }
    
    vertex qixCenter() {
      return vertex((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
    }
    
    // Get historical position for trail rendering
    vertex getHistP1(int age) {
      int idx = (historyIndex - 1 - age + QIX_HISTORY_SIZE) % QIX_HISTORY_SIZE;
      return historyP1[idx];
    }
    vertex getHistP2(int age) {
      int idx = (historyIndex - 1 - age + QIX_HISTORY_SIZE) % QIX_HISTORY_SIZE;
      return historyP2[idx];
    }
    
    void updateBox() {
      boxPhase++;
      
      // Move box edges
      boxLeft += boxVelLeft;
      boxRight += boxVelRight;
      boxTop += boxVelTop;
      boxBottom += boxVelBottom;
      
      // Screen limits and minimum box size
      int screenMinX = 3;
      int screenMaxX = WIDTH - 3;
      int screenMinY = 3;
      int screenMaxY = HEIGHT - 3;
      int minBoxWidth = 50;
      int minBoxHeight = 25;
      
      // Clamp and bounce
      if (boxLeft < screenMinX) { boxLeft = screenMinX; boxVelLeft = abs(boxVelLeft); }
      if (boxRight > screenMaxX) { boxRight = screenMaxX; boxVelRight = -abs(boxVelRight); }
      if (boxTop < screenMinY) { boxTop = screenMinY; boxVelTop = abs(boxVelTop); }
      if (boxBottom > screenMaxY) { boxBottom = screenMaxY; boxVelBottom = -abs(boxVelBottom); }
      
      // Ensure minimum size
      if (boxRight - boxLeft < minBoxWidth) {
        int center = (boxLeft + boxRight) / 2;
        boxLeft = center - minBoxWidth / 2;
        boxRight = center + minBoxWidth / 2;
        boxVelLeft = -boxVelLeft;
        boxVelRight = -boxVelRight;
      }
      if (boxBottom - boxTop < minBoxHeight) {
        int center = (boxTop + boxBottom) / 2;
        boxTop = center - minBoxHeight / 2;
        boxBottom = center + minBoxHeight / 2;
        boxVelTop = -boxVelTop;
        boxVelBottom = -boxVelBottom;
      }
      
      // Occasionally flip velocities for variety
      if ((boxPhase & 0x7F) == 0) boxVelLeft = -boxVelLeft;
      if ((boxPhase & 0x7F) == 40) boxVelRight = -boxVelRight;
      if ((boxPhase & 0x7F) == 80) boxVelTop = -boxVelTop;
      if ((boxPhase & 0x7F) == 120) boxVelBottom = -boxVelBottom;
    }
    
    void update() {
      // Store current position in history before moving
      historyP1[historyIndex] = p1;
      historyP2[historyIndex] = p2;
      historyIndex = (historyIndex + 1) % QIX_HISTORY_SIZE;
      
      // Update bounding box
      updateBox();
      
      // === AUTHENTIC QIX ALGORITHM ===
      // Randomly perturb velocities every frame (-1, 0, or +1)
      v1x += random(-1, 2);
      v1y += random(-1, 2);
      v2x += random(-1, 2);
      v2y += random(-1, 2);
      
      // Clamp to max speed
      v1x = constrain(v1x, -MAX_SPEED, MAX_SPEED);
      v1y = constrain(v1y, -MAX_SPEED, MAX_SPEED);
      v2x = constrain(v2x, -MAX_SPEED, MAX_SPEED);
      v2y = constrain(v2y, -MAX_SPEED, MAX_SPEED);
      
      // Avoid zero velocity (Qix gets stuck)
      if (v1x == 0 && v1y == 0) v1x = (random(2) == 0) ? 1 : -1;
      if (v2x == 0 && v2y == 0) v2y = (random(2) == 0) ? 1 : -1;
      
      // Line length constraint - progressive pull toward each other
      // Longer lines get stronger pull, making them less likely but not impossible
      int dx = p2.x - p1.x;
      int dy = p2.y - p1.y;
      int dist = abs(dx) + abs(dy);  // Manhattan distance
      
      // Progressive pull: probability of nudging increases with length
      // At 20px: ~25% chance, at 30px: ~50% chance, at 40px+: always
      int pullChance = (dist - 15) * 3;  // 0 at 15px, 15 at 20px, 45 at 30px, 75 at 40px
      if (dist > 15 && random(100) < pullChance) {
        // Nudge velocities toward each other
        if (dx > 0) { v1x++; v2x--; }
        else        { v1x--; v2x++; }
        if (dy > 0) { v1y++; v2y--; }
        else        { v1y--; v2y++; }
        // Re-clamp after nudge
        v1x = constrain(v1x, -MAX_SPEED, MAX_SPEED);
        v1y = constrain(v1y, -MAX_SPEED, MAX_SPEED);
        v2x = constrain(v2x, -MAX_SPEED, MAX_SPEED);
        v2y = constrain(v2y, -MAX_SPEED, MAX_SPEED);
      }
      
      // Soft push apart if too close
      if (dist < minDist) {
        if (dx > 0) { v1x--; v2x++; }
        else        { v1x++; v2x--; }
        if (dy > 0) { v1y--; v2y++; }
        else        { v1y++; v2y--; }
        v1x = constrain(v1x, -MAX_SPEED, MAX_SPEED);
        v1y = constrain(v1y, -MAX_SPEED, MAX_SPEED);
        v2x = constrain(v2x, -MAX_SPEED, MAX_SPEED);
        v2y = constrain(v2y, -MAX_SPEED, MAX_SPEED);
      }
      
      // Move endpoints
      p1.x += v1x;  p1.y += v1y;
      p2.x += v2x;  p2.y += v2y;

      // Bounce off the dynamic bounding box
      if (p1.x <= boxLeft)   { p1.x = boxLeft + 1;   v1x = abs(v1x); }
      if (p1.x >= boxRight)  { p1.x = boxRight - 1;  v1x = -abs(v1x); }
      if (p1.y <= boxTop)    { p1.y = boxTop + 1;    v1y = abs(v1y); }
      if (p1.y >= boxBottom) { p1.y = boxBottom - 1; v1y = -abs(v1y); }
      
      if (p2.x <= boxLeft)   { p2.x = boxLeft + 1;   v2x = abs(v2x); }
      if (p2.x >= boxRight)  { p2.x = boxRight - 1;  v2x = -abs(v2x); }
      if (p2.y <= boxTop)    { p2.y = boxTop + 1;    v2y = abs(v2y); }
      if (p2.y >= boxBottom) { p2.y = boxBottom - 1; v2y = -abs(v2y); }

      // Final hard clamp to screen
      p1.x = constrain(p1.x, 2, WIDTH - 3);
      p1.y = constrain(p1.y, 2, HEIGHT - 3);
      p2.x = constrain(p2.x, 2, WIDTH - 3);
      p2.y = constrain(p2.y, 2, HEIGHT - 3);
    }
};

player p;
perimeter perim;
qix q;
Arduboy2 arduboy;
uint16_t frameCounter = 0;

// Optimization flags
static bool perimDirty = true;       // Set when perimeter changes, triggers full redraw
static bool playerOnPerimeter = true; // Cached to avoid scanning every frame

// XOR state for Qix erase/redraw
static vertex qixOldP1(40, 20);
static vertex qixOldP2(80, 40);
static vertex qixOldHistP1[QIX_HISTORY_SIZE];
static vertex qixOldHistP2[QIX_HISTORY_SIZE];
static bool qixDrawn = false;

// Trail state for erase/redraw
static vertex lastTrail[MAX_VERTICES];
static int lastTrailCount = 0;
static vertex lastTrailEnd(WIDTH/2, HEIGHT - 1);  // Initialize to player start pos
static bool trailDrawn = false;

// Player state for erase/redraw
static vertex playerOldPos(WIDTH/2, HEIGHT - 1);
static bool playerDrawn = false;

// Draw mode speed tracking (stored on entry, read safely later)
static bool drawingSlowMode = false;

// Game state machine for fill animation
enum GameState {
  STATE_PLAY,
  STATE_FILLING,
  STATE_DEAD
};
static GameState gameState = STATE_PLAY;

// Fill animation state
struct FillAnimation {
  int currentY;       // scanline we're up to
  bool isSlow;        // determines fill pattern (solid vs stipple)
};
static FillAnimation fillAnim;
static vertex fillPoly[MAX_VERTICES];
static int fillPolyCount = 0;

// --- Sprite rendering system ---
// Full redraw each frame: clear, fill, draw perimeter, sprites.

// PROGMEM sprite bitmap (Arduboy2 Sprites format: width, height, data)
// Player: 5x5 diamond outline
const uint8_t playerSprite[] PROGMEM = {5, 5, 0x04, 0x0A, 0x11, 0x0A, 0x04};

void drawLine(vertex v1, vertex v2);
void drawDotLine(vertex v1, vertex v2);
void drawPerimeter();
void drawPlayer();
void erasePlayer();
void drawQix();
void eraseQix();
void drawTrail();
void eraseTrail();
void xorLine(int x1, int y1, int x2, int y2);
void drawScene();
void drawDebug();
byte getInput();
void updateActiveDirection(byte input);
void updateScreen();
void updatePlayer(byte input);
void movePlayer(byte allowedMoves);
void perimeterMove(byte input);
void updateDrawAllowance(byte input);
void updateCanMove();
void updatePerimIndex();
bool isInsidePerimeter(vertex point);
bool pointOnSegment(vertex point, vertex v1, vertex v2);
void drawMove(byte input, bool speed);
void updateCanDraw();
void updatePerim();
void fillArea();
void fillHLine(int x1, int x2, int y);
bool isInsidePolygon(vertex point, vertex* verts, int count);
bool stepFillAnimation();

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.clear();
  updateCanMove();
  drawScene(); // render initial background into buffer
}

void loop() {
  if (!arduboy.nextFrame()) return;
  frameCounter++;
  if (frameCounter >= 240) frameCounter = 0;

  // Handle fill animation state
  if (gameState == STATE_FILLING) {
    // Everything frozen — advance fill 3 scanlines per frame
    for (int i = 0; i < 3; i++) {
      if (stepFillAnimation()) {
        gameState = STATE_PLAY;
        perimDirty = true; // redraw background with fill baked in
        break;
      }
    }
    arduboy.display();
    return; // skip all game logic and other drawing
  }

  // Only do expensive full redraw when perimeter changed
  if (perimDirty) {
    arduboy.clear();
    fillArea();
    drawPerimeter();
    perimDirty = false;
    qixDrawn = false;  // Force qix redraw after clear
    trailDrawn = false;
    playerDrawn = false;
  } else {
    // Erase dynamic elements via XOR (toggles pixels back)
    eraseQix();
    eraseTrail();
    erasePlayer();
  }

  // Draw dynamic elements
  if (p.allowedMoves & 0x30) drawTrail();
  drawQix();
  drawPlayer();
  drawDebug();
  
  // Game logic
  updateScreen();
  
  arduboy.display();
}

void xorPlayerSprite(int cx, int cy) {
  // XOR the 5x5 diamond sprite centered at (cx, cy)
  static const uint8_t cols[5] = {0x04, 0x0A, 0x11, 0x0A, 0x04};
  uint8_t* buf = Arduboy2::sBuffer;
  
  for (int col = 0; col < 5; col++) {
    int x = cx - 2 + col;
    if (x < 0 || x >= WIDTH) continue;
    
    uint8_t pattern = cols[col];
    for (int bit = 0; bit < 5; bit++) {
      if (pattern & (1 << bit)) {
        int y = cy - 2 + bit;
        if (y >= 0 && y < HEIGHT) {
          buf[(y >> 3) * WIDTH + x] ^= (1 << (y & 7));
        }
      }
    }
  }
}

void drawPlayer() {
  xorPlayerSprite(p.position.x, p.position.y);
  playerOldPos = p.position;
  playerDrawn = true;
}

void erasePlayer() {
  if (!playerDrawn) return;
  xorPlayerSprite(playerOldPos.x, playerOldPos.y);
  playerDrawn = false;
}

void drawQix() {
  // Draw history lines for trailing ribbon effect (older = more sparse/stippled)
  for (int i = QIX_HISTORY_SIZE - 1; i >= 0; i--) {
    // Only draw every other historical frame for stipple effect
    if (i % 2 == 0) {
      vertex hp1 = q.getHistP1(i);
      vertex hp2 = q.getHistP2(i);
      xorLine(hp1.x, hp1.y, hp2.x, hp2.y);
      qixOldHistP1[i] = hp1;
      qixOldHistP2[i] = hp2;
    }
  }
  // Draw current line solid (always)
  xorLine(q.p1.x, q.p1.y, q.p2.x, q.p2.y);
  qixOldP1 = q.p1;
  qixOldP2 = q.p2;
  qixDrawn = true;
}

void eraseQix() {
  if (!qixDrawn) return;
  // XOR the old qix lines to erase them (toggles pixels back)
  xorLine(qixOldP1.x, qixOldP1.y, qixOldP2.x, qixOldP2.y);
  for (int i = QIX_HISTORY_SIZE - 1; i >= 0; i--) {
    if (i % 2 == 0) {
      xorLine(qixOldHistP1[i].x, qixOldHistP1[i].y, qixOldHistP2[i].x, qixOldHistP2[i].y);
    }
  }
  qixDrawn = false;
}

// Bresenham line drawn via XOR into sBuffer
void xorLine(int x1, int y1, int x2, int y2) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;
  uint8_t* buf = Arduboy2::sBuffer;
  while (true) {
    if (x1 >= 0 && x1 < WIDTH && y1 >= 0 && y1 < HEIGHT) {
      buf[(y1 >> 3) * WIDTH + x1] ^= (1 << (y1 & 7));
    }
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x1 += sx; }
    if (e2 < dx) { err += dx; y1 += sy; }
  }
}

void updateScreen() {
  byte input = getInput();
  updateActiveDirection(input);
  updatePlayer(input);
  // Update Qix at 30fps (every other frame)
  if (frameCounter % 2 == 0) {
    q.update();
  }
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
  updateDrawAllowance(input);
  if (p.allowedMoves & 0x30) {
    if ((input & 0x10) && (p.allowedMoves & 0x10)) {
      drawMove(input, true);
    } else if ((input & 0x20) && (p.allowedMoves & 0x20)) {
      drawMove(input, false);
    }
    // Trail drawn in loop(), not here
  } else if (input & 0x0F) {
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
        // Update the perimeter shape with the trail (enters fill animation)
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
  playerOnPerimeter = true;  // Player is on perimeter when this is called
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
  if (p.allowedMoves & 0x30) return;
  
  // Not in draw mode: only enter if a draw button is actually pressed
  if (!(input & 0x30)) return;
  
  // Check if active direction is not allowed by perimeter movement
  if (p.activeDir == 0 || (p.activeDir & p.allowedMoves) != 0) return;

  // Use cached flag — updated by updateCanMove() each frame
  if (!playerOnPerimeter) return;

  // Calculate next position based on the active direction
  vertex nextPos = p.position;
  if (p.activeDir & 0x01) nextPos.x -= 1;  // left
  else if (p.activeDir & 0x02) nextPos.x += 1;  // right
  if (p.activeDir & 0x04) nextPos.y -= 1;  // up
  else if (p.activeDir & 0x08) nextPos.y += 1;  // down

  // Next position must be strictly inside, not on the boundary
  if (!isInsidePerimeter(nextPos)) return;
  
  // Also reject if next position happens to land on a perimeter edge
  // (would immediately re-trigger completion logic)
  for (int i = 0; i < perim.vertexCount; i++) {
    vertex v1 = perim.vertices[i];
    vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
    if (pointOnSegment(nextPos, v1, v2)) return;
  }

  // Set only the bit corresponding to the button being pressed
  p.allowedMoves |= (input & 0x30);
  playerOnPerimeter = false; // No longer on perimeter once drawing begins
  drawingSlowMode = (input & 0x20) != 0; // Store speed for fill animation later
  
  // Determine the directed perimeter edge (i -> i+1) that contains the player
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
  p.lastTrailDir = p.activeDir;
}

void drawLine(vertex v1, vertex v2) {
  arduboy.drawLine(v1.x, v1.y, v2.x, v2.y);
}

void drawDotLine(vertex v1, vertex v2) {
  // Integer-only dotted line for axis-aligned segments (no float on AVR)
  int dx = (v2.x > v1.x) ? 1 : (v2.x < v1.x) ? -1 : 0;
  int dy = (v2.y > v1.y) ? 1 : (v2.y < v1.y) ? -1 : 0;
  int x = v1.x, y = v1.y;
  int steps = max(abs(v2.x - v1.x), abs(v2.y - v1.y));
  for (int i = 0; i <= steps; i++) {
    if (i % 2 == 0) arduboy.drawPixel(x, y);
    x += dx;
    y += dy;
  }
}

void drawScene() {
  // Full background redraw: clear, fill claimed territory, draw perimeter
  arduboy.clear();
  fillArea();
  drawPerimeter();
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
  // Draw the player's trail using XOR so we can erase it next frame.
  // Save current state for erasing.
  lastTrailCount = p.trailCount;
  lastTrailEnd = p.position;
  for (int i = 0; i < p.trailCount && i < MAX_VERTICES; i++) {
    lastTrail[i] = p.trail[i];
  }
  
  // Draw trail segments with XOR
  for (int i = 1; i < p.trailCount; i++) {
    vertex a = p.trail[i - 1], b = p.trail[i];
    xorLine(a.x, a.y, b.x, b.y);
  }
  if (p.trailCount > 0) {
    vertex a = p.trail[p.trailCount - 1], b = p.position;
    xorLine(a.x, a.y, b.x, b.y);
  }
  trailDrawn = (p.trailCount > 0);
}

void eraseTrail() {
  // XOR-erase the trail using saved state
  if (!trailDrawn) return;
  for (int i = 1; i < lastTrailCount; i++) {
    vertex a = lastTrail[i - 1], b = lastTrail[i];
    xorLine(a.x, a.y, b.x, b.y);
  }
  if (lastTrailCount > 0) {
    vertex a = lastTrail[lastTrailCount - 1], b = lastTrailEnd;
    xorLine(a.x, a.y, b.x, b.y);
  }
  trailDrawn = false;
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

  // Detect same-edge case: player entered and exited on the same perimeter edge
  bool sameEdge = (p.drawStartIndex[0] == p.drawEndIndex[0] &&
                   p.drawStartIndex[1] == p.drawEndIndex[1]);

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

  vertex qc = q.qixCenter();
  bool qixInA = (countA >= 3) ? isInsidePolygon(qc, scratch, countA) : false;

  if (!qixInA) {
    // --- Option B: trail reversed + other arc (or no arc if same edge) ---
    int countB = 0;
    for (int i = p.trailCount - 1; i >= 0 && countB < MAX_VERTICES; i--) {
      if (countB > 0 && compareVertices(scratch[countB - 1], p.trail[i])) continue;
      scratch[countB++] = p.trail[i];
    }
    if (!sameEdge) {
      // Only walk perimeter arc if start and end are on different edges
      idx = p.drawStartIndex[1];
      stop = p.drawEndIndex[0];
      for (int safety = 0; safety < perim.vertexCount && countB < MAX_VERTICES; safety++) {
        if (countB == 0 || !compareVertices(scratch[countB - 1], perim.vertices[idx])) {
          scratch[countB++] = perim.vertices[idx];
        }
        if (idx == stop) break;
        idx = (idx + 1) % perim.vertexCount;
      }
    }
    if (countB > 1 && compareVertices(scratch[0], scratch[countB - 1])) {
      countB--;
    }
    countA = countB; // reuse countA for the final copy
  }

  if (countA < 3) return; // defensive: don't corrupt perimeter with degenerate polygon

  // Build the fill polygon (the discarded region we're animating)
  // This is the trail + the opposite arc from what we're keeping
  fillPolyCount = 0;
  // Trail forward
  for (int i = 0; i < p.trailCount && fillPolyCount < MAX_VERTICES; i++) {
    if (fillPolyCount > 0 && compareVertices(fillPoly[fillPolyCount - 1], p.trail[i])) continue;
    fillPoly[fillPolyCount++] = p.trail[i];
  }
  // The arc we DIDN'T use for the kept polygon (skip if same-edge cut)
  if (!sameEdge) {
    int fidx  = qixInA ? p.drawStartIndex[1] : p.drawEndIndex[1];
    int fstop = qixInA ? p.drawEndIndex[0]   : p.drawStartIndex[0];
    for (int safety = 0; safety < perim.vertexCount && fillPolyCount < MAX_VERTICES; safety++) {
      if (fillPolyCount == 0 || !compareVertices(fillPoly[fillPolyCount - 1], perim.vertices[fidx])) {
        fillPoly[fillPolyCount++] = perim.vertices[fidx];
      }
      if (fidx == fstop) break;
      fidx = (fidx + 1) % perim.vertexCount;
    }
  }
  if (fillPolyCount > 1 && compareVertices(fillPoly[0], fillPoly[fillPolyCount - 1])) {
    fillPolyCount--;
  }

  // Copy the chosen polygon back as the new perimeter
  perim.vertexCount = countA;
  for (int i = 0; i < countA; i++) {
    perim.vertices[i] = scratch[i];
  }

  // Enter fill animation state
  fillAnim.currentY = 0;
  fillAnim.isSlow = drawingSlowMode;
  gameState = STATE_FILLING;
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

    // Guard: skip scanline if odd intersection count (degenerate vertex case)
    if (count & 1) continue;

    // Even-odd rule: fill spans OUTSIDE the polygon
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

// Progressive fill animation — fills one scanline per call
// Returns true when fill is complete
bool stepFillAnimation() {
  if (fillAnim.currentY >= HEIGHT) return true;

  int y = fillAnim.currentY;
  int xBuf[16];
  int count = 0;

  // Find x-intersections with the fill polygon
  for (int i = 0; i < fillPolyCount && count < 16; i++) {
    int j = (i + 1) % fillPolyCount;
    int y1 = fillPoly[i].y;
    int y2 = fillPoly[j].y;
    if (y1 == y2) continue;
    int yMin = (y1 < y2) ? y1 : y2;
    int yMax = (y1 < y2) ? y2 : y1;
    if (y >= yMin && y < yMax) {
      int x1 = fillPoly[i].x;
      int x2 = fillPoly[j].x;
      xBuf[count++] = (x1 == x2) ? x1 : x1 + (int)((long)(x2 - x1) * (y - y1) / (y2 - y1));
    }
  }

  // Insertion sort
  for (int i = 1; i < count; i++) {
    int key = xBuf[i], j = i - 1;
    while (j >= 0 && xBuf[j] > key) { xBuf[j + 1] = xBuf[j]; j--; }
    xBuf[j + 1] = key;
  }

  // Fill inside the polygon (even-odd rule)
  if (!(count & 1)) {
    for (int i = 0; i + 1 < count; i += 2) {
      if (fillAnim.isSlow) {
        // Solid fill for slow draw (2x points)
        fillHLine(xBuf[i], xBuf[i + 1], y);
      } else {
        // Checkerboard stipple for fast draw
        for (int x = xBuf[i]; x <= xBuf[i + 1]; x++) {
          if ((x ^ y) & 1) {
            // Single pixel fill
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
              Arduboy2::sBuffer[(y >> 3) * WIDTH + x] |= (1 << (y & 7));
            }
          }
        }
      }
    }
  }

  fillAnim.currentY++;
  return (fillAnim.currentY >= HEIGHT);
}