#include "editor.h"
#include "am.h"
#include "amdev.h"
#include "framebuffer.h"
#include "klib-macros.h"
#include "kvector.h"
#include <stdint.h>

#define EDITOR_MAX_FPS 60

#define AM_KEY_UP 0x4d
#define AM_KEY_DOWN 0x4e
#define AM_KEY_LEFT 0x4f
#define AM_KEY_RIGHT 0x50

#define MAP(f, c) c(f)
#define LETTER_KEYS(f)                                                         \
  f(Q) f(W) f(E) f(R) f(T) f(Y) f(U) f(I) f(O) f(P) f(A) f(S) f(D) f(F) f(G)   \
      f(H) f(J) f(K) f(L) f(Z) f(X) f(C) f(V) f(B) f(N) f(M)
#define NUMBER_KEYS(f) f(1) f(2) f(3) f(4) f(5) f(6) f(7) f(8) f(9) f(0)
#define NORMAL_KEYS(f) NUMBER_KEYS(f) LETTER_KEYS(f)
#define DIRECTION_KEYS(f) f(UP) f(DOWN) f(LEFT) f(RIGHT)

#define GET_TIME() io_read(AM_TIMER_UPTIME)

KVECTOR_DEFINE_OPS(Line, char);
KVECTOR_DEFINE_OPS(Buffer, Line);
KVECTOR_DEFINE_OPS(History, HistoryEntry);
KVECTOR_DEFINE_OPS(SearchResults, SearchResult);

static EditorStatus status;
static AM_TIMER_UPTIME_T last_clear_time;
static FrameBufferTexture *font_normal, *font_red, *font_highlight;

static void editor_insert_line(int pos) {
  Line l;
  Line_init(&l);
  Buffer_insert(&status.buffer, pos, l);
}

static void editor_init() {
  Buffer_init(&status.buffer);
  History_init(&status.history);
  SearchResults_init(&status.search_results);
  Line_init(&status.search_content);
  editor_insert_line(0);
  status.mode = EDITOR_MODE_INSERT;
  status.cursor = status.view = (Position){0, 0};
  font_normal = framebuffer_load_font(0xffffff, 0);
  font_red = framebuffer_load_font(0xff0000, 0);
  font_highlight = framebuffer_load_font(0, 0xffffff);
}

static void editor_draw_line(Line *l, int begin_column, int row,
                             FrameBufferTexture *font) {
  int width = framebuffer_get_width();
  int empty_column = 0;

  if (l) {
    empty_column = MAX(0, MIN(Line_size(l) - begin_column, width));
    for (int j = 0; j < empty_column; j++) {
      framebuffer_putch(j, row, Line_at(l, begin_column + j), font);
    }
  }

  for (int j = empty_column; j < width; j++) {
    framebuffer_putch(j, row, ' ', font);
  }
}

static void editor_repaint() {
  Position view = status.view;
  Buffer *buf = &status.buffer;
  int height = framebuffer_get_height(), width = framebuffer_get_width();

  for (int i = 0; i < height; i++) {
    if (view.row + i >= Buffer_size(buf)) {
      editor_draw_line(NULL, 0, i, font_normal);
    } else {
      editor_draw_line(&buf->arr[view.row + i], view.column, i, font_normal);
    }
  }

  Position cur = status.cursor;
  Line *cur_line = &buf->arr[cur.row];
  framebuffer_putch(
      cur.column - view.column, cur.row - view.row,
      cur.column >= Line_size(cur_line) ? ' ' : Line_at(cur_line, cur.column),
      font_highlight);

  if (status.mode == EDITOR_MODE_SEARCH) {
    editor_draw_line(&status.search_content, 0, height - 1, font_red);

    SearchResult *begin = status.search_results.arr,
                 *end = status.search_results.arr + status.search_results.size;
    while (begin != end && begin->begin.row < view.row) {
      begin++;
    }

    for (; begin != end && begin->begin.row < view.row + height; begin++) {
      if (begin->end.column < view.column ||
          begin->begin.column >= view.column + width)
        continue;

      Line *l = &buf->arr[begin->begin.row];
      for (int i = MAX(begin->begin.column, view.column);
           i < MIN(begin->end.column, view.column + width); i++) {
        framebuffer_putch(i - view.column, begin->begin.row - view.row,
                          Line_at(l, i), font_red);
      }
    }
  }

  framebuffer_repaint();
}

