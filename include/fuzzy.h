#ifndef FUZZY_H
#define FUZZY_H

#include "config.h"

int fuzzyScore(Config *config, const char *app_name, const char *queryLower,
               const char *nameLower, int queryLen, int nameLen);

#endif
