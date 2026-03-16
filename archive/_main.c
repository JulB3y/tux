// tux - main.c
//    basic c app launcher with fzf

// usefull info
//      activate   alternate screen with "\x1b[?1049h"
//      deactivate alternate screen with "\x1b[?1049l"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
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

volatile sig_atomic_t resized = 0;

/*
 *
 * custom structs
 *
 */

struct applist {
  char *src;

  char **pathList;
  long *mtimeList;
  char **nameList;
  char **execCmdList;

  char **nameLowerList;
  char *nameLowerSrc;

  int *nameLenList;
  int count;
};
typedef struct applist *AppList;

typedef struct {
  char *name;
  char *exec;
  int score;
} Match;

enum {
  KEY_UP = 1000,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT
}; // map arrow key ESC-sequences to ints

/*
 *
 * system functions
 *
 */

// sents ESC-sequences to stdout to activate alternative screen
void actAltScr() { write(STDOUT_FILENO, "\x1b[?1049h\x1b[2J\x1b[H", 14); }

// sents ESC-sequence to stdout to deactivate alt-screen
void deactAltScr() { write(STDOUT_FILENO, "\x1b[?1049l", 8); }

// deactivates raw mode
void deactRaw() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig); }

// Switches the terminal into raw mode for immediate, unprocessed keyboard
// input. Saves current terminal settings in `orig` so they can be restored
// later. input is read byte-by-byte without line buffering or echo, and common
// terminal-generated signals and translations are disabled.
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

// signal handler for SIGWINCH (window size change)
// marks terminal as resized
void handleWinch(int sig) {
  (void)sig;   // unused parameter
  resized = 1; // flag checked in main event loop
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

// reads single key from stdin
// handles regular characters and ANSI ESC-sequences (e.g. arrow keys)
int readKey() {
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
        ui_change = 1;
        return KEY_UP;
      case 'B':
        ui_change = 1;
        return KEY_DOWN;
      case 'C':
        ui_change = 1;
        return KEY_RIGHT;
      case 'D':
        ui_change = 1;
        return KEY_LEFT;
      }
    }

    return '\x1b';
  }

  return c;
}

int isSubsequenceLower(const char *q, const char *s) {
  while (*q && *s) {
    if (*q == *s)
      q++;
    s++;
  }
  return *q == '\0';
}

int startsWithLower(const char *s, const char *prefix) {
  while (*prefix && *s) {
    if (*s != *prefix)
      return 0;
    s++;
    prefix++;
  }
  return *prefix == '\0';
}

int containsLower(const char *s, const char *p) {
  size_t plen = strlen(p);
  if (plen == 0)
    return 1;

  for (size_t i = 0; s[i]; i++) {
    size_t j = 0;
    while (s[i + j] && p[j] && s[i + j] == p[j])
      j++;
    if (j == plen)
      return 1;
  }
  return 0;
}

int fuzzyScore(const char *queryLower, const char *nameLower, int queryLen,
               int nameLen) {
  int score = 0;

  if (queryLen == 0)
    return 1;

  if (strcmp(queryLower, nameLower) == 0)
    score += 1000;
  else if (startsWithLower(nameLower, queryLower))
    score += 300;
  else if (containsLower(nameLower, queryLower))
    score += 180;
  else if (isSubsequenceLower(queryLower, nameLower))
    score += 100;

  int len_diff = nameLen - queryLen;
  if (len_diff < 0)
    len_diff = -len_diff;
  score -= len_diff;

  return score;
}

