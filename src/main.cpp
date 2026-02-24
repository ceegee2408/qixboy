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
#include "qix_logic.h"

// Global game objects
player p;
perimeter perim;
qix q;
sparx s[4];
Arduboy2 arduboy;
uint16_t frameCounter = 0;

void updateScreen();

void setup() {
  arduboy.boot();
  arduboy.setFrameRate(60);
  arduboy.clear();
  updateCanMove();
}

void loop() {
  if (!arduboy.nextFrame()) {
    return;
  }
  frameCounter++;
  if (frameCounter >= 240) {
    frameCounter = 0;
  }
  arduboy.clear();
  drawFill();
  drawPerimeter();
  drawPlayer();
  drawQix();
  drawDebug();
  updateScreen();
  arduboy.display();
}

void updateScreen() {
  byte input = getInput();
  updateActiveDirection(input);
  updatePlayer(input);
  updateQix();
  // TODO: update sparx
  // TODO: update fill
  // TODO: check for death
}

