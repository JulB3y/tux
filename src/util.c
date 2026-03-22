#include <ctype.h>

#include "util.h"

void toLowerCopy(char *dst, const char *src) {
  while (*src) {
    *dst++ = (char)tolower((unsigned char)*src++);
  }
  *dst = '\0';
}
