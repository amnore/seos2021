#ifndef SEOS2021_LAB3_EDITOR_H
#define SEOS2021_LAB3_EDITOR_H

#include "kvector.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct Position {
  int row;
  int column;
} Position;

typedef struct HistoryEntry {
  enum {
    HISTORY_TYPE_INSERT,
    HISTORY_TYPE_DELETE,
  } type;
  Position pos;
  int nchars;
  char ch;
} HistoryEntry;

typedef struct InputStatus {
  bool lshift;
  bool rshift;
  bool lctrl;
  bool rctrl;

  bool capslock;
  bool shift;
  bool ctrl;
} InputStatus;

typedef enum EditorMode { EDITOR_MODE_INSERT, EDITOR_MODE_SEARCH } EditorMode;

typedef struct SearchResult {
  Position begin;
  Position end;
} SearchResult;

KVECTOR_DEFINE_TYPE(Line, char);
KVECTOR_DEFINE_TYPE(Buffer, Line);
KVECTOR_DEFINE_TYPE(History, HistoryEntry);
KVECTOR_DEFINE_TYPE(SearchResults, SearchResult);

typedef struct EditorStatus {
  Buffer buffer;
  History history;
  SearchResults search_results;
  Line search_content;
  Position cursor;
  Position view;
  EditorMode mode;
  InputStatus input_status;
} EditorStatus;

void start_editor();

#endif // SEOS2021_LAB3_EDITOR_H
