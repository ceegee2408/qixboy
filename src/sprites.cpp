#include "sprites.h"

// Diamond outline, SPRITE_SIZE x SPRITE_SIZE (5x5), MSB = leftmost pixel.
//   Row 0:  00100  = 0x20
//   Row 1:  01010  = 0x50
//   Row 2:  10001  = 0x88
//   Row 3:  01010  = 0x50
//   Row 4:  00100  = 0x20
const uint8_t PROGMEM playerSpriteRows[SPRITE_SIZE] = {
    0x20, 0x50, 0x88, 0x50, 0x20
};
