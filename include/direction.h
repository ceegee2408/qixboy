#ifndef DIRECTION_H
#define DIRECTION_H

class vector;

namespace Direction {
    constexpr byte UP    = 0x01;
    constexpr byte RIGHT = 0x02;
    constexpr byte LEFT  = 0x04;
    constexpr byte DOWN  = 0x08;
    constexpr byte FAST  = 0x10;
    constexpr byte SLOW  = 0x20;
    constexpr byte DRAWBUTTONS = (FAST | SLOW);
    
    byte reverse(byte dir);
    byte fromPosition(vector from, vector to);
    byte leftHandNormal(vector v);
    byte leftHandNormal(byte dir);
    vector unitVec(byte dir, byte mult);
    void movePosition(vector &v, byte direction);
}

byte Direction::reverse(byte dir) {
    byte reversed = 0;
    if (dir & UP) reversed |= DOWN;
    if (dir & RIGHT) reversed |= LEFT;
    if (dir & LEFT) reversed |= RIGHT;
    if (dir & DOWN) reversed |= UP;
    return reversed;
}

#endif