void toLowerCopy(char *dst, const char *src) {
  while (*src) {
    *dst++ = (char)tolower((unsigned char)*src++);
  }
  *dst = '\0';
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

char *readCacheFile(const char *path, long *sizeOut) {
  struct stat st;

  if (stat(path, &st) != 0)
    return NULL;

  FILE *f = fopen(path, "r");
  if (!f)
    return NULL;

  char *buf = malloc((size_t)st.st_size + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t n = fread(buf, 1, (size_t)st.st_size, f);
  if (n != (size_t)st.st_size) {
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);

  buf[st.st_size] = '\0';

  if (sizeOut)
    *sizeOut = st.st_size;

  return buf;
}

int countLines(const char *src) {
  int n = 0;
  if (!*src)
    return 0;
  while (*src) {
    if (*src == '\n')
      n++;
    src++;
  }
  return src[-1] == '\n' ? n : n + 1;
}

int allocArrays(AppList a, int count, long fileSize) {

  a->count = count;

  a->pathList = malloc(sizeof(char *) * (size_t)count);
  a->mtimeList = malloc(sizeof(long) * (size_t)count);
  a->nameList = malloc(sizeof(char *) * (size_t)count);
  a->execCmdList = malloc(sizeof(char *) * (size_t)count);

  a->nameLowerList = malloc(sizeof(char *) * (size_t)count);
  a->nameLowerSrc = malloc((size_t)fileSize + 1);

  a->nameLenList = malloc(sizeof(int) * (size_t)count);

  return a->pathList && a->mtimeList && a->nameList && a->execCmdList &&
         a->nameLowerList && a->nameLowerSrc && a->nameLenList;
}

int split4(char *line, char **p, char **t, char **n, char **e) {

  *p = line;

  char *s = strchr(line, '\t');
  if (!s)
    return 0;
  *s++ = '\0';

  *t = s;

  s = strchr(s, '\t');
  if (!s)
    return 0;
  *s++ = '\0';

  *n = s;

  s = strchr(s, '\t');
  if (!s)
    return 0;
  *s++ = '\0';

  *e = s;

  return 1;
}

long getDirMTime(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return (long)st.st_mtime;
}

int writeMetaFile(const char *metaPath, long dirMTime) {
  FILE *f = fopen(metaPath, "w");
  if (!f)
    return 0;

  fprintf(f, "%ld\n", dirMTime);
  fclose(f);
  return 1;
}

long readMetaFile(const char *metaPath) {
  FILE *f = fopen(metaPath, "r");
  if (!f)
    return -1;

  long mtime = -1;
  if (fscanf(f, "%ld", &mtime) != 1)
    mtime = -1;

  fclose(f);
  return mtime;
}

int validateCache(AppList a) {
  for (int i = 0; i < a->count; i++) {
    struct stat st;
    if (stat(a->pathList[i], &st) != 0)
      return 0;

    if ((long)st.st_mtime != a->mtimeList[i])
      return 0;
  }
  return 1;
}

int loadCache(const char *cachePath, AppList a) {

  long fileSize = 0;
  char *src = readCacheFile(cachePath, &fileSize);
  if (!src)
    return 0;

  int count = countLines(src);

  if (!allocArrays(a, count, fileSize)) {
    free(src);
    return 0;
  }

  a->src = src;

  char *lower = a->nameLowerSrc;

  int i = 0;
  char *line = src;

  while (*line && i < count) {

    char *next = strchr(line, '\n');
    if (next)
      *next = '\0';

    char *path, *mtimeStr, *name, *exec;

    if (split4(line, &path, &mtimeStr, &name, &exec)) {

      a->pathList[i] = path;

      a->mtimeList[i] = strtol(mtimeStr, NULL, 10);

      a->nameList[i] = name;
      a->nameLenList[i] = (int)strlen(name);

      a->execCmdList[i] = exec;

      a->nameLowerList[i] = lower;
      toLowerCopy(lower, name);
      lower += strlen(lower) + 1;

      i++;
    }

    if (!next)
      break;

    line = next + 1;
  }

  a->count = i;

  return 1;
}

void writeToFile(FILE *cacheFile, const char *path, long mtime,
                 const char *appName, const char *execCmd) {
  char cleanedExec[512];
  snprintf(cleanedExec, sizeof(cleanedExec), "%s", execCmd);
  stripDesktopCodes(cleanedExec);

  fprintf(cacheFile, "%s\t%ld\t%s\t%s\n", path, mtime, appName, cleanedExec);
}

// function that scans through applications path and finds all apps that have a
// gui. returns amount of apps
int writeAppDataFile(const char *dataPath) {
  char path[] = "/usr/share/applications";
  DIR *dir;
  struct dirent *ent;
  int amount = 0;

  char appName[256];
  char execCmd[256];
  char desktopPath[512];

  FILE *cacheFile = fopen(dataPath, "w");
  if (!cacheFile)
    return 0;

  char *token = strtok(path, ":");
  while (token != NULL) {
    dir = opendir(token);
    if (dir != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        appName[0] = '\0';
        execCmd[0] = '\0';

        if (getGUIApps(token, ent->d_name, appName, execCmd)) {
          snprintf(desktopPath, sizeof(desktopPath), "%s/%s", token,
                   ent->d_name);

          struct stat st;
          long mtime = 0;
          if (stat(desktopPath, &st) == 0)
            mtime = (long)st.st_mtime;

          writeToFile(cacheFile, desktopPath, mtime, appName, execCmd);
          amount++;
        }
      }
      closedir(dir);
    }
    token = strtok(NULL, ":");
  }

  fclose(cacheFile);
  return amount;
}

void freeStorage(AppList a) {

  free(a->src);

  free(a->pathList);
  free(a->mtimeList);
  free(a->nameList);
  free(a->execCmdList);

  free(a->nameLowerList);
  free(a->nameLowerSrc);

  free(a->nameLenList);
}

Match *search(Match *top, char *query, AppList appList, int appAmount,
              int *top_n) {
  clearResUi();
  if (!top)
    return 0;

  for (int i = 0; i < *top_n; i++) {
    top[i].name = NULL;
    top[i].exec = NULL;
    top[i].score = 0;
  }

  if (query[0] == '\0') {
    for (int i = 0; i < *top_n; i++) {
      top[i].name = appList->nameList[i];
      top[i].exec = appList->execCmdList[i];
      top[i].score = 1;
    }
    return top;
  }

  char queryLower[512];
  toLowerCopy(queryLower, query);
  int queryLen = (int)strlen(queryLower);

  for (int i = 0; i < appAmount; i++) {
    int score = fuzzyScore(queryLower, appList->nameLowerList[i], queryLen,
                           appList->nameLenList[i]);

    tryInsertTop(top, *top_n, appList->nameList[i], appList->execCmdList[i],
                 score);
  }

  sortTop(top, *top_n);
  return top;
}

void printResults(Match *top, int top_n) {
  for (int i = 0; i < top_n; i++) {
    if (top[i].score > 0) {
      printf("\x1b[%d;0H %s ", termRows - 3 - i, top[i].name);
    }
  }
}

void launchApp(Match *top, int selected) {
  pid_t pid = fork();
  if (pid < 0)
    return;

  if (pid == 0) {
    signal(SIGHUP, SIG_IGN);

    if (setsid() < 0)
      _exit(127);

    pid_t pid2 = fork();
    if (pid2 < 0)
      _exit(127);
    if (pid2 > 0)
      _exit(0);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0)
      _exit(127);

    if (dup2(devnull, STDIN_FILENO) < 0)
      _exit(127);
    if (dup2(devnull, STDOUT_FILENO) < 0)
      _exit(127);
    if (dup2(devnull, STDERR_FILENO) < 0)
      _exit(127);
    if (devnull > 2)
      close(devnull);

    execl("/bin/sh", "sh", "-c", top[selected].exec, NULL);
    _exit(127);
  }

  waitpid(pid, NULL, 0);
}

