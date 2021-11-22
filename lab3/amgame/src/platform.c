#include "klib-macros.h"
#include <arena.h>
#include <klib.h>
#include <platform.h>

#define PLATFORM_COLOR 0x000000

int platform_begin = 0;
int platform_end = 0;
int last_platform_y = 0;
Platform platforms[MAX_PLATFORM_NR];

void platform_draw(Platform *platform) {
  arena_fill_absolute(platform->x, platform->y, PLATFORM_WIDTH, PLATFORM_HEIGHT,
                      PLATFORM_COLOR);
}

void platform_generate(int y) {
  int i = (platform_end + 1) % MAX_PLATFORM_NR;
  assert(platform_begin != i);
  platforms[platform_end] =
      (Platform){.x = rand() % (ARENA_WIDTH - PLATFORM_WIDTH), .y = y};
  platform_end = i;
  last_platform_y = max(last_platform_y, y);
}
