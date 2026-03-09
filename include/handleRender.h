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
        int sx = (int8_t)position.x;
        int sy = (int8_t)position.y;
        if (sy >= HEIGHT || sx < -7 || sy < -7)
            return;
        byte *buf = arduboy.getBuffer();
        byte iStart = (sy < 0) ? (byte)(-sy) : 0;
        byte jStart = (sx < 0) ? (byte)(-sx) : 0;
        for (byte i = iStart; i < 8; i++)
        {
            int screenY = sy + i;
            if (screenY >= HEIGHT) break;
            byte page = (byte)screenY / 8;
            byte bit = 1 << ((byte)screenY % 8);
            for (byte j = jStart; j < 8; j++)
            {
                int screenX = sx + j;
                if (screenX >= WIDTH) break;
                if (mask[i] & (0x80 >> j))
                {
                    if (invert)
                        buf[page * WIDTH + screenX] ^= bit;
                    else
                        buf[page * WIDTH + screenX] |= bit;
                }
            }
        }
    }

    void setBackground(vertex position)
    {
        bgPosition = position;
        int sx = (int8_t)position.x;
        int sy = (int8_t)position.y;
        if (sy >= HEIGHT || sx < -7 || sy < -7)
            return;
        byte *buf = arduboy.getBuffer();
        byte topRow = (sy < 0) ? 0 : (byte)sy;
        byte botRow = (byte)((sy + 7 < HEIGHT) ? sy + 7 : HEIGHT - 1);
        byte page = topRow / 8;
        byte lastPage = botRow / 8;
        bool hasLowerPage = (lastPage > page);
        for (byte i = 0; i < 8; i++)
        {
            int screenX = sx + i;
            if (screenX < 0) continue;
            if (screenX >= WIDTH) break;
            background[i] = buf[page * WIDTH + screenX];
            if (hasLowerPage)
                backgroundLower[i] = buf[lastPage * WIDTH + screenX];
        }
        bgSet = true;
    }

    void restoreBackground()
    {
        if (!bgSet) return;
        int sx = (int8_t)bgPosition.x;
        int sy = (int8_t)bgPosition.y;
        if (sy >= HEIGHT || sx < -7 || sy < -7)
            return;
        byte topRow = (sy < 0) ? 0 : (byte)sy;
        byte botRow = (byte)((sy + 7 < HEIGHT) ? sy + 7 : HEIGHT - 1);
        byte page = topRow / 8;
        byte lastPage = botRow / 8;
        bool hasLowerPage = (lastPage > page);
        byte *buf = arduboy.getBuffer();
        for (byte i = 0; i < 8; i++)
        {
            int screenX = sx + i;
            if (screenX < 0) continue;
            if (screenX >= WIDTH) break;
            buf[page * WIDTH + screenX] = background[i];
            if (hasLowerPage)
                buf[lastPage * WIDTH + screenX] = backgroundLower[i];
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