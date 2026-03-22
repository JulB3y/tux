#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "app.h"
#include "cache.h"
#include "file.h"
#include "input.h"
#include "search.h"
#include "term.h"
#include "types.h"
#include "ui.h"

static volatile sig_atomic_t resized = 0;
// signal handler for SIGWINCH (window size change)
// marks terminal as resized
static void handleWinch(int sig) {
  (void)sig;   // unused parameter
  resized = 1; // flag checked in main event loop
}

static void freeStorage(AppList *a) {

  free(a->src);

  free(a->pathList);
  free(a->mtimeList);
  free(a->nameList);
  free(a->execCmdList);

  free(a->nameLowerList);
  free(a->nameLowerSrc);

  free(a->nameLenList);
}

App *app_init() {
  App *app = malloc(sizeof(App));

  actRaw(app);
  actAltScreen();

  AppList *appList = &app->apps;

  char *homePath = getenv("HOME");
  char dataDir[500];
  char cachePath[512];
  char metaPath[512];

  snprintf(dataDir, sizeof(dataDir), "%s/.local/share/tux-launcher", homePath);
  mkdir(dataDir, 0755);

  snprintf(cachePath, sizeof(cachePath), "%s/cache.dat", dataDir);
  snprintf(metaPath, sizeof(metaPath), "%s/cache.meta", dataDir);

  getTermSize(app);
  basicFrame(&app->ui.ui_changed, &app->term);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags);

  const char *appsDir = "/usr/share/applications/";

  long currentDirMTime = getDirMTime(appsDir);
  long cachedDirMTime = readMetaFile(metaPath);

  if (cachedDirMTime == currentDirMTime) {
    int loaded = loadCache(cachePath, appList);

    if (!loaded || !validateCache(appList)) {
      if (loaded) {
        freeStorage(appList);
        memset(appList, 0, sizeof(*appList));
      }

      writeAppDataFile(cachePath);
      writeMetaFile(metaPath, currentDirMTime);

      if (!loadCache(cachePath, appList))
        exit(1);
    }
  } else {
    writeAppDataFile(cachePath);
    writeMetaFile(metaPath, currentDirMTime);

    if (!loadCache(cachePath, appList))
      exit(1);
  }

  struct sigaction sa = {0};
  sa.sa_handler = handleWinch;
  sigaction(SIGWINCH, &sa, NULL);

  app->app_count = appList->count;

  return app;
}

static void app_shutdown(App *app) {
  deactAltScreen();
  deactRaw(app);
}

void app_run(App *app) {

  int *termRows = &app->term.rows;
  int max_rows = *termRows - 3;
  if (max_rows < 0)
    max_rows = 0;

  app->top_n = app->app_count;
  app->ui.selected = 0;
  app->ui.scroll_offset = 0;

  app->top = calloc((size_t)app->app_count, sizeof(Match));
  search(app);
  printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
  highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
  fflush(stdout);

  for (;;) {
    int ev = waitForInputOrSignal();
    if (ev < 0)
      break;

    if (resized) {
      resized = 0;
      getTermSize(app);
      basicFrame(&app->ui.ui_changed, &app->term);

      max_rows = *termRows - 3;
      if (max_rows < 0)
        max_rows = 0;

      search(app);
      printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
      highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
      printQuery(&app->ui, &app->term);
      fflush(stdout);
    }

    if (ev == 1) {
      int key = readKey(app); // darf jetzt blockierend / halbblockierend sein
      if (!keyProcessing(app, key)) {
        break;
      }

      if (app->ui.selected != app->ui.old_selected) {
        app->ui.old_selected = app->ui.selected;
        printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
        highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
      }

      if (app->ui.query_changed) {
        app->ui.query_changed = 0;
        app->ui.selected = 0;
        app->ui.old_selected = 0;
        app->ui.scroll_offset = 0;

        search(app);
        printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
        highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
        printQuery(&app->ui, &app->term);
      }

      if (app->ui.ui_changed) {
        fflush(stdout);
        app->ui.ui_changed = 0;
      }
    }
  }

  free(app->top);
  freeStorage(&app->apps);
  app_shutdown(app);
}
