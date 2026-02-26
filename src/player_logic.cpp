#include "player_logic.h"
#include "geometry.h"
#include "rendering.h"

// Set to true when a draw completes while A/B is still held, so
// updateDrawAllowance blocks re-entry until the button is released.
static bool drawCooldown = false;

// External fill capture buffer (defined in main.cpp)
extern vertex currentFillVerts[];
extern int currentFillCount;

namespace {
const byte DIR_LEFT  = 0x01;
const byte DIR_RIGHT = 0x02;
const byte DIR_UP    = 0x04;
const byte DIR_DOWN  = 0x08;
const byte MODE_FAST = 0x10;
const byte MODE_SLOW = 0x20;

byte reverseDirection(byte dir) {
  if (dir == DIR_LEFT) return DIR_RIGHT;
  if (dir == DIR_RIGHT) return DIR_LEFT;
  if (dir == DIR_UP) return DIR_DOWN;
  if (dir == DIR_DOWN) return DIR_UP;
  return 0;
}

void stepFromDirection(vertex &pos, byte dir) {
  if (dir & DIR_LEFT) {
    pos.addx(-1);
  } else if (dir & DIR_RIGHT) {
    pos.addx(1);
  } else if (dir & DIR_UP) {
    pos.addy(-1);
  } else if (dir & DIR_DOWN) {
    pos.addy(1);
  }
}

bool positionOnPerimeter(vertex pos, int &edgeStart, int &edgeEnd) {
  for (int i = 0; i < perim.vertexCount; i++) {
    int next = (i + 1) % perim.vertexCount;
    if (pointOnSegment(pos, perim.vertices[i], perim.vertices[next])) {
      edgeStart = i;
      edgeEnd = next;
      if (compareVertices(pos, perim.vertices[i])) edgeEnd = i;
      else if (compareVertices(pos, perim.vertices[next])) edgeStart = next;
      return true;
    }
  }
  return false;
}

bool refreshPlayerPerimeterIndices() {
  for (int i = 0; i < perim.vertexCount; i++) {
    int next = (i + 1) % perim.vertexCount;
    vertex a = perim.vertices[i];
    vertex b = perim.vertices[next];
    if (compareVertices(p.position, a)) {
      p.setPerimIndex(0, i);
      p.setPerimIndex(1, i);
      return true;
    }
    if (compareVertices(p.position, b)) {
      p.setPerimIndex(0, next);
      p.setPerimIndex(1, next);
      return true;
    }
    if (pointOnSegment(p.position, a, b)) {
      p.setPerimIndex(0, i);
      p.setPerimIndex(1, next);
      return true;
    }
  }
  return false;
}

bool finalizeDrawPath(bool isFastMove) {
  int edgeStart;
  int edgeEnd;
  if (!positionOnPerimeter(p.position, edgeStart, edgeEnd)) {
    return false;
  }

  p.setDrawEndIndex(0, edgeStart);
  p.setDrawEndIndex(1, edgeEnd);
  p.addTrailVertex(p.position);
  updatePerim();

  gameState = FILL_ANIMATION;
  initializeFill(isFastMove);

  bool found = refreshPlayerPerimeterIndices();
  if (!found) {
    p.allowedMoves &= ~0x30;
  }
  drawCooldown = true;
  updateCanMove();
  return true;
}
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
  if (p.allowedMoves & (MODE_FAST | MODE_SLOW)) {
    drawTrail();
    if ((input & MODE_FAST) && (p.allowedMoves & MODE_FAST)) {
      drawMove(true);
    } else if ((input & MODE_SLOW) && (p.allowedMoves & MODE_SLOW)) {
      drawMove(false);
    }
  } else if (input & 0x0F) {
    perimeterMove();
  }
}

void drawMove(bool isFastMove) {
  byte interval = isFastMove ? FAST_MOVE : SLOW_MOVE;
  if (frameCounter % interval == 0) {
    if (p.getActiveDir() & p.allowedMoves) {
      if (p.getLastTrailDir() != 0 && p.getActiveDir() != p.getLastTrailDir()) {
        p.addTrailVertex(p.position);
      }
      p.setLastTrailDir(p.getActiveDir());
      movePlayer(p.allowedMoves);
    }

    if (p.trailCount > 0 &&
        (p.position.getx() != p.trail[0].getx() || p.position.gety() != p.trail[0].gety())) {
      if (finalizeDrawPath(isFastMove)) {
        return;
      }
    }
    updateCanDraw();
  }
}

