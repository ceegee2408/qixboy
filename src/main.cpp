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
long score = 0;
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
vertex currentFillVerts[MAX_VERTICES];
int currentFillCount = 0;
int fillAnimationFrame = 0;
bool fillDith = false;
// How many frames idle in draw mode before fuze begins
const uint16_t FUZE_IDLE_THRESHOLD = 10;
GAMESTATE gameState = START_SCREEN; // change to START_SCREEN once implimented

namespace {
  using FrameHandler = void (*)();

  void tickMenuFrame() {
    // create menu sprite
    byte input = getInput();
    arduboy.setCursor(0, 0);
    arduboy.setTextSize(1);
    arduboy.print(F("Qixboy"));
    // temp
    if (input) {
      gameState = PLAYING;
    }
    // drawMenu();
  }

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
    if (fz.active) {
      saveFuzeBackground(fz.position);
      fz.render();
    }

    if (p.isInDrawMode()) {
      p.eraseTrail();
      q.update();
      p.drawTrail();
    } else {
      q.update();
    }
    fz.update();
    

    if (gameState != PLAYING) {
      return;
    }

    // Draw Qix (history + current line) only when q indicates render ready
    if (arduboy.everyXFrames(q.renderInterval)) {
      drawQix();
      eraseQixHistory();
      if(p.isInDrawMode()) {
        if (q.drawModeCheck()) {
          p.loseLife();
        }
      }
    }
    saveBackground(p.position);
    drawPlayer();
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

  void tickGameOverFrame() {
    // create game over sprite
    // drawScore();
  }

  constexpr FrameHandler FRAME_HANDLERS[] = {
    tickMenuFrame,
    tickPlayingFrame,
    tickFillAnimationFrame,
    tickDeathAnimationFrame,
    tickGameOverFrame
  };
}

void setup() {
    arduboy.boot();
    arduboy.setFrameRate(60);
    arduboy.clear();
    respawn();
    drawPerimeter();
    updateCanMove();
    saveBackground(p.position);
    drawPlayer();
    drawQix();
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
