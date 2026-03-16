#include <stdio.h>

#include "types.h"

void clearResUi(App *app) {
  for (int i = app->term.rows - 3; i > 0; i--)
    printf("\x1b[%d;1H\x1b[2K", i);
}

void basicFrame(App *app) {
  int termRows = app->term.rows;
  int termCols = app->term.cols;
  printf("\x1b[%d;1H╭", termRows - 2);
  for (int i = 0; i < termCols - 2; i++)
    printf("─");
  printf("╮");
  printf("\x1b[%d;%dH\x1b[2K│ search...", termRows - 1, 1);
  printf("\x1b[%d;%dH│", termRows - 1, termCols);
  printf("\x1b[%d;1H╰", termRows);
  for (int i = 2; i < termCols; i++)
    printf("─");
  printf("╯");
  clearResUi(app);
  app->ui.ui_changed = 1;
}

void printResults(App *app) {
  for (int i = 0; i < app->top_n; i++) {
    if (app->top[i].score > 0) {
      printf("\x1b[%d;0H %s ", app->term.rows - 3 - i, app->top[i].name);
    }
  }
}

void printQuery(UIState *ui, TermState *term) {
  char *query = ui->query;
  int queryLen = ui->query_len;
  int termRows = term->rows;
  int termCols = term->cols;
  if (query[0] == 0) {
    printf("\x1b[%d;%dH\x1b[2K│ search...", termRows - 1, 1);
  } else {

    printf("\x1b[%d;%dH\x1b[2K│ %.*s", termRows - 1, 1, termCols - 4,
           queryLen > termCols - 5 ? query + queryLen - termCols + 4 : query);
  }
  printf("\x1b[%d;%dH│", termRows - 1, termCols);
  printf("\x1b[%d;%dH", termRows - 1,
         queryLen > termCols - 4 ? termCols - 1 : queryLen + 3);

  ui->ui_changed = 1;
}

void highlightSelected(Match *top, int selected, TermState *term) {
  printf("\x1b[%d;1H", term->rows - 3 - selected);
  if (top[0].score != 0)
    printf("\x1b[48;2;100;100;100m %s \x1b[0m", top[selected].name);
  else if (top[0].score == 0)
    printf("\033[38;2;100;100;100m %s \x1b[0m", "no result");
}
