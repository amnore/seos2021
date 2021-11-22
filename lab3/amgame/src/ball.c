#include "klib-macros.h"
#include <arena.h>
#include <ball.h>
#include <platform.h>
#include <stdbool.h>

#define BALL_COLOR 0xff0000

Ball ball;

void ball_draw(const Ball *ball) {
  arena_fill_absolute(ball->x + 4, ball->y, 2, 10, BALL_COLOR);
  arena_fill_absolute(ball->x + 1, ball->y + 1, 8, 8, BALL_COLOR);
  arena_fill_absolute(ball->x, ball->y + 4, 10, 2, BALL_COLOR);
}

static bool ball_check_collision(const Ball *ball, const Platform *platform) {
  return !(ball->x + BALL_RADIUS * 2 <= platform->x ||
           ball->x > platform->x + PLATFORM_WIDTH ||
           ball->y + 2 * BALL_RADIUS <= platform->y ||
           ball->y > platform->y + PLATFORM_HEIGHT);
}

static Ball ball_reverse_to_collision(const Ball *ball,
                                      const Platform *platform) {
  Ball rev = *ball;
  while (ball_check_collision(&rev, platform)) {
    if (rev.dy > 0)
      rev.y--;
    if (rev.dx < 0)
      rev.x++;
    else if (rev.dx > 0)
      rev.x--;
  }
  return rev;
}

void ball_fix_collision(Ball *ball, const Platform *platform) {
  if (!ball_check_collision(ball, platform))
    return;

  Ball rev = ball_reverse_to_collision(ball, platform);
  if (rev.x + 2 * BALL_RADIUS <= platform->x /*from left*/ ||
      rev.x >= platform->x + PLATFORM_WIDTH /*from right*/)
    ball->x = rev.x;
  else /*from top*/
    ball->y = rev.y;
}
