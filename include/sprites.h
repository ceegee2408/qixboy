// sprites.h - external sprite data
#ifndef SPRITES_H
#define SPRITES_H

#include <Arduino.h>
#include "config.h"

// Player sprite (SPRITE_SIZE x SPRITE_SIZE), MSB = leftmost pixel.
extern const uint8_t PROGMEM playerSpriteRows[SPRITE_SIZE];

// Fuze sprite frames: [frame][row]
extern const uint8_t PROGMEM fuzeSpriteFrames[][SPRITE_SIZE];

// Sparx uses a 7x7 sprite; declare its size and frames
#define SPARX_SIZE 7
extern const uint8_t PROGMEM sparxSpriteFrames[][SPARX_SIZE];

// Death sprite arrays
extern const uint8_t PROGMEM deathSpriteRows[SPRITE_SIZE][/*frames*/5];

#endif // SPRITES_H
