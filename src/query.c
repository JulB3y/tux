#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "query.h"
#include "ui.h"
#include "exec.h"
#include "module.h"
#include "config.h"
#include "modules/apps.h"
#include "modules/calc.h"
#include "modules/web.h"
#include "modules/shell.h"

static ModuleRegistry *global_registry = NULL;
static Config *global_config = NULL;
static char *web_allocated_names[16] = {0};
static char *web_allocated_execs[16] = {0};
static int web_allocated_count = 0;

static void free_web_allocations(void) {
    for (int i = 0; i < web_allocated_count; i++) {
        free(web_allocated_names[i]);
        free(web_allocated_execs[i]);
        web_allocated_names[i] = NULL;
        web_allocated_execs[i] = NULL;
    }
    web_allocated_count = 0;
}

static int module_enabled(const char *name) {
  return config_get_module_enabled(global_config, name);
}

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

QueryType parseQuery(const char *query) {
  if (query == NULL || query[0] == '\0') {
    return QUERY_TYPE_APP_SEARCH;
  }

  if (query[0] == '$') {
    return QUERY_TYPE_COMMAND;
  }

  int has_operators = containsOperator(query);
  int has_functions = containsFunction(query);
  int has_parentheses = strchr(query, '(') != NULL || strchr(query, ')') != NULL;

  if (has_operators || has_functions || has_parentheses) {
    return QUERY_TYPE_CALCULATOR;
  }

  return QUERY_TYPE_APP_SEARCH;
}

int executeQuery(App *app, QueryType type) {
  if (!app || !global_registry)
    return 0;

  const char *query = app->ui.query;
  Module *target = NULL;

  switch (type) {
    case QUERY_TYPE_CALCULATOR:
      target = registry_find_by_name(global_registry, "calc");
      if (!target || !module_enabled("calc")) {
        app->ui.mode[0] = '\0';
        return 0;
      }
      break;
    case QUERY_TYPE_APP_SEARCH:
      target = registry_find_by_name(global_registry, "apps");
      if (!target || !module_enabled("apps")) {
        app->ui.mode[0] = '\0';
        return 0;
      }
      break;
    case QUERY_TYPE_WEB_SEARCH:
      target = registry_find_by_name(global_registry, "web");
      if (!target || !module_enabled("web")) {
        app->ui.mode[0] = '\0';
        return 0;
      }
      break;
    case QUERY_TYPE_COMMAND:
      target = registry_find_by_name(global_registry, "shell");
      if (!target || !module_enabled("shell")) {
        app->ui.mode[0] = '\0';
        return 0;
      }
      break;
    default:
      break;
  }

  if (!target || !target->search) {
    app->ui.mode[0] = '\0';
    return 0;
  }

  if (!target->initialized && target->init) {
    if (target->init() != 0) {
      app->ui.mode[0] = '\0';
      return 0;
    }
    target->initialized = 1;
  }

  Result results[16];
  memset(results, 0, sizeof(results));

  int count = target->search(query, results, 16);
  if (count <= 0) {
    if (type == QUERY_TYPE_APP_SEARCH && module_enabled("web")) {
      Module *web = registry_find_by_name(global_registry, "web");
      if (web && web->search) {
        count = web->search(query, results, 16);
        if (count > 0) {
          type = QUERY_TYPE_WEB_SEARCH;
          target = web;
        }
      }
    }
    if (count <= 0)
      return 0;
  }

  if (results[0].type == RESULT_CALC) {
    strncpy(app->ui.calc_result, results[0].title, sizeof(app->ui.calc_result) - 1);
    app->ui.calc_result[sizeof(app->ui.calc_result) - 1] = '\0';
    strcpy(app->ui.mode, "calc");
    app->top_n = 0;
    app->has_more_results = 0;
    return 1;
  }

  if (results[0].type == RESULT_URL) {
    free_web_allocations();
    strcpy(app->ui.mode, "web");

    char *browser = config_get_std_browser(app->config);
    const char *browser_cmd = browser ? browser : "xdg-open";

    for (int i = 0; i < count && i < 16; i++) {
      char exec_buf[1152];
      snprintf(exec_buf, sizeof(exec_buf), "%s \"%s\"", browser_cmd, results[i].subtitle);

      web_allocated_names[i] = strdup(results[i].title);
      web_allocated_execs[i] = strdup(exec_buf);
      app->top[i].name = web_allocated_names[i];
      app->top[i].exec = web_allocated_execs[i];
      app->top[i].score = results[i].score;
    }
    web_allocated_count = count;
    app->top_n = count;
    app->has_more_results = 0;
    return 1;
  }

  if (results[0].type == RESULT_COMMAND) {
    strcpy(app->ui.mode, "shell");
    app->top_n = 0;
    app->has_more_results = 0;
    return 1;
  }

  if (results[0].type == RESULT_APP) {
    strcpy(app->ui.mode, "apps");
    return 1;
  }

  return 0;
}

void modules_init(App *app) {
  if (global_registry)
    return;

  global_registry = registry_create();
  if (!global_registry)
    return;

  global_config = app->config;

  if (module_enabled("calc")) {
    Module *calc = calc_module_create();
    if (calc)
      registry_add_module(global_registry, calc);
  }

  if (module_enabled("apps")) {
    Module *apps = apps_module_create();
    if (apps) {
      apps_module_set_context(app, app->search_limit);
      registry_add_module(global_registry, apps);
    }
  }

  if (module_enabled("web")) {
    Module *web = web_module_create();
    if (web) {
      web_module_set_config(app->config);
      registry_add_module(global_registry, web);
    }
  }

  if (module_enabled("shell")) {
    Module *shell = shell_module_create();
    if (shell) {
      shell_module_set_config(app->config);
      registry_add_module(global_registry, shell);
    }
  }
}

void modules_shutdown(void) {
  free_web_allocations();
  if (global_registry) {
    registry_destroy(global_registry);
    global_registry = NULL;
  }
}
