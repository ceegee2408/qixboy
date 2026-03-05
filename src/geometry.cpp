#include "geometry.h"
#include "entities.h"

int cross(vertex a, vertex b)
{
  return a.x * b.y - a.y * b.x;
}

int orientation(vertex a, vertex b, vertex c)
{
  int val = cross(vertex(b.x - a.x, b.y - a.y), vertex(c.x - a.x, c.y - a.y));
  if (val == 0)
    return 0;                // collinear
  return (val > 0) ? 1 : -1; // clock or counterclock wise
}

void findIntersections(int y, int arcStart, int arcEnd, int arcDir, int *xs, int &xcount)
{
  int i = arcStart;
  while (i != arcEnd)
  {
    int next = (i + arcDir + perim.vertexCount) % perim.vertexCount;
    vertex v1 = perim.vertices[i];
    vertex v2 = perim.vertices[next];
    int minY = min(v1.y, v2.y);
    int maxY = max(v1.y, v2.y);
    if (y >= minY && y < maxY)
    { // scanline intersects this edge
      if (v1.x == v2.x)
      { // vertical edge
        xs[xcount++] = v1.x;
      }
      // horizontal edges are excluded by the y range check above
    }
    i = next;
  }
  // walk the trail in reverse
  for (int t = p.trailCount - 1; t > 0; t--)
  {
    vertex v1 = p.trail[t];
    vertex v2 = p.trail[t - 1];
    int minY = min(v1.y, v2.y);
    int maxY = max(v1.y, v2.y);
    if (y >= minY && y < maxY)
    { // scanline intersects this edge
      if (v1.x == v2.x)
      { // vertical edge
        xs[xcount++] = v1.x;
      }
    }
  }
}

bool isInsidePolygon(vertex point, vertex *verts, int count)
{
  // Check if the point is inside the polygon using the winding number method
  int windingNumber = 0;
  for (int i = 0; i < count; i++)
  {
    vertex v1 = verts[i];
    vertex v2 = verts[(i + 1) % count];
    if (v1.y <= point.y)
    {
      if (v2.y > point.y && orientation(v1, v2, point) > 0)
      {
        windingNumber++;
      }
    }
    else
    {
      if (v2.y <= point.y && orientation(v1, v2, point) < 0)
      {
        windingNumber--;
      }
    }
  }
  return windingNumber != 0;
}

bool pointOnSegment(vertex point, vertex v1, vertex v2)
{
  // Check if point is on the axis-aligned line segment from v1 to v2
  int minX = min(v1.x, v2.x);
  int maxX = max(v1.x, v2.x);
  int minY = min(v1.y, v2.y);
  int maxY = max(v1.y, v2.y);
  if (v1.x == v2.x)
  {
    return point.x == v1.x && point.y >= minY && point.y <= maxY;
  }
  else if (v1.y == v2.y)
  {
    return point.y == v1.y && point.x >= minX && point.x <= maxX;
  }
  return false;
}

bool doLinesIntersect(vertex p1, vertex p2, vertex q1, vertex q2)
{
  int o1 = orientation(q1, q2, p1), o2 = orientation(q1, q2, p2),
      o3 = orientation(p1, p2, q1), o4 = orientation(p1, p2, q2);
  return (o1 * o2 < 0) && (o3 * o4 < 0);
}

bool compareVertices(vertex v1, vertex v2)
{
  if (v1.x == v2.x && v1.y == v2.y)
  {
    return true;
  }
  return false;
}

int vertexDistance(vertex v1, vertex v2)
{
  return abs(v2.x - v1.x) + abs(v2.y - v1.y);
}

byte getDirection(vertex from, vertex to)
{
  byte dir = 0;
  if (to.x < from.x)
    dir |= 0x01; // left
  else if (to.x > from.x)
    dir |= 0x02; // right
  if (to.y < from.y)
    dir |= 0x04; // up
  else if (to.y > from.y)
    dir |= 0x08; // down
  return dir;
}
