#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "exec.h"
#include "input.h"
#include "query.h"
#include "types.h"

enum {
  KEY_UP = 1000,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT
};

int readKey(void) {
  char c;

  if (read(STDIN_FILENO, &c, 1) != 1)
    return -1;

  if (c == '\x1b') {
    struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN};

    int r = poll(&pfd, 1, 25);
    if (r <= 0)
      return '\x1b';

    char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';

    r = poll(&pfd, 1, 25);
    if (r <= 0)
      return '\x1b';

    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[') {
      switch (seq[1]) {
      case 'A':
        return KEY_UP;
      case 'B':
        return KEY_DOWN;
      case 'C':
        return KEY_RIGHT;
      case 'D':
        return KEY_LEFT;
      default:
        break;
      }
    }

    return '\x1b';
  }

  return c;
}

static void handleArrowKeyEvents(int key, UIState *ui, int top_n, int max_rows, int *needs_lazy_load) {
  if (key == KEY_UP) {
    if (ui->selected < top_n - 1) {
      ui->selected++;
      if (ui->selected - ui->scroll_offset >= max_rows) {
        ui->scroll_offset++;
      }
      if (ui->selected >= top_n - 3) {
        *needs_lazy_load = 1;
      }
    } else if (ui->selected == top_n - 1) {
      *needs_lazy_load = 1;
    }
  } else if (key == KEY_DOWN) {
    if (ui->selected > 0) {
      ui->selected--;
      if (ui->selected < ui->scroll_offset) {
        ui->scroll_offset--;
      }
    }
  }
}

int waitForInputOrSignal(void) {
  struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN};

  for (;;) {
    int r = poll(&pfd, 1, -1);

    if (r > 0)
      return 1;
    if (r == 0)
      continue;

    if (errno == EINTR)
      return 0;
    return -1;
  }
}

int keyProcessing(App *app, int key) {
  UIState *ui = &app->ui;
  Match *top = app->top;
  TermState *term = &app->term;
  int max_rows = term->rows - 3;
  if (max_rows < 0)
    max_rows = 0;

  if (key == 27) {
    if (ui->query_len > 0) {
      ui->query[0] = '\0';
      ui->query_lower[0] = '\0';
      ui->query_len = 0;
      ui->cursor_pos = 0;
      ui->query_changed = 1;
      return 1;
    }
    return 0;
  } else if (key == 127 || key == 8) {
    if (ui->cursor_pos > 0) {
      for (int i = ui->cursor_pos - 1; i < ui->query_len; i++) {
        ui->query[i] = ui->query[i + 1];
        ui->query_lower[i] = ui->query_lower[i + 1];
      }
      ui->query_len--;
      ui->cursor_pos--;
      ui->query_changed = 1;
    }
  } else if (key == '\r' || key == '\n') {
    if (strcmp(ui->mode, "calc") == 0 && ui->calc_result[0] != '\0') {
      copyToClipboard(ui->calc_result);
      return 0;
    }
    launchApp(top[ui->selected].exec);
    return 0;
  } else if (key == KEY_LEFT) {
    if (ui->cursor_pos > 0) {
      ui->cursor_pos--;
      ui->ui_changed = 1;
    }
  } else if (key == KEY_RIGHT) {
    if (ui->cursor_pos < ui->query_len) {
      ui->cursor_pos++;
      ui->ui_changed = 1;
    }
  } else if (key == KEY_UP || key == KEY_DOWN) {
    int needs_lazy_load = 0;
    handleArrowKeyEvents(key, ui, app->top_n, max_rows, &needs_lazy_load);

    if (needs_lazy_load && app->has_more_results) {
      app->search_limit *= 2;
      if (app->search_limit > app->app_count)
        app->search_limit = app->app_count;

      app->top = realloc(app->top, (size_t)app->search_limit * sizeof(Match));

      int old_selected = app->ui.selected;

      executeQuery(app, parseQuery(app->ui.query));

      app->ui.selected = old_selected;
      app->ui.old_selected = old_selected;

      ui->ui_changed = 1;
    }
  } else if (isprint(key)) {
    if (ui->query_len < 127) {
      for (int i = ui->query_len; i > ui->cursor_pos; i--) {
        ui->query[i] = ui->query[i - 1];
        ui->query_lower[i] = ui->query_lower[i - 1];
      }
      ui->query[ui->cursor_pos] = (char)key;
      ui->query_lower[ui->cursor_pos] = (char)tolower((unsigned char)key);
      ui->query_len++;
      ui->cursor_pos++;
      ui->query[ui->query_len] = '\0';
      ui->query_lower[ui->query_len] = '\0';
      ui->query_changed = 1;
    }
  }

  return 1;
}
