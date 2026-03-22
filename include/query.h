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

typedef int (*QueryHandler)(App *app, const char *query);

typedef struct {
  QueryType type;
  QueryHandler handler;
  const char *name;
} QueryHandlerRegistration;

QueryType parseQuery(const char *query);
int executeQuery(App *app, QueryType type);

#endif
