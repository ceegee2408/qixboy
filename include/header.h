#ifndef HEADER_H
#define HEADER_H

#define MAX_VECTORS 120
#define MAX_TRAIL_VECTORS 30
#define MAX_SPRITES 6
#define DEBUG INFO
#define SLOW_DRAW_SPEED 3
#define FAST_DRAW_SPEED 2
#define NORMAL_SPEED 2
#define RESPAWN true

#include <Arduboy2.h>
#include "vector.h"
#include "debug.h"
#include "bitmaps.h"
#include "handleRender.h"
#include "direction.h"
#include "trail.h"
#include "perimeter.h"


byte mod(byte a, byte b)
{
    return (a % b + b) % b;
}

#endif
