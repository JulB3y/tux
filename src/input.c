#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include "exec.h"
#include "input.h"
#include "types.h"

enum {
  KEY_UP = 1000,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT
}; // map arrow key ESC-sequences to ints

int readKey(void) {
  char c;

  // reads key + error handling
  if (read(STDIN_FILENO, &c, 1) != 1)
    return -1;

  // handler for ESC-sequences
  if (c == '\x1b') {
    // determine if esc is part of multi-byte sequence
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

static void handleArrowKeyEvents(int key, UIState *ui, int top_n, int max_rows) {
  if (key == KEY_UP) {
    if (ui->selected < top_n - 1) {
      ui->selected++;
      if (ui->selected - ui->scroll_offset >= max_rows) {
        ui->scroll_offset++;
      }
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
      return 1; // input da
    if (r == 0)
      continue; // bei -1 eigentlich unmöglich

    if (errno == EINTR)
      return 0; // signal, z.B. SIGWINCH
    return -1;  // echter fehler
  }
}

int keyProcessing(App *app, int key) {
  UIState *ui = &app->ui;
  Match *top = app->top;
  TermState *term = &app->term;
  int max_rows = term->rows - 3;
  if (max_rows < 0)
    max_rows = 0;

  if (key == 27) { // ESC
    if (ui->query_len > 0) {
      ui->query[0] = '\0';
      ui->query_lower[0] = '\0';
      ui->query_len = 0;
      ui->query_changed = 1;
      return 1;
    }
    return 0;
  } else if (key == 127 || key == 8) { // backspace
    if (ui->query_len > 0) {
      ui->query_len--;
      ui->query[ui->query_len] = '\0';
      ui->query_lower[ui->query_len] = '\0';
      ui->query_changed = 1;
    }
  } else if (key == '\r' || key == '\n') {
    launchApp(top[ui->selected].exec);
    return 0;
  } else if (key >= KEY_UP && key <= KEY_RIGHT) {
    handleArrowKeyEvents(key, ui, app->top_n, max_rows);
  } else if (isprint(key)) {
    if (ui->query_len < 511) {
      ui->query[ui->query_len] = (char)key;
      ui->query[ui->query_len + 1] = '\0';
      ui->query_lower[ui->query_len] = (char)tolower((unsigned char)key);
      ui->query_lower[ui->query_len + 1] = '\0';
      ui->query_len++;
      ui->query_changed = 1;
    }
  }

  return 1;
}
