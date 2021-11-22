#include "am.h"
#include "amdev.h"
#include "editor.h"
#include "klib-macros.h"
#include <arena.h>
#include <ball.h>
#include <klib.h>
#include <platform.h>

#define TARGET_FPS 50
#define UPDATE_INTERVAL_US (1000000 / TARGET_FPS)
#define MIN_PLATFORM_Y_DISTANCE (ARENA_HEIGHT / 10)
#define MAX_PLATFORM_Y_DISTANCE (ARENA_HEIGHT / 2)

void splash();

void wait_until_next_frame() {
  AM_TIMER_UPTIME_T wait_begin = io_read(AM_TIMER_UPTIME);
  while (io_read(AM_TIMER_UPTIME).us - wait_begin.us < UPDATE_INTERVAL_US) {
    continue;
  }
}

void update_objects() {
  // update the arena
  arena_y += ARENA_SPEED / TARGET_FPS;

  // move the ball
  ball.x += ball.dx / TARGET_FPS;
  ball.y += ball.dy / TARGET_FPS;
  // horizontally limit the ball in the arena
  if (ball.x < 0)
    ball.x = 0;
  else if (ball.x + 2 * BALL_RADIUS >= ARENA_WIDTH)
    ball.x = ARENA_WIDTH - 2 * BALL_RADIUS;
  // check for collision
  for (int i = platform_begin; i != platform_end;
       i = (i + 1) % MAX_PLATFORM_NR) {
    ball_fix_collision(&ball, &platforms[i]);
  }
  // reset speed
  ball.dy = BALL_SPEED_Y;

  // remove out of arena platforms
  while (platforms[platform_begin].y + PLATFORM_HEIGHT < arena_y)
    platform_begin = (platform_begin + 1) % MAX_PLATFORM_NR;
  // conditionally generate platforms
  int generate_y = arena_y + ARENA_HEIGHT;
  if ((platform_end + 1) % MAX_PLATFORM_NR != platform_begin &&
      generate_y - last_platform_y >= MIN_PLATFORM_Y_DISTANCE &&
      rand() % MAX_PLATFORM_Y_DISTANCE < generate_y - last_platform_y) {
    // generate a new one below the arena
    platform_generate(generate_y);
  }
}

bool check_gameover() {
  return ball.y < arena_y || ball.y + 2 * BALL_RADIUS > arena_y + ARENA_HEIGHT;
}

void reset_game() {
  arena_y = 0;

  ball.x = ARENA_WIDTH / 2 - BALL_RADIUS;
  ball.y = ARENA_HEIGHT / 3;
  ball.dx = ball.dy = 0;

  platform_begin = platform_end = 0;
  last_platform_y = 0;
  // place a platform below the ball
  platforms[platform_end++] = (Platform){
      .x = (ARENA_WIDTH - PLATFORM_WIDTH) / 2, .y = ball.y + BALL_RADIUS * 2};
  for (int y = platforms[platform_begin].y + ARENA_HEIGHT / 10;
       y < ARENA_HEIGHT - PLATFORM_HEIGHT; y += ARENA_HEIGHT / 10) {
    platform_generate(y);
  }
}

void handle_input() {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);

  if (ev.keydown) {
    switch (ev.keycode) {
    case AM_KEY_A:
      ball.dx = max(ball.dx - BALL_SPEED_X, -BALL_SPEED_X);
      break;
    case AM_KEY_D:
      ball.dx = min(ball.dx + BALL_SPEED_X, BALL_SPEED_X);
      break;
    case AM_KEY_ESCAPE:
      halt(0);
      break;
    }
  }
}

void draw_frame() {
  arena_clear();
  ball_draw(&ball);
  for (int i = platform_begin; i != platform_end;
       i = (i + 1) % MAX_PLATFORM_NR) {
    platform_draw(&platforms[i]);
  }
}

void main_loop() {
  while (true) {
    wait_until_next_frame();
    update_objects();
    handle_input();
    draw_frame();
    if (check_gameover()) {
      reset_game();
    }
  }
}

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  splash();
  start_editor();
  arena_detect_scale();
  reset_game();
  main_loop();
  return 0;
}
