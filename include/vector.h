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
    byte len() const {
        return abs(x) + abs(y);
    }
    byte dir() const {
        byte dir = 0;
        if (y < 0) dir |= Direction::UP;
        if (x > 0) dir |= Direction::RIGHT;
        if (y > 0) dir |= Direction::DOWN;
        if (x < 0) dir |= Direction::LEFT;
        return dir;
    }
    byte oppDir() const {
        return Direction::reverse(dir());
    }
    vector opp() const {
        return vector(-x, -y);
    }
};

class PackedSegment
{
public:
    static constexpr byte MAX_LEN = 63;

    static byte encode(const vector &v)
    {
        byte len = v.len();
        if (len > MAX_LEN)
        {
            len = MAX_LEN;
        }

        byte code = 0;
        if (v.y < 0)
            code = 0; // UP
        else if (v.x > 0)
            code = 1; // RIGHT
        else if (v.y > 0)
            code = 2; // DOWN
        else if (v.x < 0)
            code = 3; // LEFT

        return byte((len << 2) | code);
    }

    static vector decode(byte packed)
    {
        byte len = packed >> 2;
        byte code = packed & 0x03;
        switch (code)
        {
        case 0:
            return vector(0, -len);
        case 1:
            return vector(len, 0);
        case 2:
            return vector(0, len);
        default:
            return vector(-len, 0);
        }
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

int8_t cross(vector v1, vector v2) {
    int crossProduct = v1.x * v2.y - v1.y * v2.x;
    if (crossProduct > 0) return 1; // counterclockwise
    if (crossProduct < 0) return -1; // clockwise
    return 0; // collinear
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

bool isInsidePolygon(const vector &point, const vector &polygonStart, const vector *polygon, byte numVectors)
{
    byte crossings = 0;
    vector current = polygonStart;
    for (byte i = 0; i < numVectors; i++)
    {
        vector next = current + polygon[i];
        // Half-open edge test avoids double counting when the ray hits vertices.
        bool edgeCrossesScanline = (current.y > point.y) != (next.y > point.y);
        if (edgeCrossesScanline)
        {
            int dx = (int)next.x - (int)current.x;
            int dy = (int)next.y - (int)current.y;
            int relY = (int)point.y - (int)current.y;
            int xCross = (int)current.x + (relY * dx) / dy;
            if ((int)point.x < xCross)
            {
                crossings++;
            }
        }
        current = next;
    }
    return (crossings % 2) == 1;
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