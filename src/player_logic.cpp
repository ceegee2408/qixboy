#include "player_logic.h"
#include "geometry.h"
#include "rendering.h"

// Set to true when a draw completes while A/B is still held, so
// updateDrawAllowance blocks re-entry until the button is released.
static bool drawCooldown = false;

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
          // Re-find player position on the new perimeter — check exact corners first
          bool found = false;
          for (int j = 0; j < perim.vertexCount; j++) {
            vertex nv1 = perim.vertices[j];
            vertex nv2 = perim.vertices[(j + 1) % perim.vertexCount];
            // Exact vertex match -> collapse to corner
            if (compareVertices(p.position, nv1)) {
              p.setPerimIndex(0, j);
              p.setPerimIndex(1, j);
              found = true;
              break;
            }
            int nextIdx = (j + 1) % perim.vertexCount;
            if (compareVertices(p.position, nv2)) {
              p.setPerimIndex(0, nextIdx);
              p.setPerimIndex(1, nextIdx);
              found = true;
              break;
            }
            // Mid-edge match
            if (pointOnSegment(p.position, nv1, nv2)) {
              p.setPerimIndex(0, j);
              p.setPerimIndex(1, nextIdx);
              found = true;
              break;
            }
          }
          // If we couldn't re-find the player on the new perimeter, clear draw
          // mode bits so we don't remain stuck in draw mode.
          gameState = FILL_ANIMATION;
          initializeFill();
          if (!found) {
            p.allowedMoves &= ~0x30; // clear draw mode bits
            drawCooldown = true;     // prevent immediate re-entry if A/B still held
            updateCanMove();
            return;
          }
          // Perimeter indices are normalized above — compute allowed moves
          drawCooldown = true;       // prevent immediate re-entry if A/B still held
          updateCanMove();
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

  byte lastDir = p.getLastTrailDir();

  // Compute reverse of last trail direction — always blocked (prevents 180s)
  byte reverseDir = 0;
  if (lastDir == 0x01) reverseDir = 0x02;
  else if (lastDir == 0x02) reverseDir = 0x01;
  else if (lastDir == 0x04) reverseDir = 0x08;
  else if (lastDir == 0x08) reverseDir = 0x04;
  allowedMoves &= ~reverseDir;

  // Block U-turns: if the current segment is < 3 pixels, block the reverse
  // of the *previous* segment's direction (prevents tight parallel lines).
  if (p.trailCount >= 2) {
    int curSegLen = vertexDistance(p.position, p.trail[p.trailCount - 1]);
    if (curSegLen < 3) {
      byte prevSegDir = getDirection(p.trail[p.trailCount - 2], p.trail[p.trailCount - 1]);
      byte reversePrevDir = 0;
      if (prevSegDir == 0x01) reversePrevDir = 0x02;
      else if (prevSegDir == 0x02) reversePrevDir = 0x01;
      else if (prevSegDir == 0x04) reversePrevDir = 0x08;
      else if (prevSegDir == 0x08) reversePrevDir = 0x04;
      allowedMoves &= ~reversePrevDir;
    }
  }

  byte dirs[4] = {0x01, 0x02, 0x04, 0x08};
  int dx[4] = {-1, 1, 0, 0};
  int dy[4] = {0, 0, -1, 1};

  for (int d = 0; d < 4; d++) {
    if (!(allowedMoves & dirs[d])) continue; // already blocked

    vertex nextPos = p.position;
    nextPos.addx(dx[d]);
    nextPos.addy(dy[d]);

    // If next position is on the perimeter, always allow (don't block re-entry)
    bool onPerim = false;
    for (int i = 0; i < perim.vertexCount; i++) {
      if (pointOnSegment(nextPos, perim.vertices[i],
                         perim.vertices[(i + 1) % perim.vertexCount])) {
        onPerim = true;
        break;
      }
    }
    if (onPerim) continue;

    // Check 1 step ahead against stored trail segments
    bool blocked = false;
    for (int i = 0; i < p.trailCount - 1; i++) {
      if (pointOnSegment(nextPos, p.trail[i], p.trail[i + 1])) {
        blocked = true;
        break;
      }
    }
    // Check 1 step ahead against live segment (last corner → current position)
    if (!blocked && p.trailCount > 0) {
      if (pointOnSegment(nextPos, p.trail[p.trailCount - 1], p.position)) {
        blocked = true;
      }
    }

    // Check 2 steps ahead against trail segments (prevents drawing adjacent to trail)
    if (!blocked) {
      vertex nextPos2 = nextPos;
      nextPos2.addx(dx[d]);
      nextPos2.addy(dy[d]);
      for (int i = 0; i < p.trailCount - 1; i++) {
        if (pointOnSegment(nextPos2, p.trail[i], p.trail[i + 1])) {
          blocked = true;
          break;
        }
      }
      if (!blocked && p.trailCount > 0) {
        if (pointOnSegment(nextPos2, p.trail[p.trailCount - 1], p.position)) {
          blocked = true;
        }
      }
    }

    if (blocked) allowedMoves &= ~dirs[d];
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

  // Final corner-collapse: if we landed exactly on one of the tracked vertices
  // (e.g. after a 1- or 2-pixel adjacent-edge transition), collapse both indices
  // to that vertex so updateCanMove() doesn't compute getDirection(pos, pos) == 0.
  {
    int fi0 = p.getPerimIndex(0);
    int fi1 = p.getPerimIndex(1);
    vertex w0 = perim.vertices[fi0];
    vertex w1 = perim.vertices[fi1];
    if (p.position.getx() == w0.getx() && p.position.gety() == w0.gety()) {
      p.setPerimIndex(1, fi0);
    } else if (p.position.getx() == w1.getx() && p.position.gety() == w1.gety()) {
      p.setPerimIndex(0, fi1);
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
    // Already in draw mode — preserve the existing bit.
    return;
  }
  if (!(input & 0x30)) {
    // A/B released — reset cooldown so next press works normally.
    drawCooldown = false;
    return;
  }
  // Block re-entry for the remainder of the held press after a draw completes.
  if (drawCooldown) return;
  
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
      p.allowedMoves |= (input & 0x30);
      // Initialize trail when entering draw mode (one-shot)
      // Determine the directed perimeter edge (i -> i+1) that contains the player
      // so arc cutting is consistent even if perimIndex ordering flips.
      for (int i = 0; i < perim.vertexCount; i++) {
        vertex v1 = perim.vertices[i];
        vertex v2 = perim.vertices[(i + 1) % perim.vertexCount];
        if (pointOnSegment(p.position, v1, v2)) {
          int sA = i;
          int sB = (i + 1) % perim.vertexCount;
          // Collapse to single index if exactly at a corner vertex
          if (p.position.getx() == v1.getx() && p.position.gety() == v1.gety()) {
            sB = sA;
          } else if (p.position.getx() == v2.getx() && p.position.gety() == v2.gety()) {
            sA = sB;
          }
          p.setDrawStartIndex(0, sA);
          p.setDrawStartIndex(1, sB);
          break;
        }
      }
      p.trailCount = 0;
      p.addTrailVertex(p.position);
      p.setLastTrailDir(activeDir); // Set initial direction
    }
  }
}