static void editor_toggle_mode() {
  if (status.mode == EDITOR_MODE_INSERT) {
    status.mode = EDITOR_MODE_SEARCH;
  } else {
    status.mode = EDITOR_MODE_INSERT;
    last_clear_time = GET_TIME();
    Line_clear(&status.search_content);
    SearchResults_clear(&status.search_results);
  }
}

static void editor_move_cursor(Position pos) {
  Position *v = &status.view;
  Buffer *b = &status.buffer;
  Position *c = &status.cursor;
  int h = framebuffer_get_height(), w = framebuffer_get_width();

  pos.row = MAX(0, MIN(pos.row, Buffer_size(b) - 1));
  pos.column = MAX(0, MIN(pos.column, b->arr[pos.row].size));
  *c = pos;

  if (c->row < v->row) {
    v->row = c->row;
  } else if (c->row >= v->row + h) {
    v->row = c->row - h + 1;
  } else if (c->column < v->column) {
    v->column = c->column;
  } else if (c->column >= v->column + w) {
    v->column = c->column - w + 1;
  }
}

static void editor_move_cursor_by_key(int key) {
  Position *c = &status.cursor;
  switch (key) {
  case AM_KEY_LEFT:
    editor_move_cursor((Position){c->row, c->column - 1});
    break;
  case AM_KEY_RIGHT:
    editor_move_cursor((Position){c->row, c->column + 1});
    break;
  case AM_KEY_UP:
    editor_move_cursor((Position){c->row - 1, c->column});
    break;
  case AM_KEY_DOWN:
    editor_move_cursor((Position){c->row + 1, c->column});
    break;
  }
}

static char editor_keycode_to_char(int key) {

#define LETTER_KEY_TO_CHAR(x)                                                  \
  case AM_KEY_##x: {                                                           \
    InputStatus s = status.input_status;                                       \
    if (s.shift ^ s.capslock) {                                                \
      return #x[0];                                                            \
    } else {                                                                   \
      return #x[0] + ('a' - 'A');                                              \
    }                                                                          \
  }

#define NUMBER_KEY_TO_CHAR(x)                                                  \
  case AM_KEY_##x: {                                                           \
    return #x[0];                                                              \
  }

  switch (key) {
    // clang-format off
  MAP(LETTER_KEY_TO_CHAR, LETTER_KEYS);
  MAP(NUMBER_KEY_TO_CHAR, NUMBER_KEYS);
  // clang-format on
  case AM_KEY_SPACE:
    return ' ';
  case AM_KEY_TAB:
    return '\t';
  case AM_KEY_RETURN:
    return '\n';
  default:
    panic("Invalid key\n");
  }

#undef LETTER_KEY_TO_CHAR
#undef NUMBER_KEY_TO_CHAR
}

static void editor_insert_linebreak(bool add_history) {
  Position *cur = &status.cursor;
  Buffer *buf = &status.buffer;

  editor_insert_line(cur->row + 1);
  Line *curr_line = &buf->arr[cur->row], *new_line = &buf->arr[cur->row + 1];
  if (add_history) {
    History_push_back(&status.history,
                      (HistoryEntry){.type = HISTORY_TYPE_INSERT,
                                     .ch = '\n',
                                     .nchars = 1,
                                     .pos = (Position){cur->row + 1, 0}});
  }
  for (int i = cur->column; i < curr_line->size; i++) {
    Line_push_back(new_line, Line_at(curr_line, i));
  }
  curr_line->size = cur->column;
  editor_move_cursor((Position){cur->row + 1, 0});
}

