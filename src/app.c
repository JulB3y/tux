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
void handleWinch(int sig) {
  (void)sig;   // unused parameter
  resized = 1; // flag checked in main event loop
}

void freeStorage(AppList *a) {

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
  basicFrame(app);

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

void app_shutdown(App *app) {
  deactAltScreen();
  deactRaw(app);
}

void app_run(App *app) {

  int *termRows = &app->term.rows;
  app->top_n = app->app_count > *termRows - 3 ? *termRows - 3 : app->app_count;

  app->top = calloc((size_t)(*termRows - 3), sizeof(Match));
  search(app);
  printResults(app);
  highlightSelected(app->top, app->ui.selected, &app->term);
  fflush(stdout);

  for (;;) {
    int ev = waitForInputOrSignal();
    if (ev < 0)
      break;

    if (resized) {
      resized = 0;
      getTermSize(app);
      basicFrame(app);

      int max_rows = *termRows - 3;
      if (max_rows < 0)
        max_rows = 0;
      app->top_n = app->app_count > max_rows ? max_rows : app->app_count;

      Match *tmp = realloc(app->top, (size_t)max_rows * sizeof(Match));
      if (!tmp && max_rows > 0)
        break;
      app->top = tmp;

      search(app);
      printQuery(&app->ui, &app->term);
      printResults(app);
      highlightSelected(app->top, app->ui.selected, &app->term);
      fflush(stdout);
    }

    if (ev == 1) {
      int key = readKey(app); // darf jetzt blockierend / halbblockierend sein
      if (!keyProcessing(key, &app->ui, app->top, app->top_n, &app->term)) {
        break;
      }

      if (app->ui.selected != app->ui.old_selected) {
        app->ui.old_selected = app->ui.selected;
        printResults(app);
        highlightSelected(app->top, app->ui.selected, &app->term);
      }

      if (queryChanged(app->ui.query, app->ui.prev_query)) {
        printQuery(&app->ui, &app->term);
        app->ui.selected = 0;
        app->ui.old_selected = 0;
        search(app);
        printResults(app);
        highlightSelected(app->top, app->ui.selected, &app->term);
      }

      if (app->ui.ui_changed) {
        fflush(stdout);
        app->ui.ui_changed = 0;
      }
    }
  }

  free(app->top);
  freeStorage(&app->apps);
  deactAltScreen();
  deactRaw(app);
}
