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
 * custom structs
 *
 */

struct applist {
  char **nameList;
  char *nameSrc;
  char **execCmdList;
  char *execSrc;
};
typedef struct applist *AppList;

typedef struct {
  char *name;
  char *exec;
  int score;
} Match;

enum { KEY_UP = 1000, KEY_DOWN, KEY_RIGHT, KEY_LEFT };

/*
 *
 * system functions
 *
 */

// sents ESC-sequences to stdout to activate alternative screen
void actAltScr() { write(STDOUT_FILENO, "\x1b[?1049h\x1b[2J\x1b[H", 14); }

void deactAltScr() { write(STDOUT_FILENO, "\x1b[?1049l", 8); }

void deactRaw() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

void actRaw() {
  struct termios raw;

  // get state of terminal and save it in 'orig'
  tcgetattr(STDIN_FILENO, &orig);
  atexit(deactRaw); // define function to be called on exit

  raw = orig;

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

  if (read(0, &c, 1) != 1)
    return -1;

  if (c == '\x1b') {
    char seq[2];

    if (read(0, &seq[0], 1) != 1)
      return '\x1b';
    if (read(0, &seq[1], 1) != 1)
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
      }
    }

    return '\x1b';
  }

  return c;
}

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

int isSubsequence(const char *q, const char *s) {
  while (*q && *s) {
    if (tolower((unsigned char)*q) == tolower((unsigned char)*s))
      q++;
    s++;
  }
  return *q == '\0';
}

int startsWith(const char *s, const char *prefix) {
  while (*prefix && *s) {
    if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix))
      return 0;
    s++;
    prefix++;
  }
  return *prefix == '\0';
}

int contains(const char *s, const char *p) {
  size_t plen = strlen(p);
  if (plen == 0)
    return 1;

  for (size_t i = 0; s[i]; i++) {
    size_t j = 0;
    while (s[i + j] && p[j] &&
           tolower((unsigned char)s[i + j]) == tolower((unsigned char)p[j]))
      j++;
    if (j == plen)
      return 1;
  }
  return 0;
}

int fuzzyScore(const char *query, const char *name, const char *path) {
  int score = 0;

  if (strcasecmp(query, name) == 0)
    score += 1000;
  else if (startsWith(name, query))
    score += 300;
  else if (contains(name, query))
    score += 180;
  else if (isSubsequence(query, name))
    score += 100;

  if (startsWith(path, query))
    score += 40;
  else if (contains(path, query))
    score += 20;
  else if (isSubsequence(query, path))
    score += 10;

  int len_diff = (int)strlen(name) - (int)strlen(query);
  if (len_diff < 0)
    len_diff = -len_diff;
  score -= len_diff;

  return score;
}

long getFileSize(char *dirPath, char *appName) {
  char path[512]; // string to store the path of .desktop file
  snprintf(path, sizeof(path), "%s/%s", dirPath, appName);
  struct stat st;
  if (stat(path, &st) == 0) {
    return st.st_size;
  }
  return 0;
}

/*
 *
 * ui components
 *
 */

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
int already_in_top(Match *top, int top_n, char *name) {
  for (int i = 0; i < top_n; i++) {
    if (top[i].name == name)
      return 1;
  }
  return 0;
}

void tryInsertTop(Match *top, int top_n, char *name, char *exec, int score) {
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
void sortTop(Match *top, int top_n) {
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

FILE *openDataFile(char *dataPath, char *fileName, char *option) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", dataPath, fileName);

  return fopen(path, option);
}

void stripDesktopCodes(char *s) {
  char *src = s, *dst = s;
  while (*src) {
    if (src[0] == '%' && src[1] != '\0') {
      if (src[1] == '%') {
        *dst++ = '%';
      }
      src += 2;
      continue;
    }
    *dst++ = *src++;
  }
  *dst = '\0';
}

void writeToFile(FILE *appFile, FILE *execFile, char *appName, char *execCmd) {
  stripDesktopCodes(execCmd);
  fprintf(appFile, "%s\n", appName);
  fprintf(execFile, "%s\n", execCmd);
}

// function that scans through applications path and finds all apps that have a
// gui. returns amount of apps
int writeAppDataFile(char *dataPath) {
  char *path = "/usr/share/applications/";
  DIR *dir;
  struct dirent *ent;
  int amount = 0;

  char appName[256]; // string to store name value
  char execCmd[256]; // string to store execution command of .desktop file
  char *token = strtok(path, ":");
  FILE *appFile = openDataFile(dataPath, "app.dat", "w");
  FILE *execFile = openDataFile(dataPath, "exec.dat", "w");
  while (token != 0) {
    if ((dir = opendir(token)) != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        if (getGUIApps(token, ent->d_name, appName, execCmd)) {
          writeToFile(appFile, execFile, appName, execCmd);
          amount++;
        }
      }
    }
    token = strtok(NULL, ":");
  }
  fclose(appFile);
  fclose(execFile);
  return amount;
}

