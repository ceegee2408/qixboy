// gamestate.h - centralized GAMESTATE enum and extern
#ifndef GAMESTATE_H
#define GAMESTATE_H

enum GAMESTATE {
    START_SCREEN,
    PLAYING,
    FILL_ANIMATION,
    DEATH_ANIMATION,
    GAME_OVER
};

extern GAMESTATE gameState;
extern void initializeDeathAnimation();

#endif // GAMESTATE_H
