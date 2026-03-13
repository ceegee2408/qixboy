#ifndef HANDLE_RENDER_H
#define HANDLE_RENDER_H

#include <Arduboy2.h>
#include "vector.h"

#define MAX_SPRITES 6

extern Arduboy2 arduboy;

class handleRender
{
public:
    const byte *mask;
    byte background[8];
    vector bgPosition;
    bool bgSet = false;
    bool invert = false;

    handleRender(const byte *newMask, bool invertMask = false)
    {
        setMask(newMask);
        invert = invertMask;
    }

    void setMask(const byte *newMask)
    {
        mask = newMask;
    }

    void drawMask(vector position)
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
                if (pgm_read_byte(&mask[i]) & (0x80 >> j))
                {
                    if (invert)
                        buf[page * WIDTH + screenX] ^= bit;
                    else
                        buf[page * WIDTH + screenX] |= bit;
                }
            }
        }
    }

    void setBackground(vector position) {
        bgPosition = position;
        int sx = (int8_t)position.x;
        int sy = (int8_t)position.y;
        if (sy >= HEIGHT || sy < -7 || sx >= WIDTH || sx < -7) return;
        byte *buf = arduboy.getBuffer();
        byte offset = (byte)((sy % 8 + 8) % 8);
        int page = sy >> 3;  // arithmetic right shift = floor division (fixes negative sy)
        for (byte i = 0; i < 8; i++) {
            int screenX = sx + i;
            if (screenX < 0 || screenX >= WIDTH) continue;
            byte top = (page >= 0) ? buf[page * WIDTH + screenX] : 0;
            byte bot = (page + 1 >= 0 && page + 1 < HEIGHT / 8)
                    ? buf[(page + 1) * WIDTH + screenX] : 0;
            background[i] = (offset == 0) ? top : (top >> offset) | (bot << (8 - offset));
        }
        bgSet = true;
    }

    void restoreBackground() {
        if (!bgSet) return;
        int sx = (int8_t)bgPosition.x;
        int sy = (int8_t)bgPosition.y;
        if (sy >= HEIGHT || sy < -7 || sx >= WIDTH || sx < -7) return;
        byte *buf = arduboy.getBuffer();
        byte offset = (byte)((sy % 8 + 8) % 8);
        int page = sy >> 3;  // arithmetic right shift = floor division (fixes negative sy)
        byte mask_top = (offset == 0) ? 0xFF : (byte)(0xFF << offset);
        byte mask_bot = (offset == 0) ? 0x00 : (byte)(0xFF >> (8 - offset));
        for (byte i = 0; i < 8; i++) {
            int screenX = sx + i;
            if (screenX < 0 || screenX >= WIDTH) continue;
            if (page >= 0)
                buf[page * WIDTH + screenX] =
                    (buf[page * WIDTH + screenX] & ~mask_top) |
                    (offset == 0 ? background[i] : (background[i] << offset));
            if (page + 1 >= 0 && page + 1 < HEIGHT / 8 && mask_bot != 0)
                buf[(page + 1) * WIDTH + screenX] =
                    (buf[(page + 1) * WIDTH + screenX] & ~mask_bot) |
                    (background[i] >> (8 - offset));
        }
    }
};

class SpriteManager {
    public:
        handleRender *sprites[MAX_SPRITES];
        byte numSprites = 0;
        vector positions[MAX_SPRITES];

        void add(handleRender *sprite, vector position) {
            if (numSprites < MAX_SPRITES) {
                sprite->bgSet = false;
                sprites[numSprites] = sprite;
                positions[numSprites] = position;
                numSprites++;
            } else {
                debug.critical("Max sprites reached");
            }
        }

        void update(handleRender *sprite, vector newPosition) {
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