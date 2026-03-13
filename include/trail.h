#ifndef TRAIL_H
#define TRAIL_H

#include "header.h"

class Trail
{
    public:
    vector vectors[MAX_TRAIL_VECTORS];
    vector initialVector;
    byte numVectors;
    byte speed;
    bool fillAnimation;
    Trail() : numVectors(0), speed(0), fillAnimation(false) {}
    void draw() {
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + vectors[i];
            arduboy.drawLine(pos.x, pos.y, next.x, next.y, WHITE);
            pos = next;
        }
    }
    void addVector(vector v)
    {
        if (numVectors < MAX_TRAIL_VECTORS)
        {
            vectors[numVectors] = v;
            numVectors++;
        }
        else
        {
            debug.critical("Max trail vectors reached");
        }
    }
    void clear()
    {
        numVectors = 0;
        speed = 0;
        fillAnimation = false;
        initialVector = vector(0, 0);
    }

    void beginTrail(vector start, byte trailSpeed) {
        clear();
        initialVector = start;
        speed = trailSpeed;
    }

    byte getAllowedMoves(vector position, byte drawInput)
    {   
        byte allowed = 0;
        byte inputSpeed = (drawInput & Direction::FAST) ? 2 : (drawInput & Direction::SLOW) ? 1 : 0; 
        if(speed != inputSpeed) {
            return allowed;
        }

        // Trail can be entered before the first segment is recorded.
        if (numVectors == 0) {
            return Direction::UP | Direction::DOWN | Direction::LEFT | Direction::RIGHT;
        }

        allowed = Direction::UP | Direction::DOWN | Direction::LEFT | Direction::RIGHT;
        allowed &= ~vectors[numVectors - 1].oppDir();
        for (byte i = 0; i < 4; i++) {
            byte dir = 1 << i;
            vector nextPos = position + Direction::unitVec(dir, 2);
            if (isVectorOnTrail(nextPos)) {
                allowed &= ~dir;
            }
        }
        // prevent U-turns if trail is 2 units or shorter
        if (numVectors > 1 && vectors[numVectors - 1].len() < 3) {
            allowed &= ~vectors[numVectors - 2].oppDir();
        }
        return allowed;
    }

    void recordMovement(vector from, vector to) {
        vector delta = to - from;
        if (delta == vector(0, 0)) {
            return;
        }

        if (numVectors == 0) {
            addVector(delta);
            return;
        }

        if (delta.dir() != vectors[numVectors - 1].dir()) {
            addVector(delta);
        } else {
            vectors[numVectors - 1] = vectors[numVectors - 1] + delta;
        }
    }

    bool isVectorOnTrail(vector v) {
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + vectors[i];
            if (along(v, pos, vectors[i])) {
                return true;
            }
            pos = next;
        }
        return false;
    }

    long calculateArea() {
        long area = 0;
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + vectors[i];
            area += (long)pos.x * next.y - (long)next.x * pos.y;
            pos = next;
        }
        return abs(area) / 2;
    }

    long calculateScore() {
        long area = calculateArea();
        if (speed == 2) {
            area *= 3;
        } else if (speed == 1) {
            area *= 2;
        }
        return area;
    }

    void updateVectors(vector position) {
        return;
    }

};


#endif