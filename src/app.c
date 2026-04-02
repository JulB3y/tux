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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "app.h"
#include "cache.h"
#include "config.h"
#include "file.h"
#include "input.h"
#include "module.h"
#include "query.h"
#include "term.h"
#include "types.h"
#include "ui.h"
#include "string.h"

static volatile sig_atomic_t resized = 0;

static void handleWinch(int sig) {
  (void)sig;
  resized = 1;
}

static void freeStorage(AppList *a) {
  if (a->src_is_mmaped)
    munmap(a->src, (size_t)a->src_size + 1);
  else
    free(a->src);

  free(a->pathList);
  free(a->mtimeList);
  free(a->nameList);
  free(a->execCmdList);

  free(a->nameLowerList);
  free(a->nameLowerSrc);

  free(a->nameLenList);

  if (a->keywords) {
    for (int i = 0; i < a->count; i++) {
      if (a->keywords[i].keywords) {
        for (int j = 0; j < a->keywords[i].keyword_count; j++) {
          free(a->keywords[i].keywords[j]);
        }
        free(a->keywords[i].keywords);
      }
    }
    free(a->keywords);
  }
}

App *app_init() {
  App *app = malloc(sizeof(App));
  if (!app)
    return NULL;

  memset(app, 0, sizeof(App));

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
  app->config = config_init();

  if (app->config && appList->count > 0) {
    appList->keywords = calloc((size_t)appList->count, sizeof(AppKeywords));

    int has_keywords = 0;
    for (int i = 0; i < appList->count; i++) {
      int keyword_count = 0;
      char **keywords = config_get_keywords(app->config, appList->nameList[i], &keyword_count);

      if (keywords && keyword_count > 0) {
        has_keywords = 1;
        appList->keywords[i].keyword_count = keyword_count;
        appList->keywords[i].keywords = malloc((size_t)keyword_count * sizeof(char *));

        for (int j = 0; j < keyword_count; j++) {
          appList->keywords[i].keywords[j] = strdup(keywords[j]);
        }
      }
    }

    if (!has_keywords) {
      free(appList->keywords);
      appList->keywords = NULL;
    }
  }

  modules_init(app);

  return app;
}

static void app_shutdown(App *app) {
  modules_shutdown();
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
  app->ui.calc_result[0] = '\0';
  app->ui.cursor_pos = 0;
  strcpy(app->ui.mode, "apps");
  app->search_limit = max_rows * 2;
  if (app->search_limit < 10)
    app->search_limit = 10;

  app->top = malloc((size_t)app->search_limit * sizeof(Match));
  executeQuery(app, parseQuery(app->ui.query));
  printQuery(&app->ui, &app->term);
  printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
  if (app->top_n > 0) {
    highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
  }
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

      executeQuery(app, parseQuery(app->ui.query));
      printQuery(&app->ui, &app->term);
      printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
      if (app->top_n > 0) {
        highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
      }
      fflush(stdout);
    }

    if (ev == 1) {
      int key = readKey();
      if (!keyProcessing(app, key)) {
        break;
      }

      if (app->ui.selected != app->ui.old_selected) {
        app->ui.old_selected = app->ui.selected;
        printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
        if (app->top_n > 0) {
          highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
        }
        fflush(stdout);
      }

      if (app->ui.query_changed) {
        app->ui.query_changed = 0;
        app->ui.selected = 0;
        app->ui.old_selected = 0;
        app->ui.scroll_offset = 0;
        app->ui.calc_result[0] = '\0';
        app->search_limit = max_rows * 2;
        if (app->search_limit < 10)
          app->search_limit = 10;

        executeQuery(app, parseQuery(app->ui.query));
        printQuery(&app->ui, &app->term);
        printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
        if (app->top_n > 0) {
          highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
        }
        fflush(stdout);
      }

      if (app->ui.ui_changed) {
        printResults(*termRows, app->term.cols, app->top, app->top_n, app->ui.scroll_offset, max_rows);
        if (app->top_n > 0) {
          highlightSelected(app->top, app->ui.selected, app->ui.scroll_offset, &app->term, max_rows);
        }
        printQuery(&app->ui, &app->term);
        fflush(stdout);
        app->ui.ui_changed = 0;
      }
    }
  }

  free(app->top);
  freeStorage(&app->apps);
  config_free(app->config);
  app_shutdown(app);
}
