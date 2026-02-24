#include "player_logic.h"
#include "geometry.h"
#include "rendering.h"

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
    // Fast move
    input |= 0x10; // set bit 4 for fast move
  }
  if (arduboy.pressed(B_BUTTON)) {
    // Slow move
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
    p.setActiveDir(newlyPressed);
  } else if ((p.getActiveDir() & currentDir) == 0) {
    // Active direction was released, switch to any remaining pressed button
    p.setActiveDir(currentDir);
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
      // Fast move
      drawMove(input, true);
    } else if ((input & 0x20) && (p.allowedMoves & 0x20)) {
      // Slow move
      drawMove(input, false);
    }
  } else if (input & 0x0F) {
    // Perimeter move
    perimeterMove(input);
  }
}

void drawMove(byte input, bool speed) {
  byte interval = speed ? FAST_MOVE : SLOW_MOVE;
  // Only move when frameCounter is divisible by the interval
  if (frameCounter % interval == 0) {
    // Only update trail direction and move if activeDir is actually allowed
    if (p.getActiveDir() & p.allowedMoves) {
      // Check if direction changed BEFORE moving - add current position as corner
      if (p.getLastTrailDir() != 0 && p.getActiveDir() != p.getLastTrailDir()) {
        p.addTrailVertex(p.position);
      }
      p.setLastTrailDir(p.getActiveDir());
      movePlayer(p.allowedMoves);
    }
    
    // Check if player has returned to the perimeter (only after moving away from start)
    if (p.trailCount > 0 &&
        (p.position.getx() != p.trail[0].getx() || p.position.gety() != p.trail[0].gety())) {
      for (int i = 0; i < perim.vertexCount; i++) {
        vertex v1 = perim.vertices[i];
        vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
        if (pointOnSegment(p.position, v1, v2)) {
          // Player is back on perimeter, store end indices
          // If we re-enter exactly at a corner, normalize indices to that single vertex.
          int endA = i;
          int endB = (i + 1) % perim.vertexCount;
          if (p.position.getx() == v1.getx() && p.position.gety() == v1.gety()) {
            endA = i;
            endB = i;
          } else if (p.position.getx() == v2.getx() && p.position.gety() == v2.gety()) {
            endA = endB;
            endB = endB;
          }
          p.setDrawEndIndex(0, endA);
          p.setDrawEndIndex(1, endB);
          // Add end position as final trail vertex
          p.addTrailVertex(p.position);
          // Update the perimeter shape with the trail
          updatePerim();
          // Re-find player position on the new perimeter
          for (int j = 0; j < perim.vertexCount; j++) {
            vertex nv1 = perim.vertices[j];
            vertex nv2 = perim.vertices[(j + 1) % perim.vertexCount];
            if (pointOnSegment(p.position, nv1, nv2)) {
              p.setPerimIndex(0, j);
              p.setPerimIndex(1, (j + 1) % perim.vertexCount);
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

void updateCanDraw() {
  byte allowedMoves = 0x0F;

  // Block the opposite of the current travel direction (not input direction)
  byte lastDir = p.getLastTrailDir();
  if (lastDir & 0x01) allowedMoves &= ~0x02; // traveling left, block right
  else if (lastDir & 0x02) allowedMoves &= ~0x01; // traveling right, block left
  if (lastDir & 0x04) allowedMoves &= ~0x08; // traveling up, block down
  else if (lastDir & 0x08) allowedMoves &= ~0x04; // traveling down, block up

  // For each of the 4 directions, check if moving there would land on a trail segment
  byte dirs[4] = {0x01, 0x02, 0x04, 0x08};
  int dx[4] = {-1, 1, 0, 0};
  int dy[4] = {0, 0, -1, 1};

  for (int d = 0; d < 4; d++) {
    if (!(allowedMoves & dirs[d])) continue; // already blocked
    vertex nextPos = p.position;
    nextPos.addx(dx[d]);
    nextPos.addy(dy[d]);

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
  byte activeDir = p.getActiveDir();
  if (activeDir & allowedMoves) {
    // Move player based on activeDir (only one direction per frame)
    if (activeDir & 0x01) {
      // Move left
      p.position.addx(-1);
    } else if (activeDir & 0x02) {
      // Move right
      p.position.addx(1);
    } else if (activeDir & 0x04) {
      // Move up
      p.position.addy(-1);
    } else if (activeDir & 0x08) {
      // Move down
      p.position.addy(1);
    }
  }
}

void perimeterMove(byte input) {
  // Only move when frameCounter is divisible by FAST_MOVE interval
  if (frameCounter % FAST_MOVE == 0) {
    // Call movePlayer with perimeter constraints
    vertex prevPos = p.position;
    movePlayer(p.allowedMoves);
    if (p.position.getx() != prevPos.getx() || p.position.gety() != prevPos.gety()) {
      updatePerimIndex();
      updateCanMove();
    }
  }
}

void updatePerimIndex() {
  int idxA = p.getPerimIndex(0);
  int idxB = p.getPerimIndex(1);
  vertex v1 = perim.vertices[idxA];
  vertex v2 = perim.vertices[idxB];
  int prevIdx = (idxA - 1 + perim.vertexCount) % perim.vertexCount;
  int nextIdx = (idxB + 1) % perim.vertexCount;

  if (p.position.getx() == v1.getx() && p.position.gety() == v1.gety()) {
    p.setPerimIndex(0, idxA);
    p.setPerimIndex(1, idxA);
    return;
  }
  if (p.position.getx() == v2.getx() && p.position.gety() == v2.gety()) {
    p.setPerimIndex(0, idxB);
    p.setPerimIndex(1, idxB);
    return;
  }

  // If player moved off the current edge (or corner), pick the adjacent edge.
  if (!pointOnSegment(p.position, v1, v2)) {
    vertex prevV = perim.vertices[prevIdx];
    if (pointOnSegment(p.position, v1, prevV)) {
      p.setPerimIndex(0, idxA);
      p.setPerimIndex(1, prevIdx);
    } else {
      vertex nextV = perim.vertices[nextIdx];
      if (pointOnSegment(p.position, v2, nextV)) {
        p.setPerimIndex(0, nextIdx);
        p.setPerimIndex(1, idxB);
      } else {
        // Fallback: full scan after perimeter restructure
        for (int i = 0; i < perim.vertexCount; i++) {
          vertex a = perim.vertices[i];
          vertex b = perim.vertices[(i + 1) % perim.vertexCount];
          if (pointOnSegment(p.position, a, b)) {
            p.setPerimIndex(0, i);
            p.setPerimIndex(1, (i + 1) % perim.vertexCount);
            return;
          }
        }
      }
    }
  }
}

void updateCanMove() {
  p.allowedMoves = 0;
  byte idx0 = p.getPerimIndex(0);
  byte idx1 = p.getPerimIndex(1);
  vertex v1 = perim.vertices[idx0];
  vertex v2 = perim.vertices[idx1];
  
  // Get adjacent vertices for corner handling
  int prevIdx = (idx0 - 1 + perim.vertexCount) % perim.vertexCount;
  int nextIdx = (idx1 + 1) % perim.vertexCount;
  vertex prevV = perim.vertices[prevIdx];
  vertex nextV = perim.vertices[nextIdx];

  if (idx0 == idx1) {
    // At a corner, allow movement along both adjacent edges
    p.allowedMoves = getDirection(v1, prevV) | getDirection(v1, nextV);
  } else {
    // On edge between v1 and v2 - can move toward either endpoint
    p.allowedMoves = getDirection(p.position, v1) | getDirection(p.position, v2);
  }
}

void updateDrawAllowance(byte input) {
  if (p.allowedMoves & 0x30) {
    // Preserve the existing draw mode bit
    return;
  }
  if (!(input & 0x30)) return;
  
  // Check if active direction is not allowed by perimeter movement
  byte activeDir = p.getActiveDir();
  if (activeDir != 0 && (activeDir & p.allowedMoves) == 0) {
    vertex nextPos = p.position;
    
    // Check each individual direction bit
    if (activeDir & 0x01) nextPos.addx(-1);  // left
    else if (activeDir & 0x02) nextPos.addx(1);  // right
    
    if (activeDir & 0x04) nextPos.addy(-1);  // up
    else if (activeDir & 0x08) nextPos.addy(1);  // down
    
    // Only allow draw mode if next position is inside the perimeter
    if (isInsidePolygon(nextPos, perim.vertices, perim.vertexCount)) {
      // Set only the bit corresponding to the button being pressed
      p.allowedMoves |= (input & 0x30);
      // Initialize trail when entering draw mode (one-shot)
      // Determine the directed perimeter edge (i -> i+1) that contains the player
      // so arc cutting is consistent even if perimIndex ordering flips.
      for (int i = 0; i < perim.vertexCount; i++) {
        vertex v1 = perim.vertices[i];
        vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
        if (pointOnSegment(p.position, v1, v2)) {
          p.setDrawStartIndex(0, i);
          p.setDrawStartIndex(1, (i + 1) % perim.vertexCount);
          break;
        }
      }
      p.trailCount = 0;
      p.addTrailVertex(p.position);
      p.setLastTrailDir(activeDir); // Set initial direction
    }
  }
}

void updatePerim() {
  // Determine which arc to fill (the one NOT containing the Qix)
  int startIdx = p.getDrawStartIndex(0);
  int endIdx = p.getDrawEndIndex(0);
  int arcLen = (endIdx - startIdx + perim.vertexCount) % perim.vertexCount;
  
  if (arcLen == 0) {
    p.trailCount = 0;
    return; // Player re-entered at same point
  }
  
  // Test if Qix is inside the forward arc using ray-casting
  // Cast a horizontal ray from Qix to the right; count intersections with arc + trail
  int testY = q.position.gety();
  int xCount = 0;
  
  // Count intersections with forward arc
  int i = startIdx;
  while (i != endIdx) {
    int next = (i + 1) % perim.vertexCount;
    vertex v1 = perim.vertices[i];
    vertex v2 = perim.vertices[next];
    
    int minY = min(v1.gety(), v2.gety());
    int maxY = max(v1.gety(), v2.gety());
    if (testY >= minY && testY < maxY && v1.getx() != v2.getx()) {
      int x = v1.getx() + (testY - v1.gety()) * (v2.getx() - v1.getx()) / (v2.gety() - v1.gety());
      if (x >= q.position.getx()) {
        xCount++;
      }
    }
    i = next;
  }
  
  // Count intersections with trail (boundary of claimed territory)
  for (int t = 0; t < p.trailCount - 1; t++) {
    vertex v1 = p.trail[t];
    vertex v2 = p.trail[t + 1];
    int minY = min(v1.gety(), v2.gety());
    int maxY = max(v1.gety(), v2.gety());
    if (testY >= minY && testY < maxY && v1.getx() != v2.getx()) {
      int x = v1.getx() + (testY - v1.gety()) * (v2.getx() - v1.getx()) / (v2.gety() - v1.gety());
      if (x >= q.position.getx()) {
        xCount++;
      }
    }
  }
  
  // If odd count, Qix is inside forward arc; fill forward arc (dir=1) instead
  int arcDir = (xCount % 2 == 1) ? 1 : -1;
  
  // Fill using virtual iteration — no new array needed
  for (int y = 0; y < HEIGHT; y++) {
    int xs[16];
    int xCount = 0;
    findIntersections(y, startIdx, endIdx, arcDir, xs, xCount);
    
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
    
    // Fill between pairs
    for (int a = 0; a + 1 < xCount; a += 2) {
      arduboy.drawFastHLine(xs[a], y, xs[a + 1] - xs[a]);
    }
  }
  
  // Splice trail into perimeter, replacing the arc we just filled
  int newCount = perim.vertexCount - arcLen + p.trailCount;
  
  if (newCount <= MAX_VERTICES) {
    if (arcDir == 1) {
      // Forward arc does NOT contain Qix: remove [startIdx..endIdx), insert trail
      // Result: [0..startIdx) + trail + [endIdx..vertexCount)
      
      int tailLen = perim.vertexCount - endIdx;
      
      // Move tail [endIdx..vertexCount) to position (startIdx + p.trailCount)
      if (tailLen > 0) {
        // Move in reverse if expanding to avoid overwriting source
        if (p.trailCount > arcLen) {
          for (int j = tailLen - 1; j >= 0; j--) {
            perim.vertices[startIdx + p.trailCount + j] = perim.vertices[endIdx + j];
          }
        } else {
          for (int j = 0; j < tailLen; j++) {
            perim.vertices[startIdx + p.trailCount + j] = perim.vertices[endIdx + j];
          }
        }
      }
      
      // Copy trail into [startIdx..startIdx + p.trailCount)
      for (int t = 0; t < p.trailCount; t++) {
        perim.vertices[startIdx + t] = p.trail[t];
      }
      
    } else {
      // Backward arc does NOT contain Qix: keep [startIdx..endIdx), replace everything else with trail
      // Result: trail + [startIdx..endIdx)
      
      int forwardArcLen = endIdx - startIdx;
      
      // Move [startIdx..endIdx) to [p.trailCount..p.trailCount + forwardArcLen)
      // Move in reverse to avoid overwriting while copying
      for (int j = forwardArcLen - 1; j >= 0; j--) {
        perim.vertices[p.trailCount + j] = perim.vertices[startIdx + j];
      }
      
      // Copy trail into [0..p.trailCount)
      for (int t = 0; t < p.trailCount; t++) {
        perim.vertices[t] = p.trail[t];
      }
    }
  }
  
  perim.vertexCount = newCount;
  p.trailCount = 0;
}

bool isQixInsidePerimeter() {
  return isInsidePolygon(q.position, perim.vertices, perim.vertexCount);
}
