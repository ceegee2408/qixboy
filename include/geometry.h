#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <Arduboy2.h>
#include "entities.h"

// Forward declarations to avoid include cycle with entities.h
class perimeter;
class player;

// External references to global objects
extern perimeter perim;
extern player p;
class vertex;

// Geometry utility functions
int cross(vertex a, vertex b);
int orientation(vertex a, vertex b, vertex c);
void findIntersections(int y, int arcStart, int arcEnd, int arcDir, int* xs, int& xcount);
bool isInsidePolygon(vertex point, vertex* verts, int count);
bool pointOnSegment(vertex point, vertex v1, vertex v2);
bool compareVertices(vertex v1, vertex v2);
int vertexDistance(vertex v1, vertex v2);
byte getDirection(vertex from, vertex to);
bool doLinesIntersect(vertex p1, vertex p2, vertex p3, vertex p4);

#endif // GEOMETRY_H
