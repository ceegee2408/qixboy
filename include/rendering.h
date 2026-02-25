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

// Game state tracker (declare, define in one .cpp only)
enum GAMESTATE {
    START_SCREEN,
    PLAYING,
    FILL_ANIMATION,
    DEATH_ANIMATION,
    GAME_OVER
};

extern GAMESTATE gameState;

void saveBackground(vertex pos);
void restoreBackground();
void initializeFill();

void drawLine(vertex v1, vertex v2);
void drawDotLine(vertex v1, vertex v2);
void drawPerimeter();
void drawPlayer();
void drawQix();
void drawTrail();
void drawDebug();
void drawFill();
void scanlineFill(vertex* verts, int count, bool fast);

#endif // RENDERING_H
