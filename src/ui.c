#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "ui.h"

static void printf_cols(int cols, const char *fmt, ...) {
  if (cols <= 0)
    return;

  static char buf[2048];
  
  va_list args;
  va_start(args, fmt);

  va_list copy;
  va_copy(copy, args);
  int len = vsnprintf(NULL, 0, fmt, copy);
  va_end(copy);

  if (len < 0 || len >= (int)sizeof(buf)) {
    va_end(args);
    return;
  }

  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  int visible = 0;
  for (int i = 0; buf[i] != '\0' && visible < cols;) {
    if (buf[i] == '\x1b' && buf[i + 1] == '[') {
      putchar(buf[i++]); // ESC
      putchar(buf[i++]); // [
      while (buf[i] && buf[i] != 'm') {
        putchar(buf[i++]);
      }
      if (buf[i] == 'm') {
        putchar(buf[i++]);
      }
    } else {
      putchar(buf[i++]);
      visible++;
    }
  }
}

void clearResUi(int rows) {
  for (int i = rows - 3; i > 0; i--)
    printf("\x1b[%d;1H\x1b[2K", i);
}

void basicFrame(int *ui_changed, TermState *term) {
  int termRows = term->rows;
  int termCols = term->cols;
  printf("\x1b[%d;1H╭", termRows - 2);
  for (int i = 0; i < termCols - 2; i++)
    printf("─");
  printf("╮");
  printf_cols(term->cols, "\x1b[%d;%dH\x1b[2K│ search...", termRows - 1, 1);
  printf("\x1b[%d;%dH│", termRows - 1, termCols);
  printf("\x1b[%d;1H╰", termRows);
  for (int i = 2; i < termCols; i++)
    printf("─");
  printf("╯");
  clearResUi(term->rows);
  *ui_changed = 1;
}

void printResults(int rows, int cols, Match *top, int top_n, int scroll_offset, int max_rows) {
  int visible = 0;

  for (int i = scroll_offset; i < top_n && visible < max_rows; i++) {
    if (top[i].score > 0) {
      int display_row = rows - 3 - visible;
      printf("\x1b[%d;1H\x1b[2K", display_row);
      printf_cols(cols, " %s ", top[i].name);
      visible++;
    }
  }
}

void highlightSelected(Match *top, int selected, int scroll_offset, TermState *term, int max_rows) {
  int visible_idx = selected - scroll_offset;
  if (visible_idx < 0 || visible_idx >= max_rows)
    return;
  int visible_row = term->rows - 3 - visible_idx;
  printf("\x1b[%d;1H\x1b[2K", visible_row);
  if (top[selected].score > 0)
    printf_cols(term->cols, "\x1b[48;2;100;100;100m %s ", top[selected].name);
  else if (top[selected].score == 0)
    printf_cols(term->cols, "\x1b[38;2;100;100;100m %s ", "no result");
  printf("\x1b[0m");
}

void printQuery(UIState *ui, TermState *term) {
  char *query = ui->query;
  int queryLen = ui->query_len;
  int termRows = term->rows;
  int termCols = term->cols;
  if (query[0] == 0) {
    printf_cols(term->cols, "\x1b[%d;%dH\x1b[2K│ search...", termRows - 1, 1);
  } else {

    printf("\x1b[%d;%dH\x1b[2K│ %.*s", termRows - 1, 1, termCols - 4,
           queryLen > termCols - 5 ? query + queryLen - termCols + 4 : query);
  }
  printf("\x1b[%d;%dH│", termRows - 1, termCols);
  printf("\x1b[%d;%dH", termRows - 1,
         queryLen > termCols - 4 ? termCols - 1 : queryLen + 3);

  ui->ui_changed = 1;
}
