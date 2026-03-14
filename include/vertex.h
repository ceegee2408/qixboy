#ifndef VERTEX_H
#define VERTEX_H

#include "debug.h"

class vertex
{
public:
    byte x;
    byte y;
    vertex() : x(0), y(0) {}
    vertex(byte x, byte y) : x(x), y(y) {}
};

class vector
{
public:
    int x;
    int y;
    vector() : x(0), y(0) {}
    vector(int x, int y) : x(x), y(y) {}
};

vector vertexToVector(const vertex &from, const vertex &to)
{
    return vector(to.x - from.x, to.y - from.y);
}

vertex vectorToVertex(const vertex &start, const vector &vec)
{
    return vertex(start.x + vec.x, start.y + vec.y);
}

bool operator==(const vertex &v1, const vertex &v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

bool operator!=(const vertex &v1, const vertex &v2)
{
    return !(v1 == v2);
}

vertex operator+(const vertex &v1, const vertex &v2)
{
    return vertex(v1.x + v2.x, v1.y + v2.y);
}

vertex operator-(const vertex &v1, const vertex &v2)
{
    return vertex(v1.x - v2.x, v1.y - v2.y);
}

bool winding(const vertex &v1, const vertex &v2, const vertex &v3)
{
    return (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x) > 0;
}

bool isUShape(const vertex &v0, const vertex &v1, const vertex &v2, const vertex &v3) {
    int dx = v2.x - v1.x;
    int dy = v2.y - v1.y;
    if (abs(dx - dy) > 2) return false;
    bool w1 = winding(v0, v1, v2);
    bool w2 = winding(v1, v2, v3);
    bool w3 = winding(v0, v1, v3);
    return (w1 == w2) && (w1 != w3);
}

bool intersecting(const vertex &v1, const vertex &v2, const vertex &v3, const vertex &v4)
{
    int d1x = v2.x - v1.x, d1y = v2.y - v1.y;
    int d2x = v4.x - v3.x, d2y = v4.y - v3.y;

    int det = d1x * d2y - d1y * d2x;
    if (det == 0)
        return false;

    int t_num = (v3.x - v1.x) * d2y - (v3.y - v1.y) * d2x;
    int u_num = (v3.x - v1.x) * d1y - (v3.y - v1.y) * d1x;

    if (det > 0)
    {
        return (t_num >= 0 && t_num <= det) &&
               (u_num >= 0 && u_num <= det);
    }
    else
    {
        return (t_num <= 0 && t_num >= det) &&
               (u_num <= 0 && u_num >= det);
    }
}

bool along(const vertex &v, const vertex &start, const vertex &end)
{
    if (start.x == end.x) // vertical
        return v.x == start.x && v.y >= min(start.y, end.y) && v.y <= max(start.y, end.y);
    if (start.y == end.y) // horizontal
        return v.y == start.y && v.x >= min(start.x, end.x) && v.x <= max(start.x, end.x);
    return false;
}

bool isInsidePolygon(const vertex &point, const vertex *polygon, byte numVertices)
{
    if (numVertices < 3) return false;
    
    int intersections = 0;
    for (byte i = 0; i < numVertices; i++)
    {
        vertex v1 = polygon[i];
        vertex v2 = polygon[(i + 1) % numVertices];

        if ((v1.y <= point.y && v2.y > point.y) || (v2.y <= point.y && v1.y > point.y))
        {
            int xIntersection = v1.x + (point.y - v1.y) * (v2.x - v1.x) / (v2.y - v1.y);
            if (point.x < xIntersection)
            {
                intersections++;
            }
        }
    }
    return (intersections % 2) == 1;
}

int manhattanDistance(const vertex &v1, const vertex &v2)
{
    return abs(v1.x - v2.x) + abs(v1.y - v2.y);
}

void Debug::printVertex(const vertex &v)
{
    Serial.print("Vertex(");
    Serial.print(v.x);
    Serial.print(", ");
    Serial.print(v.y);
    Serial.println(")");
}

#endif