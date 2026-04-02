#ifndef QUERY_H
#define QUERY_H

#include "types.h"

typedef enum {
  QUERY_TYPE_APP_SEARCH,
  QUERY_TYPE_CALCULATOR,
  QUERY_TYPE_FILE_SEARCH,
  QUERY_TYPE_WEB_SEARCH,
  QUERY_TYPE_COMMAND
} QueryType;

QueryType parseQuery(const char *query);
int executeQuery(App *app, QueryType type);
void modules_init(App *app);
void modules_shutdown(void);

#endif
