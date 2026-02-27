#include "geometry.h"
#include "entities.h"

int cross(vertex a, vertex b) {
  return a.getx() * b.gety() - a.gety() * b.getx();
}

int orientation(vertex a, vertex b, vertex c) {
  int val = cross(vertex(b.getx() - a.getx(), b.gety() - a.gety()), vertex(c.getx() - a.getx(), c.gety() - a.gety()));
  if (val == 0) return 0; // collinear
  return (val > 0) ? 1 : -1; // clock or counterclock wise
}

void findIntersections(int y, int arcStart, int arcEnd, int arcDir, int* xs, int& xcount) {
    int i = arcStart;
    while (i != arcEnd) {
        int next = (i + arcDir + perim.vertexCount) % perim.vertexCount;
        vertex v1 = perim.vertices[i];
        vertex v2 = perim.vertices[next];
        int minY = min(v1.gety(), v2.gety());
        int maxY = max(v1.gety(), v2.gety());
        if (y >= minY && y < maxY) {  // scanline intersects this edge
            if (v1.getx() == v2.getx()) {  // vertical edge
                xs[xcount++] = v1.getx();
            }
            // horizontal edges are excluded by the y range check above
        }
        i = next;
    }
    // walk the trail in reverse
    for (int t = p.trailCount - 1; t > 0; t--) {
        vertex v1 = p.trail[t];
        vertex v2 = p.trail[t - 1];
        int minY = min(v1.gety(), v2.gety());
        int maxY = max(v1.gety(), v2.gety());
        if (y >= minY && y < maxY) {  // scanline intersects this edge
            if (v1.getx() == v2.getx()) {  // vertical edge
                xs[xcount++] = v1.getx();
            }
        }
    }
}

bool isInsidePolygon(vertex point, vertex* verts, int count) {
  // Check if the point is inside the polygon using the winding number method
  int windingNumber = 0;
  for (int i = 0; i < count; i++) {
    vertex v1 = verts[i];
    vertex v2 = verts[(i + 1) % count];
    if (v1.gety() <= point.gety()) {
      if (v2.gety() > point.gety() && orientation(v1, v2, point) > 0) {
        windingNumber++;
      }
    } else {
      if (v2.gety() <= point.gety() && orientation(v1, v2, point) < 0) {
        windingNumber--;
      }
    }
  }
  return windingNumber != 0;
}

bool pointOnSegment(vertex point, vertex v1, vertex v2) {
  // Check if point is on the axis-aligned line segment from v1 to v2
  int minX = min(v1.getx(), v2.getx());
  int maxX = max(v1.getx(), v2.getx());
  int minY = min(v1.gety(), v2.gety());
  int maxY = max(v1.gety(), v2.gety());
  if (v1.getx() == v2.getx()) {
    return point.getx() == v1.getx() && point.gety() >= minY && point.gety() <= maxY;
  } else if (v1.gety() == v2.gety()) {
    return point.gety() == v1.gety() && point.getx() >= minX && point.getx() <= maxX;
  }
  return false;
}

bool doLinesIntersect(vertex p1, vertex p2, vertex q1, vertex q2) { 
  int o1 = orientation(q1, q2, p1), o2 = orientation(q1, q2, p2),
      o3 = orientation(p1, p2, q1), o4 = orientation(p1, p2, q2);
  return (o1 * o2 < 0) && (o3 * o4 < 0);
}

bool compareVertices(vertex v1, vertex v2) {
  if (v1.getx() == v2.getx() && v1.gety() == v2.gety()) {
    return true;
  }
  return false;
}

int vertexDistance(vertex v1, vertex v2) {
  return abs(v2.getx() - v1.getx()) + abs(v2.gety() - v1.gety());
}

byte getDirection(vertex from, vertex to) {
  byte dir = 0;
  if (to.getx() < from.getx()) dir |= 0x01;      // left
  else if (to.getx() > from.getx()) dir |= 0x02; // right
  if (to.gety() < from.gety()) dir |= 0x04;      // up
  else if (to.gety() > from.gety()) dir |= 0x08; // down
  return dir;
}
