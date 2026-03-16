#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "types.h"

void actAltScreen(void) {
  write(STDOUT_FILENO, "\x1b[?1049h\x1b[2J\x1b[H", 14);
}

void deactAltScreen(void) { write(STDOUT_FILENO, "\x1b[?1049l", 8); }

void deactRaw(App *app) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &app->term.orig_termios);
}

// Switches the terminal into raw mode for immediate, unprocessed keyboard
// input. Saves current terminal settings in `orig` so they can be restored
// later. input is read byte-by-byte without line buffering or echo, and common
// terminal-generated signals and translations are disabled.
void actRaw(App *app) {
  struct termios raw;

  // get state of terminal and save it in 'orig'
  tcgetattr(STDIN_FILENO, &app->term.orig_termios);

  raw = app->term.orig_termios;

  raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO |
                              ISIG);  // disabling ICANON, ECHO and ISIG
                                      // (linebuffer, auto-out, ctrl-c and co.)
  raw.c_iflag &= (tcflag_t) ~(IXON |  // diabling ctrl-s/q
                              ICRNL); //  enabling normal return for return key

  raw.c_oflag &= (tcflag_t) ~(OPOST); // no auto output

  raw.c_cc[VMIN] = 1; // 1 byte to compute input
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// get window size
int getTermSize(App *app) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    return 0;
  app->term.rows = ws.ws_row;
  app->term.cols = ws.ws_col;
  return 1;
}
