#ifndef AMGAME_BALL_H
#define AMGAME_BALL_H

#include "platform.h"
#include "common.h"

#define BALL_RADIUS 5
#define BALL_SPEED_X 100
#define BALL_SPEED_Y 100

typedef struct {
  /* (x, y) points to the top-left corner of the ball
   * here -> +/--\
   *          |  |
   *          \--/
   */
  int x;
  int y;
  int dx;
  int dy;
} Ball;

extern Ball ball;

void ball_draw(const Ball *ball);

void ball_fix_collision(Ball *ball, const Platform *platform);

#endif