void writeAppList(char *dataPath, AppList appList, int *appAmount) {

  FILE *appFile = openDataFile(dataPath, "app.dat", "r");
  long appFileSize = getFileSize(dataPath, "app.dat");
  FILE *execFile = openDataFile(dataPath, "exec.dat", "r");
  long execFileSize = getFileSize(dataPath, "exec.dat");

  char *appLine = NULL;
  size_t appSize = 0;
  char *execLine = NULL;
  size_t execSize = 0;

  char *names = malloc(sizeof(char) * (size_t)(appFileSize + 1));
  char **nameList = malloc(sizeof(char *) * (size_t)*appAmount);
  char *execs = malloc(sizeof(char) * (size_t)(execFileSize + 1));
  char **execList = malloc(sizeof(char *) * (size_t)*appAmount);
  if (!nameList || !execList)
    return;

  appList->nameList = nameList;
  appList->nameSrc = names;
  appList->execCmdList = execList;
  appList->execSrc = execs;

  char *a = names;
  char *e = execs;

  for (int i = 0; getline(&appLine, &appSize, appFile) != -1 &&
                  getline(&execLine, &execSize, execFile) != -1;
       i++) {
    nameList[i] = a;
    execList[i] = e;
    a += sprintf(a, "%s", appLine);
    *(a - 1) = '\0';
    e += sprintf(e, "%s", execLine);
    *(e - 1) = '\0';
  }
}

void freeStorage(AppList appList) {
  free(appList->nameList);
  free(appList->nameSrc);
  free(appList->execCmdList);
  free(appList->execSrc);
}

Match *search(Match *top, char *query, AppList appList, int appAmount,
              char *path, int *top_n) {
  clearResUi();
  if (!top)
    return 0;

  for (int i = 0; i < appAmount; i++) {
    top[i].name = NULL;
    top[i].exec = NULL;
    top[i].score = 0;
    int score = fuzzyScore(query, *(appList->nameList + i), path);
    tryInsertTop(top, *top_n, *(appList->nameList + i),
                 *(appList->execCmdList + i), score);
  }

  sortTop(top, *top_n);

  return top;
}

void printResults(Match *top, int top_n) {
  for (int i = 0; i < top_n; i++) {
    if (top[i].score > 0) {
      printf("\x1b[%d;0H %s", termRows - 3 - i, top[i].name);
    }
  }
}

void launchApp(Match *top, int selected) {
  pid_t pid = fork();
  if (pid == 0) {
    deactAltScr();
    deactRaw();

    int devnull = open("/dev/null", O_WRONLY);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    close(devnull);

    execl("/bin/sh", "sh", "-c", top[selected].exec, NULL);
    _exit(1);
  }
}

void onStartUp(int *appAmount, AppList appList) {
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

  *appAmount = writeAppDataFile(dataPath);
  writeAppList(dataPath, appList, appAmount);
}

void highlightSelected(int selected, Match *top) {
  printf("\x1b[48;2;180;180;180m\x1b[%d;2H%s\x1b[0m", termRows - 3 - selected,
         top[selected].score == 0 ? "no result" : top[selected].name);
}

int keyProcessing(int key, char query[], int *queryLen, int *selected,
                  Match *top) {
  if (key == 27) { // ESC
    if (*queryLen > 0) {
      for (; *queryLen >= 0; (*queryLen)--)
        query[*queryLen] = '\0';
      return 1;
    }
    return 0;
  } else if (key == 127 || key == 8) { // backspace
    if (*queryLen > 0)
      query[--*queryLen] = '\0';
  } else if (key == '\r' || key == '\n') {
    launchApp(top, *selected);
    return 0;
    system(top[0].exec);
  } else if (key == KEY_UP) {
    *selected = *selected < termRows - 3 ? *selected + 1 : 0;
  } else if (isprint(key)) {
    if (*queryLen < 512) {
      *queryLen += 1;
      query[*(queryLen)-1] = (char)key;
      query[*(queryLen)] = '\0';
    }
  }

  return 1;
}

int queryChanged(char *query, char *altquery) {
  if (strcmp(query, altquery) != 0) {
    strcpy(altquery, query);
    return 1;
  }
  return 0;
}

void printQuery(char *query, int queryLen) {
  if (query[0] == 0) {
    printf("\x1b[%d;%dH\x1b[2K│ search...", termRows - 1, 1);
  } else {

    printf("\x1b[%d;%dH\x1b[2K│ %.*s", termRows - 1, 1, termCols - 4,
           queryLen > termCols - 5 ? query + queryLen - termCols + 4 : query);
  }
  printf("\x1b[%d;%dH│", termRows - 1, termCols);
  printf("\x1b[%d;%dH", termRows - 1,
         queryLen > termCols - 4 ? termCols - 1 : queryLen + 3);

  ui_change = 1;
}

void app() {
  char *path = "/usr/share/applications/";
  char query[512] = {0};
  char altquery[512] = {0};
  int queryLen = 0;
  int appAmount = 0;
  int selected = 0;
  int oldselc = 0;
  struct applist appList;

  onStartUp(&appAmount, &appList);
  int top_n = appAmount > termRows ? termRows : appAmount;

  Match *top = calloc((size_t)termRows, sizeof(Match));
  search(top, query, &appList, appAmount, path, &top_n);
  printResults(top, top_n);
  highlightSelected(selected, top);

  for (;;) {
    int key = readKey();

    if (!keyProcessing(key, query, &queryLen, &selected, top)) {
      freeStorage(&appList);
      deactAltScr();
      deactRaw();
      break;
    }

    if (selected != oldselc) {
      oldselc = selected;
      printResults(top, top_n);
      highlightSelected(selected, top);
    }

    if (queryChanged(query, altquery)) {
      printQuery(query, queryLen);
      top = search(top, query, &appList, appAmount, path, &top_n);
      printResults(top, top_n);
      highlightSelected(selected, top);
    }

    if (sizeChanged()) {
      basicFrame();
      printQuery(query, queryLen);
    }

    if (ui_change) {
      fflush(stdout);
      ui_change = 0;
    }

    usleep(1000);
  }
}

int main() { app(); }
