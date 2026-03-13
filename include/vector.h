#ifndef vector_H
#define vector_H

#include "debug.h"
#include "direction.h"

class vector 
{
public:
    int8_t x;
    int8_t y;
    vector() : x(0), y(0) {}
    vector(int8_t x, int8_t y) : x(x), y(y) {}
    byte len() {
        return abs(x) + abs(y);
    }
    byte dir() {
        byte dir = 0;
        if (y < 0) dir |= Direction::UP;
        if (x > 0) dir |= Direction::RIGHT;
        if (y > 0) dir |= Direction::DOWN;
        if (x < 0) dir |= Direction::LEFT;
        return dir;
    }
    byte oppDir() {
        return Direction::reverse(dir());
    }
};

bool operator==(const vector &v1, const vector &v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

bool operator!=(const vector &v1, const vector &v2)
{
    return !(v1 == v2);
}

vector operator+(const vector &v1, const vector &v2)
{
    return vector(v1.x + v2.x, v1.y + v2.y);
}

vector operator-(const vector &v1, const vector &v2)
{
    return vector(v1.x - v2.x, v1.y - v2.y);
}

vector operator+=(vector &v1, vector &v2)
{
    return v1 = v1 + v2;
}

bool winding(const vector &v1, const vector &v2, const vector &v3)
{
    return (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x) > 0;
}

bool isUShape(const vector &position, const vector &v1, const vector &v2, const vector &v3) {
    return (winding(position, v1, v2) != winding(position, v1, v3)) &&
           (winding(v1, v2, v3) != winding(v1, v2, position)) &&
           (winding(v2, v3, position) != winding(v2, v3, v1)) &&
           (winding(v3, position, v1) != winding(v3, position, v2));
}

bool intersecting(const vector &v1, const vector &v2, const vector &v3, const vector &v4)
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

bool along(const vector &v, const vector &position, const vector &direction)
{   
    vector start = position;
    vector end = position + direction;
    if (start.x == end.x) // vertical
        return v.x == start.x && v.y >= min(start.y, end.y) && v.y <= max(start.y, end.y);
    if (start.y == end.y) // horizontal
        return v.y == start.y && v.x >= min(start.x, end.x) && v.x <= max(start.x, end.x);
    return false;
}

bool isInsidePolygon(const vector &point, const vector *polygon, byte numVectors)
{
    if (numVectors < 3) return false;
    
    int intersections = 0;
    for (byte i = 0; i < numVectors; i++)
    {
        vector v1 = polygon[i];
        vector v2 = polygon[(i + 1) % numVectors];

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

int manhattanDistance(const vector &v1, const vector &v2)
{
    return abs(v1.x - v2.x) + abs(v1.y - v2.y);
}

void Debug::printVector(const vector &v)
{
    Serial.print("Vector(");
    Serial.print(v.x);
    Serial.print(", ");
    Serial.print(v.y);
    Serial.println(")");
}

byte Direction::fromPosition(vector from, vector to) {
    vector delta = to - from;
    return delta.dir();
}

byte Direction::leftHandNormal(vector v) {
    byte dir = v.dir();
    if (dir == Direction::DOWN)  return Direction::RIGHT;
    if (dir == Direction::RIGHT) return Direction::UP;
    if (dir == Direction::UP)    return Direction::LEFT;
    if (dir == Direction::LEFT)  return Direction::DOWN;
    return 0;
}

byte Direction::leftHandNormal(byte dir) {
    if (dir == Direction::DOWN)  return Direction::RIGHT;
    if (dir == Direction::RIGHT) return Direction::UP;
    if (dir == Direction::UP)    return Direction::LEFT;
    if (dir == Direction::LEFT)  return Direction::DOWN;
    return 0;
}

vector Direction::unitVec(byte dir, byte mult = 1) {
    byte x = 0, y = 0;
    if (dir & Direction::UP) y = -1;
    if (dir & Direction::RIGHT) x = 1;
    if (dir & Direction::DOWN) y = 1;
    if (dir & Direction::LEFT) x = -1;
    return vector(x * mult, y * mult);
}

void movePosition(vector &v, byte direction) {
    if (direction & Direction::UP) v.y--;
    if (direction & Direction::RIGHT) v.x++;
    if (direction & Direction::DOWN) v.y++;
    if (direction & Direction::LEFT) v.x--;
}

#endif