// sprites.h - external sprite data
#ifndef SPRITES_H
#define SPRITES_H

#include <Arduino.h>
#include "config.h"

// Player sprite (SPRITE_SIZE x SPRITE_SIZE), MSB = leftmost pixel.
extern const uint8_t PROGMEM playerSpriteRows[SPRITE_SIZE];

#endif // SPRITES_H
