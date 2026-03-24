#include <ctype.h>
#include <string.h>

#include "calc.h"
#include "query.h"
#include "search.h"
#include "ui.h"
#include "web.h"

static int containsOperator(const char *query) {
  const char *operators = "+-*/%^";
  return strpbrk(query, operators) != NULL;
}

static int containsFunction(const char *query) {
  const char *functions[] = {"sqrt", "sin", "cos", "tan", "log", "exp"};
  size_t num_functions = sizeof(functions) / sizeof(functions[0]);

  for (size_t i = 0; i < num_functions; i++) {
    if (strstr(query, functions[i]) != NULL) {
      return 1;
    }
  }
  return 0;
}

static int appSearchHandler(App *app, const char *query) {
  app->ui.calc_result[0] = '\0';
  strcpy(app->ui.mode, "apps");
  (void)query;
  int result = search(app, app->search_limit);

  if (result && app->ui.query[0] != '\0' && app->top_n == 0) {
    return executeQuery(app, QUERY_TYPE_WEB_SEARCH);
  }

  return result;
}

QueryType parseQuery(const char *query) {
  if (query == NULL || query[0] == '\0') {
    return QUERY_TYPE_APP_SEARCH;
  }

  int has_operators = containsOperator(query);
  int has_functions = containsFunction(query);
  int has_parentheses = strchr(query, '(') != NULL || strchr(query, ')') != NULL;

  if (has_operators || has_functions || has_parentheses) {
    return QUERY_TYPE_CALCULATOR;
  }

  return QUERY_TYPE_APP_SEARCH;
}

static QueryHandlerRegistration handlers[] = {
  {QUERY_TYPE_APP_SEARCH, appSearchHandler, "app_search"},
  {QUERY_TYPE_CALCULATOR, calcHandler, "calculator"},
  {QUERY_TYPE_FILE_SEARCH, NULL, "file_search"},
  {QUERY_TYPE_WEB_SEARCH, webSearchHandler, "web_search"},
  {QUERY_TYPE_COMMAND, NULL, "command"}
};

static const int handler_count = sizeof(handlers) / sizeof(handlers[0]);

int executeQuery(App *app, QueryType type) {
  for (int i = 0; i < handler_count; i++) {
    if (handlers[i].type == type) {
      if (handlers[i].handler != NULL) {
        return handlers[i].handler(app, app->ui.query);
      }
      break;
    }
  }
  return 0;
}
