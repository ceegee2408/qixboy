// Qixboy - A Qix clone for Arduboy
// File structure:
// - config.h: Game constants and defines
// - types.h: Basic types (vertex)
// - entities.h: Game entities (player, sparx, perimeter, qix)
// - geometry.h/cpp: Geometry utilities
// - rendering.h/cpp: Drawing functions
// - player_logic.h/cpp: Player movement and logic
// - main.cpp: Setup and main loop

#define EEPROM_HIGH_SCORE_ADDR 0 // Address in EEPROM to store high score (2 bytes)

#include <Arduino.h>
#include <Arduboy2.h>
#include "config.h"
#include "types.h"
#include "entities.h"
#include "geometry.h"
#include "rendering.h"
#include "player_logic.h"
#include "EEPROM.h"

// Global game objects
int score = 0;
int highScores[6] = {
  EEPROM.read(EEPROM_HIGH_SCORE_ADDR),
  EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 1),
  EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 2),
  EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 3),
  EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 4),
  EEPROM.read(EEPROM_HIGH_SCORE_ADDR + 5)
};
player p;
perimeter perim;
qix q;
sparx s[4];
fuze fz;
Arduboy2 arduboy;
uint16_t frameCounter = 0;
vertex currentFillVerts[MAX_VERTICES / 4];
int currentFillCount = 0;
int fillAnimationFrame = 0;
bool fillDith = false;
// How many frames idle in draw mode before fuze begins
const uint16_t FUZE_IDLE_THRESHOLD = 45;
GAMESTATE gameState = PLAYING; // change to START_SCREEN once implimented

void setup() {
    arduboy.boot();
    arduboy.setFrameRate(60);
    arduboy.clear();
    drawPerimeter();
    updateCanMove();
    saveBackground(p.position);
    drawPlayer();
    // Advance player's idle/movement frame counter every loop iteration
    p.tickFrame();
    arduboy.display();
}

void loop() {
    if (!arduboy.nextFrame()) return;
    frameCounter++;
    if (frameCounter >= 255) frameCounter = 0;

    if (gameState == PLAYING) {
      // Restore previous fuze render first, then player background.
      restoreFuzeBackground();
      restoreBackground();
      byte input = getInput();
      updateActiveDirection(input);
      updatePlayer(input);

      // Start fuze: either resume instantly if we have a saved resume position
      // and the player just stopped (framesSinceMove == 1), or after the
      // configured idle threshold.
      if (!fz.active) {
        if (fz.hasResumePos && p.framesSinceMove <= 5 && (p.allowedMoves & 0x30)) {
          fz.begin();
        } else if (p.isInDrawModeAndIdle(FUZE_IDLE_THRESHOLD)) {
          fz.begin();
        }
      }
      // Always update fuze state (it will deactivate when appropriate)
      fz.update();

      if (gameState == PLAYING) {
        saveBackground(p.position);
        drawPlayer();
        if (fz.active) saveFuzeBackground(fz.position);
        // Now draw the fuze on top
        fz.render();
        drawDebug();
      }
    } else if(gameState == FILL_ANIMATION) {
      restoreBackground();
      scanlineFill(currentFillVerts, currentFillCount, fillDith);
      drawPerimeter();
      saveBackground(p.position);
      drawPlayer();
    } else if (gameState == DEATH_ANIMATION) {
      drawDeathAnimation();
    }

    // Advance player's idle/movement frame counter every loop iteration
    p.tickFrame();

    arduboy.display();
}

