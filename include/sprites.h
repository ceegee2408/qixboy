// sprites.h - external sprite data
#ifndef SPRITES_H
#define SPRITES_H
#include <Arduino.h>
#include "config.h"

// Player sprite (SPRITE_SIZE x SPRITE_SIZE), MSB = leftmost pixel.
extern const uint8_t PROGMEM playerSpriteRows[SPRITE_SIZE];

// Fuze sprite frames: [frame][row]
// Define FUZE_FRAME_COUNT to match the frames in `sprites.cpp`.
#define FUZE_FRAME_COUNT 4
extern const uint8_t PROGMEM fuzeSpriteFrames[][SPRITE_SIZE];

// Sparx uses a 7x7 sprite; declare its size and frames
#define SPARX_SIZE 7
extern const uint8_t PROGMEM sparxSpriteFrames[][SPARX_SIZE];

// 5x7 digit sprites (0–9). Each row byte is MSB-left: bit7=col0 … bit3=col4.
#define DIGIT_HEIGHT 7
#define DIGIT_WIDTH  5
extern const uint8_t PROGMEM digitSprites[10][DIGIT_HEIGHT];

#endif // SPRITES_H
