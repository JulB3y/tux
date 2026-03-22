#include <stddef.h>

#include "fuzzy.h"
#include "search.h"
#include "types.h"
#include "ui.h"

static void sortTop(Match *top, int top_n) {
  for (int i = 0; i < top_n; i++) {
    for (int j = i + 1; j < top_n; j++) {
      if (top[j].score > top[i].score) {
        Match tmp = top[i];
        top[i] = top[j];
        top[j] = tmp;
      }
    }
  }
}

static void emptyTop(Match *top, int top_n) {
  for (int i = 0; i < top_n; i++) {
    top[i].name = NULL;
    top[i].exec = NULL;
    top[i].score = 0;
  }
}

int search(App *app) {
  clearResUi(app->term.rows);
  if (!app->top)
    return 0;

  emptyTop(app->top, app->app_count);

  int result_count = 0;

  if (app->ui.query[0] == '\0') {
    for (int i = 0; i < app->app_count; i++) {
      app->top[i].name = app->apps.nameList[i];
      app->top[i].exec = app->apps.execCmdList[i];
      app->top[i].score = 1;
      result_count++;
    }
  } else {
    for (int i = 0; i < app->app_count; i++) {
      int score = fuzzyScore(app->ui.query_lower, app->apps.nameLowerList[i],
                             app->ui.query_len, app->apps.nameLenList[i]);

      if (score > 0) {
        app->top[result_count].name = app->apps.nameList[i];
        app->top[result_count].exec = app->apps.execCmdList[i];
        app->top[result_count].score = score;
        result_count++;
      }
    }

    sortTop(app->top, result_count);
  }

  app->top_n = result_count;
  return 1;
}
