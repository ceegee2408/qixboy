#ifndef PERIMETER_H
#define PERIMETER_H

#include "header.h"

class Perimeter
{
public:
    vertex vertices[MAX_VERTICES];
    byte numVertices;

    byte nextIndex(byte i)
    {
        byte next = i + 1;
        return (next >= numVertices) ? 0 : next;
    }
    byte prevIndex(byte i)
    {
        return (i == 0) ? numVertices - 1 : i - 1;
    }

    void draw()
    {
        debug.log("RUN", "Perimeter draw", DETAILED);
        for (byte i = 0; i < numVertices; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1) % numVertices];
            arduboy.drawLine(v1.x, v1.y, v2.x, v2.y, WHITE);
        }
    }

    void addVertex(vertex v, byte index)
    {
        debug.log("RUN", "Perimeter addVertex", INFO);
        for (byte i = numVertices; i > index; i--)
        {
            vertices[i] = vertices[i - 1];
        }
        vertices[index] = v;
        numVertices++;
    }

    void removeVertex(byte index)
    {
        debug.log("RUN", "Perimeter removeVertex", INFO);
        for (byte i = index; i < numVertices - 1; i++)
        {
            vertices[i] = vertices[i + 1];
        }
        numVertices--;
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
        numVertices = 4;
    }

    bool isVertexOnPerim(vertex v)
    {
        for (byte i = 0; i < numVertices; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1) % numVertices];
            if (along(v, v1, v2))
                return true;
        }
        return false;
    }

    byte perimLengthUpTo(vertex v)
    {
        byte length = 0;
        for (byte i = 0; i < numVertices; i++)
        {
            vertex v1 = vertices[i];
            vertex v2 = vertices[(i + 1) % numVertices];
            if (along(v, v1, v2))
            {
                length += manhattanDistance(v1, v);
                return length;
            }
            else
            {
                length += manhattanDistance(v1, v2);
            }
        }
        debug.critical("in: perimLengthUpTo, vertex not found on perimeter");
        return 255;
    }

    byte getAllowedMoves(vertex position, bool drawInput = 0)
    {
        byte allowed = 0;
        for (byte i = 0; i < numVertices; i++)
        {
            vertex a = vertices[i];
            vertex b = vertices[nextIndex(i)];
            if (along(position, a, b))
            {
                allowed |= Direction::fromVertices(position, a);
                allowed |= Direction::fromVertices(position, b);
                if (drawInput)
                {
                    byte edgeDir = Direction::fromVertices(a, b);
                    allowed |= Direction::leftHandNormal(edgeDir);
                }
            }
        }
        return allowed;
    }

    void finishTrail(Trail &t) {
        //TODO
        t.clear();
    }
};

#endif