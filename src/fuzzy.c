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

int fuzzyScore(const char **keywords, int keyword_count,
               const char *queryLower, const char *nameLower,
               int queryLen, int nameLen) {
  if (queryLen == 0)
    return 1;

  if (strcmp(queryLower, nameLower) == 0)
    return 1000;
  if (startsWithLower(nameLower, queryLower))
    return 300;
  if (containsLower(nameLower, queryLower))
    return 180;
  if (isSubsequenceLower(queryLower, nameLower))
    return 100;

  int score = 0;

  for (int i = 0; i < keyword_count; i++) {
    const char *keyword = keywords[i];
    int keyword_len = (int)strlen(keyword);
    int keyword_score = 0;

    if (strcmp(queryLower, keyword) == 0)
      keyword_score += 1000;
    else if (startsWithLower(keyword, queryLower))
      keyword_score += 300;
    else if (containsLower(keyword, queryLower))
      keyword_score += 180;
    else if (isSubsequenceLower(queryLower, keyword))
      keyword_score += 100;

    int len_diff = keyword_len - queryLen;
    if (len_diff < 0)
      len_diff = -len_diff;
    keyword_score -= len_diff;

    if (keyword_score > 0 && keyword_score > score)
      score = keyword_score;
  }

  int len_diff = nameLen - queryLen;
  if (len_diff < 0)
    len_diff = -len_diff;
  score -= len_diff;

  return score;
}
