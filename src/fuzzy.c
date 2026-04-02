#include <ctype.h>
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

static int isWordBoundary(const char *name, int pos) {
  if (pos == 0)
    return 1;
  char prev = name[pos - 1];
  if (prev == ' ' || prev == '-' || prev == '_' || prev == '.')
    return 1;
  if (islower((unsigned char)prev) && isupper((unsigned char)name[pos]))
    return 1;
  return 0;
}

static int findContainsPos(const char *haystack, const char *needle) {
  if (!*needle)
    return 0;
  for (int i = 0; haystack[i]; i++) {
    int j = 0;
    while (needle[j] && haystack[i + j] && haystack[i + j] == needle[j])
      j++;
    if (!needle[j])
      return i;
  }
  return -1;
}

static int findSubseqFirstPos(const char *q, const char *s) {
  for (int i = 0; s[i]; i++) {
    if (s[i] == q[0])
      return i;
  }
  return -1;
}

static int computeBonus(const char *queryLower, const char *nameLower,
                        const char *queryOrig, const char *nameOrig,
                        int queryLen, int matchPos) {
  int bonus = 0;

  if (isWordBoundary(nameOrig, matchPos))
    bonus += 25;

  if (queryOrig[0] == nameOrig[matchPos])
    bonus += 10;

  if (queryLen >= 2 && nameLower[matchPos + 1] == queryLower[1])
    bonus += 20;

  return bonus;
}

int fuzzyScore(const char **keywords, int keyword_count,
               const char *queryLower, const char *nameLower,
               int queryLen, int nameLen,
               const char *queryOrig, const char *nameOrig) {
  if (queryLen == 0)
    return 1;

  int bonus = 0;
  int base_score = 0;
  int matchPos = -1;

  if (strcmp(queryLower, nameLower) == 0) {
    base_score = 1000;
    matchPos = 0;
  } else if (startsWithLower(nameLower, queryLower)) {
    base_score = 300;
    matchPos = 0;
  } else if (containsLower(nameLower, queryLower)) {
    base_score = 180;
    matchPos = findContainsPos(nameLower, queryLower);
  } else if (isSubsequenceLower(queryLower, nameLower)) {
    base_score = 100;
    matchPos = findSubseqFirstPos(queryLower, nameLower);
  }

  if (base_score > 0 && matchPos >= 0)
    bonus = computeBonus(queryLower, nameLower, queryOrig, nameOrig,
                         queryLen, matchPos);

  int score = base_score + bonus;

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
