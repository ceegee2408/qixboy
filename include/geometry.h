#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <Arduino.h>
#include "types.h"
#include "entities.h"

// External references to global objects
extern perimeter perim;
extern player p;

// Geometry utility functions
int crossProduct(vertex a, vertex b, vertex c);
void findIntersections(int y, int arcStart, int arcEnd, int arcDir, int* xs, int& xcount);
bool isInsidePolygon(vertex point, vertex* verts, int count);
bool pointOnSegment(vertex point, vertex v1, vertex v2);
bool compareVertices(vertex v1, vertex v2);
int vertexDistance(vertex v1, vertex v2);
byte getDirection(vertex from, vertex to);

#endif // GEOMETRY_H
