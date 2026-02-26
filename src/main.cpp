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
vertex currentFillVerts[MAX_VERTICES];
int currentFillCount = 0;
int fillAnimationFrame = 0;
bool fillDith = false;
// How many frames idle in draw mode before fuze begins
const uint16_t FUZE_IDLE_THRESHOLD = 45;
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
    fz.update();

    // Erase Qix history before pixel checks, then update Qix movement
    eraseQixHistory();
    q.update();

    // Check Qix-trail collision: if player is drawing and Qix endpoint touches trail, kill player
    if (p.allowedMoves & 0x30) {  // player is in draw mode
      // Trail pixels are white on framebuffer; check if either Qix endpoint is on trail
      if (arduboy.getPixel(q.p1.getx(), q.p1.gety()) ||
          arduboy.getPixel(q.p2.getx(), q.p2.gety())) {
        p.loseLife();
      }
    }

    if (gameState != PLAYING) {
      return;
    }

    saveBackground(p.position);
    drawPlayer();
    // Draw Qix (history + current line)
    drawQix();
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