void reverseVertices(vertex* verts, int count) {
  for (int i = 0; i < count / 2; i++) {
    vertex temp = verts[i];
    verts[i] = verts[count - 1 - i];
    verts[count - 1 - i] = temp;
  }
}

void updatePerim() {
  if (p.trailCount < 2) {
    p.trailCount = 0;
    return;
  }

  int startIdx     = p.getDrawStartIndex(0);
  int startIdxNext = p.getDrawStartIndex(1);
  int endIdx       = p.getDrawEndIndex(0);
  int endIdxNext   = p.getDrawEndIndex(1);

  // Normalize so startIdx <= endIdx. If we swap, reverse the trail
  // so trail[0] is still the vertex adjacent to perim[startIdx].
  if (endIdx < startIdx) {
    int tmp;
    tmp = startIdx;      startIdx     = endIdx;      endIdx     = tmp;
    tmp = startIdxNext;  startIdxNext = endIdxNext;   endIdxNext = tmp;
    reverseVertices(p.trail, p.trailCount);
  }
  if (startIdx == endIdx) {
  vertex edgeV1 = perim.vertices[startIdx];
  vertex edgeV2 = perim.vertices[startIdxNext];
  bool needFlip;
  if (edgeV1.getx() != edgeV2.getx()) {
    // Horizontal edge: normalize by x
    if (edgeV1.getx() < edgeV2.getx())
      needFlip = p.trail[0].getx() > p.trail[p.trailCount - 1].getx();
    else
      needFlip = p.trail[0].getx() < p.trail[p.trailCount - 1].getx();
  } else {
    // Vertical edge: normalize by y
    if (edgeV1.gety() < edgeV2.gety())
      needFlip = p.trail[0].gety() > p.trail[p.trailCount - 1].gety();
    else
      needFlip = p.trail[0].gety() < p.trail[p.trailCount - 1].gety();
  }
  if (needFlip) reverseVertices(p.trail, p.trailCount);
  }

  // After normalization:
  //   trail[0]    sits on edge perim[startIdx] → perim[startIdxNext]
  //   trail[last] sits on edge perim[endIdx]   → perim[endIdxNext]
  // When the player enters/exits exactly at a corner vertex the two
  // indices for that end collapse to the same value.

  // Build test polygon: trail + backward arc (endIdxNext → … → startIdx)
  // The arc starts one vertex past the exit edge and ends at the near
  // vertex of the entry edge, so the polygon closes along the perimeter
  // with no corner-cutting.
  vertex scratch[MAX_VERTICES];
  int writeIdx = 0;

  for (int i = 0; i < p.trailCount && writeIdx < MAX_VERTICES; i++) {
    scratch[writeIdx++] = p.trail[i];
  }
  int idx = endIdxNext;
  // Skip first arc vertex if it duplicates trail's last vertex (corner exit)
  if (endIdx == endIdxNext) {
    idx = (idx + 1) % perim.vertexCount;
  }
  while (idx != startIdx && writeIdx < MAX_VERTICES) {
    scratch[writeIdx++] = perim.vertices[idx];
    idx = (idx + 1) % perim.vertexCount;
  }
  // Connector vertex — skip if it duplicates trail[0] (corner entry)
  if (startIdx != startIdxNext && writeIdx < MAX_VERTICES) {
    scratch[writeIdx++] = perim.vertices[startIdx];
  }

  bool qixInTestPoly = isInsidePolygon(q.position, scratch, writeIdx);

  if (qixInTestPoly) {
    // Backward arc polygon is already in scratch — reuse it.
  } else {
    // Keep forward arc: startIdxNext → … → endIdx, then reversed trail.
    writeIdx = 0;
    idx = startIdxNext;
    // Skip first arc vertex if it duplicates trail[0] (corner entry)
    if (startIdx == startIdxNext) {
      idx = (idx + 1) % perim.vertexCount;
    }
    while (idx != endIdx && writeIdx < MAX_VERTICES) {
      scratch[writeIdx++] = perim.vertices[idx];
      idx = (idx + 1) % perim.vertexCount;
    }
    // Include endIdx connector — skip if it duplicates reversed trail's first vertex (corner exit)
    if (endIdx != endIdxNext && writeIdx < MAX_VERTICES) {
      scratch[writeIdx++] = perim.vertices[endIdx];
    }
    for (int i = p.trailCount - 1; i >= 0 && writeIdx < MAX_VERTICES; i--) {
      scratch[writeIdx++] = p.trail[i];
    }
  }

  // Copy scratch back to perimeter
  for (int i = 0; i < writeIdx; i++) {
    perim.vertices[i] = scratch[i];
  }
  perim.vertexCount = writeIdx;
  perim.removeCollinear();
  p.trailCount = 0;
}

bool isQixInsidePerimeter() {
  return isInsidePolygon(q.position, perim.vertices, perim.vertexCount);
}
