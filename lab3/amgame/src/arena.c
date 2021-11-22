#include "am.h"
#include "amdev.h"
#include <arena.h>
#include <klib-macros.h>
#include <stdint.h>
#include <string.h>

int arena_y = 0;
int arena_scale = 1;

#define BACKGROUND_COLOR 0xffffff

void arena_clear() {
  arena_fill_absolute(0, arena_y, ARENA_WIDTH, ARENA_HEIGHT, BACKGROUND_COLOR);
}

void arena_fill_absolute(int x, int y, int w, int h, uint32_t color) {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < arena_y) {
    h += y - arena_y;
    y = arena_y;
  }
  if (w <= 0 || h <= 0) {
    return;
  }
  if (x + w > ARENA_WIDTH) {
    w = ARENA_WIDTH - x;
  }
  if (y + h > arena_y + ARENA_HEIGHT) {
    h = arena_y + ARENA_HEIGHT - y;
  }
  
  // the stack is too small; we need a static array
  static uint32_t
    pixels[ARENA_HEIGHT * ARENA_WIDTH * MAX_ARENA_SCALE * MAX_ARENA_SCALE];

  AM_GPU_FBDRAW_T ev = {.x = x * arena_scale,
                        .y = (y - arena_y) * arena_scale,
                        .w = w * arena_scale,
                        .h = h * arena_scale,
                        .sync = 1,
                        .pixels = pixels};

  for (int i = 0; i < w * h * arena_scale * arena_scale; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &ev);
}

void arena_detect_scale() {
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  arena_scale = max(1, min(ev.height / ARENA_HEIGHT, ev.width / ARENA_WIDTH));
}
