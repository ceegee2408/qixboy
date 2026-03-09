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
    
    byte reverse(byte dir);
    byte fromVertices(vertex from, vertex to);
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

void moveVertex(vertex &v, byte direction) {
    if (direction & Direction::UP) v.y--;
    if (direction & Direction::RIGHT) v.x++;
    if (direction & Direction::DOWN) v.y++;
    if (direction & Direction::LEFT) v.x--;
}

#endif