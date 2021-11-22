#ifndef AMGAME_ARENA_H
#define AMGAME_ARENA_H

#include <stdint.h>

#include "common.h"

#define ARENA_HEIGHT 200
#define ARENA_WIDTH 150
#define ARENA_SPEED 50
#define MAX_ARENA_SCALE 4

extern int arena_y;
extern int arena_scale;

void arena_clear();

void arena_fill_absolute(int x, int y, int w, int h, uint32_t color);

void arena_detect_scale();

#endif