void updateCanDraw() {
  byte allowedMoves = 0x0F;

  byte lastDir = p.getLastTrailDir();

  allowedMoves &= ~reverseDirection(lastDir);

  // Block U-turns: if the current segment is < 3 pixels, block the reverse
  // of the *previous* segment's direction (prevents tight parallel lines).
  if (p.trailCount >= 2) {
    int curSegLen = vertexDistance(p.position, p.trail[p.trailCount - 1]);
    if (curSegLen < 3) {
      byte prevSegDir = getDirection(p.trail[p.trailCount - 2], p.trail[p.trailCount - 1]);
      allowedMoves &= ~reverseDirection(prevSegDir);
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

  // If the trail is near capacity, prevent any turns that would add a
  // new trail vertex.  `addTrailVertex()` stores up to `MAX_VERTICES/4`
  // vertices, so when we're at capacity-1, a subsequent turn would
  // consume the final slot — block turning proactively to avoid
  // reaching the limit mid-draw.
  int trailCapacity = MAX_VERTICES / 4;
  if (p.trailCount >= trailCapacity - 1) {
    byte lastDir = p.getLastTrailDir();
    if (lastDir != 0) {
      // Only allow continuing in the same direction as the last trail
      // segment; clear other directional bits so the player cannot turn.
      allowedMoves &= lastDir;
    }
  }

  p.allowedMoves = allowedMoves | (p.allowedMoves & 0x30); // Preserve draw mode bits
}

bool movePlayer(byte allowedMoves) {
  vertex prevPos = p.position;
  byte activeDir = p.getActiveDir();
  if (activeDir & allowedMoves) {
    stepFromDirection(p.position, activeDir);
  }

  if (p.position.getx() != prevPos.getx() || p.position.gety() != prevPos.gety()) {
    p.noteMoved();
    return true;
  }
  return false;
}

void perimeterMove() {
  if (frameCounter % FAST_MOVE == 0) {
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
  
  byte activeDir = p.getActiveDir();
  if (activeDir != 0 && (activeDir & p.allowedMoves) == 0) {
    vertex nextPos = p.position;
    stepFromDirection(nextPos, activeDir);
    
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
    // New perimeter = scratch (trail + backward arc) — already built.
    // Captured area = forward arc + reversed trail. Special-case when both
    // trail endpoints lie on the same perimeter edge (startIdx == endIdx):
    // the small captured polygon is just the trail vertices (U-shape).
    int ci = 0;
    if (startIdx == endIdx) {
      for (int i = 0; i < p.trailCount && ci < MAX_VERTICES; i++)
        currentFillVerts[ci++] = p.trail[i];
      currentFillCount = ci;
    } else {
      int cidx = startIdxNext;
      if (startIdx == startIdxNext) cidx = (cidx + 1) % perim.vertexCount;
      while (cidx != endIdx && ci < MAX_VERTICES) {
        currentFillVerts[ci++] = perim.vertices[cidx];
        cidx = (cidx + 1) % perim.vertexCount;
      }
      if (endIdx != endIdxNext && ci < MAX_VERTICES)
        currentFillVerts[ci++] = perim.vertices[endIdx];
      for (int i = p.trailCount - 1; i >= 0 && ci < MAX_VERTICES; i--)
        currentFillVerts[ci++] = p.trail[i];
      currentFillCount = ci;
    }

    // scratch already holds the new perimeter
  } else {
    // Captured area = trail + backward arc — that's currently in scratch.
    // If the trail starts and ends on the same perimeter edge, the small
    // captured polygon is only the trail vertices; otherwise copy scratch.
    if (startIdx == endIdx) {
      int ci = 0;
      for (int i = 0; i < p.trailCount && ci < MAX_VERTICES; i++)
        currentFillVerts[ci++] = p.trail[i];
      currentFillCount = ci;
    } else {
      for (int i = 0; i < writeIdx; i++) currentFillVerts[i] = scratch[i];
      currentFillCount = writeIdx;
    }

    // Rebuild scratch as forward arc + reversed trail (existing logic)
    writeIdx = 0;
    idx = startIdxNext;
    if (startIdx == startIdxNext) idx = (idx + 1) % perim.vertexCount;
    while (idx != endIdx && writeIdx < MAX_VERTICES) {
      scratch[writeIdx++] = perim.vertices[idx];
      idx = (idx + 1) % perim.vertexCount;
    }
    if (endIdx != endIdxNext && writeIdx < MAX_VERTICES)
      scratch[writeIdx++] = perim.vertices[endIdx];
    for (int i = p.trailCount - 1; i >= 0 && writeIdx < MAX_VERTICES; i--)
      scratch[writeIdx++] = p.trail[i];
  }

  // Copy scratch back to perimeter
  for (int i = 0; i < writeIdx; i++) {
    perim.vertices[i] = scratch[i];
  }
  perim.vertexCount = writeIdx;
  perim.removeCollinear();
  p.trailCount = 0;
  // Draw completed: forget any saved fuze resume position and erase any
  // remaining fuze pixels so a subsequent new draw starts fresh.
  extern fuze fz;
  if (fz.hasResumePos) fz.hasResumePos = false;
  restoreFuzeBackground();
}

bool isQixInsidePerimeter() {
  return isInsidePolygon(q.position, perim.vertices, perim.vertexCount);
}
