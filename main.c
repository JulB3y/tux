// main.c
//    basic c app launcher with fzf

// usefull info
//      activate   alternate screen with "\x1b[?1049h"
//      deactivate alternate screen with "\x1b[?1049l"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

/*
 *
 * Global Vars
 *
 */

static struct termios orig;

// State vars
int ui_change = 1;

// size vars
int termRows = 0;
int termCols = 0;

/*
 *
 * system functions
 *
 */

void actAltScr() {
  printf("\x1b[?1049h"); // activate alternate screen
  printf("\x1b[2J");
  printf("\x1b[H");
  fflush(stdout);
}

void deactAltScr() { printf("\x1b[?1049l"); }

void deactRaw() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

void actRaw() {
  struct termios raw;

  // get state of terminal and save it in 'orig'
  tcgetattr(STDIN_FILENO, &orig);
  atexit(deactRaw); // define function to be called on exit

  raw = orig;

  raw.c_lflag &=
      ~(ICANON | ECHO | ISIG); // disabling ICANON, ECHO and ISIG (linebuffer,
                               // auto-out, ctrl-c and co.)
  raw.c_iflag &= ~(IXON |      // diabling ctrl-s/q
                   ICRNL);     //  enabling normal return for return key

  raw.c_oflag &= ~(OPOST); // no auto output

  raw.c_cc[VMIN] = 1; // 1 byte to compute input
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void uiPrint(char *str, ...) {
  ui_change = 1;

  va_list ap;
  va_start(ap, str);
  vprintf(str, ap);
  va_end(ap);
}

// get window size
int getTermSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    return 0;
  if (rows)
    *rows = ws.ws_row;
  if (cols)
    *cols = ws.ws_col;
  return 1;
}

int readKey() {
  char c;
  read(0, &c, 1);
  return c;
}

/*
 *
 * ui components
 *
 */

int sizeChanged() {
  int cols, rows;
  if (!getTermSize(&rows, &cols))
    return 0;
  if (termCols == cols && termRows == rows)
    return 0;
  termCols = cols;
  termRows = rows;
  return 1;
}

void clearResUi() {
  for (int i = termRows - 3; i > 0; i--)
    printf("\x1b[%d;1H\x1b[2K", i);
}

void basicFrame() {
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
  clearResUi();
  ui_change = 1;
}

int getGUIApps(char *d, char *n, char *appName, char *execCmd) {
  char path[512]; // string to store the path of .desktop file
  snprintf(path, sizeof(path), "%s/%s", d, n); // append filename to global path
  FILE *f = fopen(path, "r");                  // read file in path
  if (!f)
    return 0;       // error handling if file not readable
  char line[512];   // string for each line of the file
  int in_entry = 0; // bool to check if under [Desktop Entry]
  int isTerm = 1;
  // reads each line of file
  while (fgets(line, sizeof(line), f)) {
    // checks if in right sub sector and logs it in in_entry
    if (line[0] == '[') {
      in_entry = strstr(line, "Desktop Entry") != NULL;
      continue;
    }
    // skips rest of code if not in correct sub sector
    if (!in_entry)
      continue;

    char key[20];    // string to store key
    char value[256]; // string to store value
    // for each line -> gets key and value and stores name and exec values in
    // vars
    if (sscanf(line, "%19[^=]=%255[^\n]", key, value) == 2) {
      if (strcmp(key, "Name") == 0)
        strcpy(appName, value);
      if (strcmp(key, "Terminal") == 0)
        if (!(strcmp(value, "true") == 0))
          isTerm = 0;
      if (strcmp(key, "Exec") == 0)
        strcpy(execCmd, value);
    }
  }
  fclose(f);
  return !isTerm;
}

FILE *openDataFile(char *dataPath, char *fileName) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", dataPath, fileName);

  return fopen(path, "w");
}

void writeToFile(FILE *file, char *appName, char *execCmd) {
  fprintf(file, "%s|%s\n", appName, execCmd);
}

void writeAppDataFile(char *dataPath) {
  char *path = "/usr/share/applications/";
  DIR *dir;
  struct dirent *ent;

  char appName[256]; // string to store name value
  char execCmd[256]; // string to store execution command of .desktop file
  char *token = strtok(path, ":");
  FILE *file = openDataFile(dataPath, "app.dat");
  while (token != 0) {
    if ((dir = opendir(token)) != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        if (getGUIApps(token, ent->d_name, appName, execCmd))
          writeToFile(file, appName, execCmd);
      }
    }
    token = strtok(NULL, ":");
  }
  fclose(file);
}

void search() {}

void onStartUp() {
  actRaw();
  actAltScr();

  // Getting home and data path
  char *homePath = getenv("HOME");
  char dataPath[512];
  snprintf(dataPath, sizeof(dataPath), "%s/.local/share/tui-launcher/",
           homePath);

  getTermSize(&termRows, &termCols);

  basicFrame();
  // deactivate blocking behavior of read()
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  // creating path
  mkdir(dataPath, 0755);

  writeAppDataFile(dataPath);
}

int keyProcessing(int key, char query[], int *queryLen) {
  if (key == 27) { // ESC
    deactAltScr();
    deactRaw();

    return 0;
  } else if (key == 127 || key == 8) { // backspace
    if (*queryLen > 0)
      query[--*queryLen] = '\0';
  } else if (key == '\r' || key == '\n') {
    // TODO: exec app
  } else if (isprint(key)) {
    if (*queryLen < 512) {
      *queryLen += 1;
      query[*(queryLen)-1] = (char)key;
      query[*(queryLen)] = '\0';
    }
  }

  return 1;
}

void app() {
  onStartUp();

  char query[512] = {0};
  char altquery[512] = {0};
  int queryLen = 0;

  for (;;) {
    int key = readKey();

    if (!keyProcessing(key, query, &queryLen)) {
      break;
    }

    if (strcmp(query, altquery) != 0) {
      strcpy(altquery, query);
      if (query[0] == 0) {
        printf("\x1b[%d;%dH\x1b[2K│ search...", termRows - 1, 1);
      } else {
        printf("\x1b[%d;%dH\x1b[2K│ %s", termRows - 1, 1, query);
      }
      ui_change = 1;
    }

    if (sizeChanged()) {
      basicFrame();
    }

    if (ui_change) {
      fflush(stdout);
      ui_change = 0;
    }
  }
}

int main() { app(); }
