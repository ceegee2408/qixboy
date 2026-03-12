#ifndef TRAIL_H
#define TRAIL_H

#include "header.h"

class Trail
{
    public:
    vertex vertices[MAX_TRAIL_VERTICES];
    byte numVertices;
    bool fillAnimation;
    Trail() : numVertices(0), fillAnimation(false) {}
    void draw() {}
    void addVertex(vertex v)
    {
        if (numVertices < MAX_TRAIL_VERTICES)
        {
            vertices[numVertices] = v;
            numVertices++;
        }
        else
        {
            debug.critical("Max trail vertices reached");
        }
    }
    void clear()
    {
        numVertices = 0;
        fillAnimation = false;
    }

    void beginTrail(vertex start) {
        clear();
        addVertex(start);
    }

    byte getAllowedMoves(vertex position, bool input)
    {

    }
    void updateVertices(vertex position) {
        return;
    }

};


#endif