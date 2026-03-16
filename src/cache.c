#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "types.h"
#include "util.h"

int validateCache(AppList *a) {
  for (int i = 0; i < a->count; i++) {
    struct stat st;
    if (stat(a->pathList[i], &st) != 0)
      return 0;

    if ((long)st.st_mtime != a->mtimeList[i])
      return 0;
  }
  return 1;
}

static int countLines(const char *src) {
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

static int allocArrays(AppList *a, int count, long fileSize) {

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

static int split4(char *line, char **p, char **t, char **n, char **e) {

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

static char *readCacheFile(const char *path, long *sizeOut) {
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

int loadCache(const char *cachePath, AppList *a) {

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
