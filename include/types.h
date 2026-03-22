#ifndef TYPES_H
#define TYPES_H

#include <termios.h>

typedef struct {
  int rows;
  int cols;
  struct termios orig_termios;
} TermState;

typedef struct {
  char query[512];
  char query_lower[512];
  int query_len;
  int query_changed;
  int selected;
  int scroll_offset;
  int old_selected;
  int ui_changed;
} UIState;

typedef struct applist {
  char *src;

  char **pathList;
  long *mtimeList;
  char **nameList;
  char **execCmdList;

  char **nameLowerList;
  char *nameLowerSrc;

  int *nameLenList;
  int count;
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
} App;

#endif
