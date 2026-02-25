// Qixboy - A Qix clone for Arduboy
// File structure:
// - config.h: Game constants and defines
// - types.h: Basic types (vertex)
// - entities.h: Game entities (player, sparx, perimeter, qix)
// - geometry.h/cpp: Geometry utilities
// - rendering.h/cpp: Drawing functions
// - player_logic.h/cpp: Player movement and logic
// - main.cpp: Setup and main loop

#include <Arduino.h>
#include <Arduboy2.h>
#include "config.h"
#include "types.h"
#include "entities.h"
#include "geometry.h"
#include "rendering.h"
#include "player_logic.h"

// Global game objects
player p;
perimeter perim;
qix q;
sparx s[4];
Arduboy2 arduboy;
uint16_t frameCounter = 0;
vertex currentFillVerts[MAX_VERTICES];
int currentFillCount = 0;
int fillAnimationFrame = 0;
GAMESTATE gameState = PLAYING; // change to START_SCREEN once implimented

void setup() {
    arduboy.boot();
    arduboy.setFrameRate(60);
    arduboy.clear();
    drawPerimeter();
    updateCanMove();
    saveBackground(p.position);
    drawPlayer();
    arduboy.display();
}

void loop() {
    if (!arduboy.nextFrame()) return;
    frameCounter++;
    if (frameCounter >= 255) frameCounter = 0;

    if (gameState == PLAYING) {
      restoreBackground();
      byte input = getInput();
      updateActiveDirection(input);
      updatePlayer(input);
      saveBackground(p.position);
      drawPlayer();
      drawDebug();
    } else if(gameState == FILL_ANIMATION) {
      scanlineFill(currentFillVerts, currentFillCount, (p.allowedMoves & 0x10));
    }

    arduboy.display();
}

