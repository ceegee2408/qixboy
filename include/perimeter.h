#ifndef PERIMETER_H
#define PERIMETER_H

#include "header.h"

class index
{
public:
    byte i1;
    byte i2;
    index() : i1(0), i2(0) { debug.log("RUN", "index default constructor", INFO); }
    index(byte i1, byte i2) : i1(i1), i2(i2) {}
};

class Perimeter
{
public:
    vertex vertices[MAX_VERTICES];
    byte numPerimVertices;
    byte totalVertices;
    index trailIndex; // i1: index where player began draw, i2: the num of vertices when draw began

    void drawPerim()
    {
        debug.log("RUN", "Perimeter draw", INFO);
        for (byte i = 0; i < numPerimVertices; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1) % numPerimVertices];
            arduboy.drawLine(v1.x, v1.y, v2.x, v2.y, WHITE);
        }
    }

    void restartPerimeter()
    {
        debug.log("RUN", "restartPerimeter", INFO);
        for (int i = 0; i < MAX_VERTICES; i++)
        {
            vertices[i].x = 0;
            vertices[i].y = 0;
        }
        vertices[0] = vertex(0, 0);
        vertices[1] = vertex(0, HEIGHT - 1);
        vertices[2] = vertex(WIDTH - 1, HEIGHT - 1);
        vertices[3] = vertex(WIDTH - 1, 0);
        numPerimVertices = 4;
        totalVertices = 4;
    }

    bool isVertexOnPerim(vertex v)
    {
        for (byte i = 0; i < numPerimVertices; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1) % numPerimVertices];
            if (along(v, v1, v2))
                return true;
        }
        return false;
    }

    bool isVertexOnTrail(vertex v)
    {
        for (byte i = trailIndex.i2; i < totalVertices - 1; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1)];
            if (along(v, v1, v2))
                return true;
        }
        return false;
    }

    byte trailLength()
    {
        return totalVertices - trailIndex.i2;
    }

    vertex trail(byte index)
    {
        return vertices[trailIndex.i2 + index];
    }

    void drawTrail()
    {
        for (byte i = trailIndex.i2; i < totalVertices - 1; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1)];
            arduboy.drawLine(v1.x, v1.y, v2.x, v2.y, WHITE);
        }
    }

    void addTrail()
    {
    }

    void finishTrail()
    {
        
    }
};
Perimeter perimeter;

#endif