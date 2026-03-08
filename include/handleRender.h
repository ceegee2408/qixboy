#ifndef HANDLE_RENDER_H
#define HANDLE_RENDER_H

#include <Arduboy2.h>
#include "vertex.h"

#define MAX_SPRITES 6

extern Arduboy2 arduboy;

class handleRender
{
public:
    byte mask[8];
    byte background[8];
    byte backgroundLower[8];
    vertex bgPosition;
    bool bgSet = false;
    bool invert = false;

    handleRender(const byte *newMask, bool invertMask = false)
    {
        setMask(newMask);
        invert = invertMask;
    }

    void setMask(const byte *newMask)
    {
        memcpy_P(mask, newMask, 8);
    }

    void drawMask(vertex position)
    {
        if (position.x >= WIDTH || position.y >= HEIGHT)
            return;
        byte *buf = arduboy.getBuffer();
        for (byte i = 0; i < 8; i++)
        {
            if (position.y + i >= HEIGHT) break;
            byte page = (position.y + i) / 8;
            byte bit = 1 << ((position.y + i) % 8);
            for (byte j = 0; j < 8; j++)
            {
                if (position.x + j >= WIDTH) break;
                if (mask[i] & (0x80 >> j))
                {
                    if (invert)
                        buf[page * WIDTH + (position.x + j)] ^= bit;
                    else
                        buf[page * WIDTH + (position.x + j)] |= bit;
                }
            }
        }
    }

    void setBackground(vertex position)
    {
        bgPosition = position;
        byte page = position.y / 8;
        byte yOffset = position.y % 8;
        if (page >= HEIGHT / 8)
            return;
        byte *buf = arduboy.getBuffer();
        for (byte i = 0; i < 8; i++)
        {
            if (position.x + i >= WIDTH)
                break;
            background[i] = buf[page * WIDTH + (position.x + i)];
            if (yOffset > 0 && page + 1 < HEIGHT / 8)
                backgroundLower[i] = buf[(page + 1) * WIDTH + (position.x + i)];
        }
        bgSet = true;
    }

    void restoreBackground()
    {
        if (!bgSet) return;
        byte page = bgPosition.y / 8;
        byte yOffset = bgPosition.y % 8;
        if (page >= HEIGHT / 8)
            return;
        byte *buf = arduboy.getBuffer();
        for (byte i = 0; i < 8; i++)
        {
            if (bgPosition.x + i >= WIDTH)
                break;
            buf[page * WIDTH + (bgPosition.x + i)] = background[i];
            if (yOffset > 0 && page + 1 < HEIGHT / 8)
                buf[(page + 1) * WIDTH + (bgPosition.x + i)] = backgroundLower[i];
        }
    }

    void restoreSetDraw(vertex position)
    {
        if (!bgSet)
        {
            setBackground(position);
            bgSet = true;
        }
        restoreBackground();
        setBackground(position);
        drawMask(position);
    }
};

class SpriteManager {
    public:
        handleRender *sprites[MAX_SPRITES];
        byte numSprites = 0;
        vertex positions[MAX_SPRITES];

        void add(handleRender *sprite, vertex position) {
            if (numSprites < MAX_SPRITES) {
                sprite->bgSet = false;
                sprites[numSprites] = sprite;
                positions[numSprites] = position;
                numSprites++;
            } else {
                debug.critical("Max sprites reached");
            }
        }

        void update(handleRender *sprite, vertex newPosition) {
            for (byte i = 0; i < numSprites; i++) {
                if (sprites[i] == sprite) {
                    positions[i] = newPosition;
                    return;
                }
            }
            debug.raise("Sprite not found in manager");
        }

        void restore() {
            for (byte i = 0; i < numSprites; i++) {
                sprites[i]->restoreBackground();
            }
        }

        void draw() {
            for (byte i = 0; i < numSprites; i++) {
                sprites[i]->setBackground(positions[i]);
                sprites[i]->drawMask(positions[i]);
            }
        }
};

SpriteManager spriteManager;

#endif