void onStartUp(int *appAmount, AppList appList) {
  actRaw();
  actAltScr();

  char *homePath = getenv("HOME");
  char dataDir[500];
  char cachePath[512];
  char metaPath[512];

  snprintf(dataDir, sizeof(dataDir), "%s/.local/share/tux-launcher", homePath);
  mkdir(dataDir, 0755);

  snprintf(cachePath, sizeof(cachePath), "%s/cache.dat", dataDir);
  snprintf(metaPath, sizeof(metaPath), "%s/cache.meta", dataDir);

  getTermSize(&termRows, &termCols);
  basicFrame();

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
  *appAmount = appList->count;

  struct sigaction sa = {0};
  sa.sa_handler = handleWinch;
  sigaction(SIGWINCH, &sa, NULL);
}

void handleArrowKeyEvents(int key, int *selected, int top_n) {
  if (key == KEY_UP) {
    *selected = *selected + 1 < top_n ? *selected + 1 : *selected;
  } else if (key == KEY_DOWN) {
    *selected = *selected > 0 ? *selected - 1 : *selected;
  }
}

void highlightSelected(int selected, Match *top) {
  printf("\x1b[%d;1H", termRows - 3 - selected);
  if (top[0].score != 0)
    printf("\x1b[48;2;100;100;100m %s \x1b[0m", top[selected].name);
  else if (top[0].score == 0)
    printf("\033[38;2;100;100;100m %s \x1b[0m", "no result");
}

int keyProcessing(int key, char query[], int *queryLen, int *selected,
                  Match *top, int top_n) {
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
  } else if (key >= KEY_UP && key <= KEY_RIGHT) {
    handleArrowKeyEvents(key, selected, top_n);
    highlightSelected(*selected, top);
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

static int waitForInputOrSignal(void) {
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

void app(void) {
  char query[512] = {0};
  char altquery[512] = {0};
  int queryLen = 0;
  int appAmount = 0;
  int selected = 0;
  int oldselc = 0;
  struct applist appList = {0};

  onStartUp(&appAmount, &appList);
  int top_n = appAmount > termRows - 3 ? termRows - 3 : appAmount;

  Match *top = calloc((size_t)(termRows - 3), sizeof(Match));
  search(top, query, &appList, appAmount, &top_n);
  printResults(top, top_n);
  highlightSelected(selected, top);
  fflush(stdout);

  for (;;) {
    int ev = waitForInputOrSignal();
    if (ev < 0)
      break;

    if (resized) {
      resized = 0;
      getTermSize(&termRows, &termCols);
      basicFrame();

      int max_rows = termRows - 3;
      if (max_rows < 0)
        max_rows = 0;
      top_n = appAmount > max_rows ? max_rows : appAmount;

      top = realloc(top, (size_t)max_rows * sizeof(Match));
      if (!top && max_rows > 0)
        break;

      search(top, query, &appList, appAmount, &top_n);
      printQuery(query, queryLen);
      printResults(top, top_n);
      highlightSelected(selected, top);
      fflush(stdout);
    }

    if (ev == 1) {
      int key = readKey(); // darf jetzt blockierend / halbblockierend sein
      if (!keyProcessing(key, query, &queryLen, &selected, top, top_n)) {
        break;
      }

      if (selected != oldselc) {
        oldselc = selected;
        printResults(top, top_n);
        highlightSelected(selected, top);
      }

      if (queryChanged(query, altquery)) {
        printQuery(query, queryLen);
        selected = 0;
        oldselc = 0;
        top = search(top, query, &appList, appAmount, &top_n);
        printResults(top, top_n);
        highlightSelected(selected, top);
      }

      if (ui_change) {
        fflush(stdout);
        ui_change = 0;
      }
    }
  }

  free(top);
  freeStorage(&appList);
  deactAltScr();
  deactRaw();
}

int main() { app(); }
