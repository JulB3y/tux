#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int getGUIApps(char *d, char *n, char *appName, char *execCmd) {
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

static void stripDesktopCodes(char *s) {
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

static void writeToFile(FILE *cacheFile, const char *path, long mtime,
                        const char *appName, const char *execCmd) {
  char cleanedExec[512];
  snprintf(cleanedExec, sizeof(cleanedExec), "%s", execCmd);
  stripDesktopCodes(cleanedExec);

  fprintf(cacheFile, "%s\t%ld\t%s\t%s\n", path, mtime, appName, cleanedExec);
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

long getDirMTime(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return (long)st.st_mtime;
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