static void editor_insert_char(char c, bool add_history) {
  if (c == '\n') {
    editor_insert_linebreak(add_history);
    return;
  }

  Position *cur = &status.cursor;
  Buffer *buf = &status.buffer;

  Line *curr_line = &buf->arr[cur->row];

  int char_width = c == '\t' ? 4 : 1;
  c = c == '\t' ? ' ' : c;
  if (add_history) {
    History_push_back(
        &status.history,
        (HistoryEntry){.type = HISTORY_TYPE_INSERT,
                       .ch = c,
                       .nchars = char_width,
                       .pos = (Position){cur->row, cur->column + 1}});
  }
  for (int i = 0; i < char_width; i++) {
    Line_insert(curr_line, cur->column, c);
    editor_move_cursor((Position){cur->row, cur->column + 1});
  }
}

static void editor_join_line(bool add_history) {
  Position *cur = &status.cursor;
  Buffer *buf = &status.buffer;
  Line *curr_line = &buf->arr[cur->row];

  if (cur->row == 0) {
    return;
  }

  Line *prev_line = &buf->arr[cur->row - 1];
  int sz = prev_line->size;
  if (add_history) {
    History_push_back(
        &status.history,
        (HistoryEntry){.ch = '\n',
                       .nchars = 1,
                       .type = HISTORY_TYPE_DELETE,
                       .pos = (Position){cur->row - 1, Line_size(prev_line)}});
  }
  for (int i = 0; i < curr_line->size; i++) {
    Line_push_back(prev_line, Line_at(curr_line, i));
  }
  Line_drop(curr_line);
  Buffer_erase(buf, cur->row);
  *cur = (Position){.row = cur->row - 1, .column = sz};
}

static void editor_delete_char(bool add_history) {
  Position *cur = &status.cursor;
  Buffer *buf = &status.buffer;
  Line *curr_line = &buf->arr[cur->row];

  if (cur->column == 0) {
    editor_join_line(add_history);
    return;
  }

  bool has_tab = cur->column >= 4;
  if (has_tab) {
    for (int i = 1; i <= 4; i++) {
      if (Line_at(curr_line, cur->column - i) != ' ') {
        has_tab = false;
        break;
      }
    }
  }

  int chars_to_erase = has_tab ? 4 : 1;
  if (add_history) {
    History_push_back(
        &status.history,
        (HistoryEntry){.ch = Line_at(curr_line, cur->column - chars_to_erase),
                       .nchars = chars_to_erase,
                       .type = HISTORY_TYPE_DELETE,
                       .pos =
                           (Position){cur->row, cur->column - chars_to_erase}});
  }
  for (int i = 0; i < chars_to_erase; i++) {
    Line_erase(curr_line, cur->column - 1);
    editor_move_cursor((Position){cur->row, cur->column - 1});
  }
}

static void editor_toggle_input_status(AM_INPUT_KEYBRD_T ev) {
  InputStatus *s = &status.input_status;
  switch (ev.keycode) {
  case AM_KEY_CAPSLOCK:
    if (ev.keydown) {
      s->capslock ^= true;
    }
    break;
  case AM_KEY_LSHIFT:
    s->lshift = ev.keydown;
    break;
  case AM_KEY_RSHIFT:
    s->rshift = ev.keydown;
    break;
  case AM_KEY_LCTRL:
    s->lctrl = ev.keydown;
    break;
  case AM_KEY_RCTRL:
    s->rctrl = ev.keydown;
    break;
  }

  s->shift = s->lshift || s->rshift;
  s->ctrl = s->lctrl || s->rctrl;
}

static void editor_do_search() {
  if (!SearchResults_empty(&status.search_results) ||
      Line_empty(&status.search_content)) {
    return;
  }

  Buffer *buf = &status.buffer;
  Line *search = &status.search_content;
  for (int i = 0; i < buf->size; i++) {
    Line *l = &buf->arr[i];
    for (int j = 0; j <= l->size - search->size;) {
      bool match = true;
      for (int k = 0; k < search->size; k++) {
        if (Line_at(search, k) != Line_at(l, j + k)) {
          match = false;
          break;
        }
      }

      if (!match) {
        j++;
        continue;
      }

      SearchResults_push_back(&status.search_results,
                              (SearchResult){{i, j}, {i, j + search->size}});
      j += search->size;
    }
  }
}

