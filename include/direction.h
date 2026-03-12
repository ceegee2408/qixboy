#ifndef DIRECTION_H
#define DIRECTION_H

#include "vertex.h"

namespace Direction {
    constexpr byte UP    = 0x01;
    constexpr byte RIGHT = 0x02;
    constexpr byte LEFT  = 0x04;
    constexpr byte DOWN  = 0x08;
    constexpr byte FAST  = 0x10;
    constexpr byte SLOW  = 0x20;
    constexpr byte DRAWBUTTONS = (FAST | SLOW);
    
    byte reverse(byte dir);
    byte fromVertices(vertex from, vertex to);
    byte leftHandNormal(byte dir);
}

byte Direction::reverse(byte dir) {
    byte reversed = 0;
    if (dir & UP) reversed |= DOWN;
    if (dir & RIGHT) reversed |= LEFT;
    if (dir & LEFT) reversed |= RIGHT;
    if (dir & DOWN) reversed |= UP;
    return reversed;
}

byte Direction::fromVertices(vertex from, vertex to) {
    byte dir = 0;
    if (to.y < from.y) dir |= UP;
    if (to.x > from.x) dir |= RIGHT;
    if (to.y > from.y) dir |= DOWN;
    if (to.x < from.x) dir |= LEFT;
    return dir;
}

byte Direction::leftHandNormal(byte dir) {
    // returns the left-hand normal (90-degree CCW rotation)
    // for a clockwise perimeter, this points inward
    if (dir == DOWN)  return RIGHT;
    if (dir == RIGHT) return UP;
    if (dir == UP)    return LEFT;
    if (dir == LEFT)  return DOWN;
    return 0;
}

void moveVertex(vertex &v, byte direction) {
    if (direction & Direction::UP) v.y--;
    if (direction & Direction::RIGHT) v.x++;
    if (direction & Direction::DOWN) v.y++;
    if (direction & Direction::LEFT) v.x--;
}

#endif