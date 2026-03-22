#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include "exec.h"
#include "input.h"
#include "types.h"
#include "ui.h"

enum {
  KEY_UP = 1000,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT
}; // map arrow key ESC-sequences to ints

int readKey(App *app) {
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
        app->ui.ui_changed = 1;
        return KEY_UP;
      case 'B':
        app->ui.ui_changed = 1;
        return KEY_DOWN;
      case 'C':
        app->ui.ui_changed = 1;
        return KEY_RIGHT;
      case 'D':
        app->ui.ui_changed = 1;
        return KEY_LEFT;
      default:
        break;
      }
    }

    return '\x1b';
  }

  return c;
}

static void handleArrowKeyEvents(int key, UIState *ui, Match *top) {
  if (key == KEY_UP) {
    int next = ui->selected + 1;
    if (top[next].score != 0) {
      ui->selected = next;
    }
  } else if (key == KEY_DOWN) {
    if (ui->selected > 0) {
      ui->selected--;
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

int keyProcessing(int key, UIState *ui, Match *top, TermState *term) {
  if (key == 27) { // ESC
    if (ui->query_len > 0) {
      ui->query[0] = '\0';
      ui->query_len = 0;
      ui->query_changed = 1;
      return 1;
    }
    return 0;
  } else if (key == 127 || key == 8) { // backspace
    if (ui->query_len > 0) {
      ui->query[--ui->query_len] = '\0';
      ui->query_changed = 1;
    }
  } else if (key == '\r' || key == '\n') {
    launchApp(top[ui->selected].exec);
    return 0;
  } else if (key >= KEY_UP && key <= KEY_RIGHT) {
    handleArrowKeyEvents(key, ui, top);
    highlightSelected(top, ui->selected, term);
  } else if (isprint(key)) {
    if (ui->query_len < 512) {
      ui->query_len += 1;
      ui->query[ui->query_len - 1] = (char)key;
      ui->query[ui->query_len] = '\0';
      ui->query_changed = 1;
    }
  }

  return 1;
}
