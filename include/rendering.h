#ifndef RENDERING_H
#define RENDERING_H

#include <Arduino.h>
#include <Arduboy2.h>
#include "types.h"
#include "entities.h"
#include "config.h"

// External references to global objects
extern Arduboy2 arduboy;
extern player p;
extern perimeter perim;
extern qix q;
extern int fillAnimationFrame;
extern bool fillDith;

// Game state tracker (declare, define in one .cpp only)
#include "gamestate.h"

void saveBackground(vertex pos);
void restoreBackground();
void saveFuzeBackground(vertex pos);
void restoreFuzeBackground();
void initializeFill(bool speed);

void drawLine(vertex v1, vertex v2);
void drawDotLine(vertex v1, vertex v2);
void drawPerimeter();
void drawPlayer();
void drawQix();
void eraseQixHistory();
void drawTrail();
void drawDebug();
void respawn();
extern vertex deathAnimCenter;
void initializeDeathAnimation();
void drawDeathAnimation();
void drawFill();
void scanlineFill(vertex* verts, int count, bool fast);
// Draw a frame from a PROGMEM frames array stored as [frame][row].
// `frameW` may be 5 or 7; helper chooses appropriate bit mask.
void drawSpriteFrame_P(const uint8_t *frames, int frameIdx, int frameW, int frameH, int x, int y);

#endif // RENDERING_H