static void editor_undo() {
  if (History_empty(&status.history)) {
    return;
  }

  HistoryEntry e = History_back(&status.history);
  History_pop_back(&status.history);

  for (int i = 0; i < e.nchars; i++) {
    if (e.type == HISTORY_TYPE_DELETE) {
      editor_move_cursor(e.pos);
      editor_insert_char(e.ch, false);
    } else {
      editor_move_cursor(e.pos);
      editor_delete_char(false);
    }
  }
}

static bool editor_handle_key() {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode == AM_KEY_NONE) {
    return true;
  }

  printf("%s %x\n", ev.keydown ? "Keydown" : "Keyup", ev.keycode);

  switch (ev.keycode) {
  case AM_KEY_LSHIFT:
  case AM_KEY_RSHIFT:
  case AM_KEY_CAPSLOCK:
  case AM_KEY_LCTRL:
  case AM_KEY_RCTRL:
    editor_toggle_input_status(ev);
    return true;
  }

  if (!ev.keydown) {
    return true;
  }

  if (status.input_status.ctrl) {
    if (ev.keydown && ev.keycode == AM_KEY_Z) {
      editor_undo();
    }

    return true;
  }

#define CASE_KEYS(x) case AM_KEY_##x:

  switch (ev.keycode) {
  case AM_KEY_F2:
    return false;
  case AM_KEY_ESCAPE:
    editor_toggle_mode();
    break;
    // clang-format off
  MAP(CASE_KEYS, DIRECTION_KEYS)
    // clang-format on
    if (status.mode == EDITOR_MODE_INSERT) {
      editor_move_cursor_by_key(ev.keycode);
    }
    break;
    // clang-format off
  MAP(CASE_KEYS, NORMAL_KEYS)
    // clang-format on
  case AM_KEY_TAB:
  case AM_KEY_SPACE: {
    char c = editor_keycode_to_char(ev.keycode);
    if (status.mode == EDITOR_MODE_SEARCH) {
      if (SearchResults_empty(&status.search_results)) {
        Line_push_back(&status.search_content, c);
      }
    } else {
      editor_insert_char(c, true);
    }
    break;
  }
  case AM_KEY_RETURN:
    if (status.mode == EDITOR_MODE_SEARCH) {
      editor_do_search();
    } else {
      editor_insert_char('\n', true);
    }
    break;
  case AM_KEY_BACKSPACE:
    if (status.mode == EDITOR_MODE_SEARCH) {
      if (SearchResults_empty(&status.search_results) &&
          !Line_empty(&status.search_content)) {
        Line_pop_back(&status.search_content);
      }
    } else {
      editor_delete_char(true);
    }
    break;
  }

#undef CASE_KEYS

  return true;
}

static void editor_wait_until_next_frame() {
  static AM_TIMER_UPTIME_T last_frame_time;
  uint64_t wait_interval = 1000000 / EDITOR_MAX_FPS;

  if (last_frame_time.us == 0) {
    last_frame_time = GET_TIME();
  }

  AM_TIMER_UPTIME_T current_time;
  for (current_time = GET_TIME();
       current_time.us - last_frame_time.us < wait_interval;
       current_time = GET_TIME()) {
    continue;
  }
  last_frame_time = current_time;
}

static void editor_maybe_clear_screen() {
  uint64_t clear_interval = 1000000 * 20;

  if (last_clear_time.us == 0) {
    last_clear_time = GET_TIME();
  }

  if (status.mode == EDITOR_MODE_SEARCH) {
    return;
  }

  AM_TIMER_UPTIME_T current_time = GET_TIME();
  if (current_time.us - last_clear_time.us > clear_interval) {
    last_clear_time = current_time;
    printf("Cleared screen!\n");
    editor_init();
  }
}

static void editor_main_loop() {
  while (true) {
    editor_maybe_clear_screen();
    if (!editor_handle_key()) {
      return;
    }
    editor_repaint();
    editor_wait_until_next_frame();
  }
}

void start_editor() {
  framebuffer_init();
  editor_init();
  editor_main_loop();
}
