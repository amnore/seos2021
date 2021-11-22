#ifndef AMGAME_PLATFORM_H
#define AMGAME_PLATFORM_H

#include <common.h>

#define PLATFORM_HEIGHT 3
#define PLATFORM_WIDTH 30

#define MAX_PLATFORM_NR 32

typedef struct {
  int x;
  int y;
} Platform;

// we use a circular buffer to store all platforms
extern Platform platforms[MAX_PLATFORM_NR];
extern int platform_begin, platform_end;

extern int last_platform_y;

void platform_draw(Platform *platform);

// generate a new platform at position y
void platform_generate(int y);
#endif
