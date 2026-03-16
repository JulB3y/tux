#ifndef FUZZY_H
#define FUZZY_H

int fuzzyScore(const char *queryLower, const char *nameLower, int queryLen,
               int nameLen);

#endif
