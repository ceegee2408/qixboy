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

class line
{
public:
    vertex start;
    vertex end;
    line() : start(vertex()), end(vertex()) {}
    line(vertex start, vertex end) : start(start), end(end) {}
};

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

void Debug::printVertex(const vertex &v)
{
    Serial.print("Vertex(");
    Serial.print(v.x);
    Serial.print(", ");
    Serial.print(v.y);
    Serial.println(")");
}

#endif