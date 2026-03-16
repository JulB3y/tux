#ifndef CACHE_H
#define CACHE_H

#include "types.h"

int validateCache(AppList *a);
int loadCache(const char *cachePath, AppList *a);

#endif
