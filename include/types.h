#ifndef TYPES_H
#define TYPES_H

#include <termios.h>
#include "config.h"

typedef struct {
  int rows;
  int cols;
  struct termios orig_termios;
} TermState;

typedef struct {
  char query[128];
  char query_lower[128];
  int query_len;
  int cursor_pos;
  int query_changed;
  int selected;
  int scroll_offset;
  int old_selected;
  int ui_changed;
  char calc_result[64];
  char mode[8];
} UIState;

typedef struct app_keywords {
  char **keywords;
  int keyword_count;
} AppKeywords;

typedef struct applist {
  char *src;
  int src_is_mmaped;
  long src_size;

  char **pathList;
  long *mtimeList;
  char **nameList;
  char **execCmdList;

  char **nameLowerList;
  char *nameLowerSrc;

  int *nameLenList;
  int count;

  AppKeywords *keywords;
} AppList;

typedef struct {
  char *name;
  char *exec;
  int score;
} Match;

typedef struct {
  TermState term;
  UIState ui;
  AppList apps;

  Match *top;
  int top_n;
  int app_count;
  int search_limit;
  int has_more_results;

  Config *config;
} App;

#endif
