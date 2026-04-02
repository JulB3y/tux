#ifndef FUZZY_H
#define FUZZY_H

#include "types.h"

int fuzzyScore(const char **keywords, int keyword_count,
               const char *queryLower, const char *nameLower,
               int queryLen, int nameLen,
               const char *queryOrig, const char *nameOrig);

#endif
