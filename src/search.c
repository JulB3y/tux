#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "fuzzy.h"
#include "types.h"
#include "ui.h"
#include "util.h"

static int already_in_top(Match *top, int top_n, char *name) {
  for (int i = 0; i < top_n; i++) {
    if (top[i].name == name)
      return 1;
  }
  return 0;
}

static void tryInsertTop(Match *top, int top_n, char *name, char *exec,
                         int score) {
  if (score <= 0 || top_n == 0)
    return;
  if (already_in_top(top, top_n, name))
    return;

  int worst = 0;
  for (int i = 1; i < top_n; i++) {
    if (top[i].score < top[worst].score)
      worst = i;
  }

  if (score > top[worst].score) {
    top[worst].name = name;
    top[worst].exec = exec;
    top[worst].score = score;
  }
}
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

// void handleQuery(char *query, int query_len) {}

int search(App *app) {
  clearResUi(app->term.rows);
  if (!app->top)
    return 0;

  emptyTop(app->top, app->top_n);

  // handling empty query -> putting first entries of nameList into top
  if (app->ui.query[0] == '\0') {
    for (int i = 0; i < app->top_n; i++) {
      app->top[i].name = app->apps.nameList[i];
      app->top[i].exec = app->apps.execCmdList[i];
      app->top[i].score = 1;
    }
    return 1;
  }

  char queryLower[512];
  toLowerCopy(queryLower, app->ui.query);

  for (int i = 0; i < app->app_count; i++) {
    int score = fuzzyScore(queryLower, app->apps.nameLowerList[i],
                           app->ui.query_len, app->apps.nameLenList[i]);

    tryInsertTop(app->top, app->top_n, app->apps.nameList[i],
                 app->apps.execCmdList[i], score);
  }

  sortTop(app->top, app->top_n);
  return 1;
}
