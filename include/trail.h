#ifndef TRAIL_H
#define TRAIL_H

#include "header.h"

class Trail
{
public:
    byte trailVectors[MAX_TRAIL_VECTORS];
    vector initialVector;
    byte numVectors;
    byte speed;
    bool fillAnimation;
    Trail() : numVectors(0), speed(0), fillAnimation(false) {}

    vector getVector(byte index) const
    {
        return PackedSegment::decode(trailVectors[index]);
    }

    void setVector(byte index, const vector &v)
    {
        trailVectors[index] = PackedSegment::encode(v);
    }

    bool appendPacked(const vector &v)
    {
        if (numVectors >= MAX_TRAIL_VECTORS)
        {
            debug.critical("Max trail vectors reached");
            return false;
        }
        setVector(numVectors, v);
        numVectors++;
        return true;
    }

    void appendSplit(const vector &v)
    {
        byte len = v.len();
        if (len == 0)
        {
            return;
        }

        byte dir = v.dir();
        while (len > 0)
        {
            byte chunk = (len > PackedSegment::MAX_LEN) ? PackedSegment::MAX_LEN : len;
            if (!appendPacked(Direction::unitVec(dir, chunk)))
            {
                return;
            }
            len -= chunk;
        }
    }

    void draw() {
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + getVector(i);
            arduboy.drawLine(pos.x, pos.y, next.x, next.y, WHITE);
            pos = next;
        }
    }
    void addVector(vector v)
    {
        appendSplit(v);
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

    void setFillPolygon(vector start, const vector *newVectors, byte newNumVectors)
    {
        if (newNumVectors > MAX_TRAIL_VECTORS)
        {
            debug.critical("Fill polygon exceeds trail storage");
        }
        clear();
        initialVector = start;
        speed = 0;
        fillAnimation = true;
        for (byte i = 0; i < newNumVectors; i++)
        {
            addVector(newVectors[i]);
        }
        fillAnimation = true;
    }

    void fill()
    {
        if (!fillAnimation)
        {
            return;
        }
    }

    byte getAllowedMoves(vector position, byte drawInput)
    {   
        byte allowed = 0;
        byte inputSpeed = (drawInput & Direction::FAST) ? 2 : (drawInput & Direction::SLOW) ? 1 : 0; 
        if(speed != inputSpeed) {
            return allowed;
        }
        if(numVectors == MAX_TRAIL_VECTORS){
            return getVector(numVectors - 1).dir();
        }
        // Trail can be entered before the first segment is recorded.
        if (numVectors == 0) {
            return Direction::UP | Direction::DOWN | Direction::LEFT | Direction::RIGHT;
        }
        allowed = Direction::UP | Direction::DOWN | Direction::LEFT | Direction::RIGHT;
        allowed &= ~getVector(numVectors - 1).oppDir();
        for (byte i = 0; i < 4; i++) {
            byte dir = 1 << i;
            vector nextPos = position + Direction::unitVec(dir, 2);
            if (isVectorOnTrail(nextPos)) {
                allowed &= ~dir;
            }
        }
        // prevent U-turns if trail is 2 units or shorter
        if (numVectors > 1 && getVector(numVectors - 1).len() < 3) {
            allowed &= ~getVector(numVectors - 2).oppDir();
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

        vector tail = getVector(numVectors - 1);
        if (delta.dir() != tail.dir()) {
            addVector(delta);
        } else {
            byte remaining = delta.len();
            byte dir = tail.dir();
            while (remaining > 0)
            {
                tail = getVector(numVectors - 1);
                byte space = PackedSegment::MAX_LEN - tail.len();
                if (space == 0)
                {
                    addVector(Direction::unitVec(dir, remaining));
                    return;
                }
                byte addLen = (remaining > space) ? space : remaining;
                setVector(numVectors - 1, Direction::unitVec(dir, tail.len() + addLen));
                remaining -= addLen;
            }
        }
    }

    bool isVectorOnTrail(vector v) {
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector segment = getVector(i);
            vector next = pos + segment;
            if (along(v, pos, segment)) {
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
            vector next = pos + getVector(i);
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

    void reverseSequence() {
        for (byte i = 0; i < numVectors / 2; i++) {
            vector temp = getVector(i);
            setVector(i, getVector(numVectors - 1 - i).opp());
            setVector(numVectors - 1 - i, temp.opp());
        }
        if (numVectors % 2 == 1) {
            setVector(numVectors / 2, getVector(numVectors / 2).opp());
        }
    }

};


#endif