#ifndef DIRECTION_H
#define DIRECTION_H

#include "header.h"

enum DirectionEnum
{
    UP = 1,
    RIGHT = 2,
    DOWN = 4,
    LEFT = 8
};

class direction
{
    public:
        byte dir; // bitfield for moves, bit 0 = up, bit 1 = right, bit 2 = down, bit 3 = left
        direction() : dir(0) {}
        direction(byte d) : dir(d) {}
        void addDirection(DirectionEnum direction) {
            dir |= direction;
        }
        void removeDirection(DirectionEnum direction) {
            dir &= ~direction;
        }
        explicit operator bool() const {
            return dir != 0;
        }
};

direction operator&(const direction &d1, const direction &d2) {
    return direction(d1.dir & d2.dir);
}

direction operator|(const direction &d1, const direction &d2) {
    return direction(d1.dir | d2.dir);
}

bool operator==(const direction &d1, const direction &d2) {
    return d1.dir == d2.dir;
}

bool operator!(const direction &d)
{
    return d.dir == 0;
}

direction operator^(const direction &d1, const direction &d2) {
    return direction(d1.dir ^ d2.dir);
}

direction directionFromVertexToVertex(vertex from, vertex to) {
    direction result;
    if (to.y < from.y) result.addDirection(UP);
    else if (to.y > from.y) result.addDirection(DOWN);
    if (to.x > from.x) result.addDirection(RIGHT);
    else if (to.x < from.x) result.addDirection(LEFT);
    return result;
}

#endif