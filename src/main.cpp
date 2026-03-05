#define EEPROM_HIGH_SCORE_ADDR 0 // Address in EEPROM to store high score (2 bytes)

#include <Arduino.h>
#include <Arduboy2.h>
#include "config.h"
#include "entities.h"
#include "geometry.h"
#include "rendering.h"
#include "player_logic.h"
#include "EEPROM.h"

// Global game objects
long score = 0;
int highScores[6] = {
  0, 0, 0, 0, 0, 0
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
    if (input) {
      arduboy.clear();
      respawn();
      drawPerimeter();
    }
    // drawMenu();
  }

  void tickPlayingFrame() {
    /*
    Rendering priority:
      During Gameplay:
        1. Restore previous frame sprites (fuze first, then player)
        2. Process input / movement / trail
        3. Update enemies (qix, fuze)
        4. Draw qix
        5. Save backgrounds, then draw sprites (player, fuze)
    */

    // 1. Restore previous frame's sprites
    restoreBackground(BG_FUZE);
    restoreBackground(BG_PLAYER);

    // 2. Process input and movement
    byte input = getInput();
    updateActiveDirection(input);
    updatePlayerPosition(input);
    saveBackground(p.position, BG_PLAYER);
    drawPlayer();

    // 3. Trail handling: erase trail, update qix (collision uses buffer), redraw trail
    if (p.isInDrawMode()) {
      p.eraseTrail();
      q.update();
      p.drawTrail();
    } else {
      q.update();
    }

    // 4. Fuze activation and update
    if (!fz.active) {
      if (fz.hasResumePos && p.getFramesSinceMove() <= 5 && (p.allowedMoves & 0x30)) {
        fz.begin();
      } else if (p.isInDrawModeAndIdle(FUZE_IDLE_THRESHOLD)) {
        fz.begin();
      }
    }
    fz.update();

    // If state changed (e.g. death or fill), bail out before drawing
    if (gameState != PLAYING) {
      return;
    }

    // 5. Draw Qix (history + current line) only when q indicates render ready
    if (arduboy.everyXFrames(q.renderInterval)) {
      drawQix();
      eraseQixHistory();
      if(p.isInDrawMode()) {
        if (q.isCollidingWithPlayer()) {
          p.loseLife();
        }
      }
    }

    // 6. Save backgrounds and draw sprites


    if (fz.active) {
      saveBackground(fz.position, BG_FUZE);
      fz.render();
    }

    drawDebug();
  }

  void tickLevelComplete() {
    // to be filled
  }

  void tickFillAnimationFrame() {
    restoreBackground(BG_PLAYER);
    scanlineFill(currentFillVerts, currentFillCount, fillDith);
    drawPerimeter();
    saveBackground(p.position, BG_PLAYER);
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
    tickLevelComplete,
    tickFillAnimationFrame,
    tickDeathAnimationFrame,
    tickGameOverFrame
  };
}

void setup() {
    arduboy.boot();
    arduboy.flashlight();
    arduboy.setFrameRate(60);
    arduboy.clear();
    arduboy.display();
}

void loop() {
    if (!arduboy.nextFrame()) return;
        frameCounter++;
    if (frameCounter >= 255) frameCounter = 0;

    // render next frame based on game state
    const FrameHandler handler = FRAME_HANDLERS[gameState];
    if (handler != nullptr) {
        handler();
    }

    arduboy.display();
}
