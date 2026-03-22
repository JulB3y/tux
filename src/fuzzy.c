#include <string.h>

#include "fuzzy.h"

static int isSubsequenceLower(const char *q, const char *s) {
  while (*q && *s) {
    if (*q == *s)
      q++;
    s++;
  }
  return *q == '\0';
}

static int startsWithLower(const char *s, const char *prefix) {
  while (*prefix && *s) {
    if (*s != *prefix)
      return 0;
    s++;
    prefix++;
  }
  return *prefix == '\0';
}

static int containsLower(const char *haystack, const char *needle) {
  if (!*needle)
    return 1;

  while (*haystack) {
    const char *h = haystack;
    const char *n = needle;

    while (*h && *n && *h == *n) {
      h++;
      n++;
    }

    if (!*n)
      return 1;

    haystack++;
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
