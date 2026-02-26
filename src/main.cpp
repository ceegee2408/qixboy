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

namespace {
  using FrameHandler = void (*)();

  void tickPlayingFrame() {
    restoreFuzeBackground();
    restoreBackground();

    byte input = getInput();
    updateActiveDirection(input);
    updatePlayer(input);

    if (!fz.active) {
      if (fz.hasResumePos && p.framesSinceMove <= 5 && (p.allowedMoves & 0x30)) {
        fz.begin();
      } else if (p.isInDrawModeAndIdle(FUZE_IDLE_THRESHOLD)) {
        fz.begin();
      }
    }
    fz.update();

    if (gameState != PLAYING) {
      return;
    }

    saveBackground(p.position);
    drawPlayer();
    if (fz.active) {
      saveFuzeBackground(fz.position);
      fz.render();
    }
    drawDebug();
  }

  void tickFillAnimationFrame() {
    restoreBackground();
    scanlineFill(currentFillVerts, currentFillCount, fillDith);
    drawPerimeter();
    saveBackground(p.position);
    drawPlayer();
  }

  void tickDeathAnimationFrame() {
    drawDeathAnimation();
  }

  constexpr FrameHandler FRAME_HANDLERS[] = {
    nullptr,
    tickPlayingFrame,
    tickFillAnimationFrame,
    tickDeathAnimationFrame,
    nullptr
  };
}

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

    const FrameHandler handler = FRAME_HANDLERS[gameState];
    if (handler != nullptr) {
      handler();
    }
    p.tickFrame();
    arduboy.display();
}
