#ifndef PLAYER_LOGIC_H
#define PLAYER_LOGIC_H

#include <Arduino.h>
#include <Arduboy2.h>
#include "entities.h"
#include "config.h"

// External references to global objects
extern Arduboy2 arduboy;
extern player p;
extern perimeter perim;
extern qix q;
extern uint16_t frameCounter;

// Input and movement functions
byte getInput();
void updateActiveDirection(byte input);
void updatePlayerPosition(byte input);
bool movePlayer(byte allowedMoves);
void perimeterMove();
void drawMove(bool isFastMove);

// State management functions
void updatePerimIndex();
void updateCanMove();
void updateCanDraw();
void updateDrawAllowance(byte input);
void updatePerim();
void reverseVertices(vertex* verts, int count);
bool isQixInsidePerimeter();

#endif // PLAYER_LOGIC_H
