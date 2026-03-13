#ifndef PERIMETER_H
#define PERIMETER_H

#include "header.h"

class Perimeter
{
public:
    vector vectors[MAX_VECTORS];
    vector initialVector;
    byte numVectors;
    byte nextIndex(byte i)
    {
        byte next = i + 1;
        return (next >= numVectors) ? 0 : next;
    }
    byte prevIndex(byte i)
    {
        return (i == 0) ? numVectors - 1 : i - 1;
    }

    void draw()
    {   
        debug.log("RUN", "Perimeter draw", DETAILED);
        vector pos = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector next = pos + vectors[i];
            arduboy.drawLine(pos.x, pos.y, next.x, next.y, WHITE);
            pos = next;
        }
    }

    void addVector(vector v, byte index)
    {
        debug.log("RUN", "Perimeter addvector", INFO);
        for (byte i = numVectors; i > index; i--)
        {
            vectors[i] = vectors[i - 1];
        }
        vectors[index] = v;
        numVectors++;
    }

    void removeVector(byte index)
    {
        debug.log("RUN", "Perimeter removevector", INFO);
        for (byte i = index; i < numVectors - 1; i++)
        {
            vectors[i] = vectors[i + 1];
        }
        numVectors--;
    }

    void restartPerimeter()
    {
        debug.log("RUN", "restartPerimeter", INFO);
        for (int i = 0; i < MAX_VECTORS; i++)
        {
            vectors[i].x = 0;
            vectors[i].y = 0;
        }
        initialVector = vector(0, 0);
        vectors[0] = vector(0, HEIGHT - 1);
        vectors[1] = vector(WIDTH - 1, 0);
        vectors[2] = vector(0, -HEIGHT + 1);
        vectors[3] = vector(-WIDTH + 1, 0);
        numVectors = 4;
    }

    bool isVectorOnPerim(vector v)
    {
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector b = a + vectors[i];
            if (along(v, a, vectors[i]))
            {
                return true;
            }
            a = b;
        }
        return false;
    }

    byte perimLengthUpTo(vector v)
    {
        byte length = 0;
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector b = a + vectors[i];
            if (along(v, a, vectors[i]))
            {
                length += (v - a).len();
                return length;
            }
            else
            {
                length += vectors[i].len();
            }
            a = b;
        }
        debug.critical("in: perimLengthUpTo, vector not found on perimeter");
        return 255;
    }


    byte getAllowedMoves(vector position, bool drawInput = 0)
    {
        byte allowed = 0;
        vector a = initialVector;
        for (byte i = 0; i < numVectors; i++)
        {
            vector b = a + vectors[i];
            if (along(position, a, vectors[i]))
            {
                byte segDir = vectors[i].dir();
                if (position != b) allowed |= segDir;
                if (position != a) allowed |= Direction::reverse(segDir);
                if (drawInput)
                    allowed |= Direction::leftHandNormal(vectors[i]);
            }
            a = b;
        }
        return allowed;
    }

    void insertTrail(Trail &t, byte startIndex, byte endIndex) {

    }

    void finishTrail(Trail &t, vector position, vector qixPosition) {
        //vertex SCRATCH[]
        if (t.numVectors == 0) {
            return;
        }
        vector trailStart = t.initialVector;
        vector trailEnd = position;
        int8_t indexSeq = cross(qixPosition - trailStart, qixPosition - trailEnd); // idxseq returns 1 if the trail is indexed counterclockwise, -1 if clockwise, and 0 if collinear
        // if -1 we rearrange the trail vectors in reverse order and invert them
        if (indexSeq == -1) t.reverseSequence();
        if (indexSeq == 0) {
            debug.critical("in: finishTrail, trail is collinear with qix");
        }
        // insert trail vectors into perimeter
        byte insertIndex = 0;
    }
};

#endif