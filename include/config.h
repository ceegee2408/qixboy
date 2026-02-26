#ifndef CONFIG_H
#define CONFIG_H

#define NUM_LIVES 3
#define PLAYER_SIZE 2
#define SPRITE_SIZE (PLAYER_SIZE * 2 + 1)  // 5x5; must satisfy SPRITE_SIZE^2 <= 32
#define FAST_MOVE 3
#define SLOW_MOVE 5
#define MAX_VERTICES 128
#define DEBUG 0

// Fuze configuration: animation ticks per sprite frame, and movement interval
// (in game frames). Tweak these to change fuze animation speed and travel.
#define FUZE_FRAME_TICKS 5
#define FUZE_MOVE_INTERVAL 2

#endif // CONFIG_H
