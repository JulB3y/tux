#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "exec.h"
#include "types.h"
#include "ui.h"
#include "web.h"

static void url_encode(char *dest, const char *src) {
  while (*src) {
    if ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') ||
        *src == '-' || *src == '_' || *src == '.' || *src == '~') {
      *dest++ = *src;
    } else {
      sprintf(dest, "%%%02X", (unsigned char)*src);
      dest += 3;
    }
    src++;
  }
  *dest = '\0';
}

int webSearchHandler(App *app, const char *query) {
  (void)query;
  app->ui.calc_result[0] = '\0';
  strcpy(app->ui.mode, "web");

  if (!app->config)
    return 0;

  int web_count = 0;
  WebConfig *web_configs = config_get_web_configs(app->config, &web_count);

  if (!web_configs || web_count == 0)
    return 0;

  if (app->ui.query[0] == '\0') {
    return 0;
  }

  int max_rows = app->term.rows - 3;
  if (max_rows < 0)
    max_rows = 0;

  int limit = app->search_limit;
  if (limit <= 0)
    limit = max_rows * 2;
  if (limit < 10)
    limit = 10;

  if (!app->top)
    return 0;

  int result_count = 0;
  int needed_capacity = web_count;

  if (needed_capacity > limit)
    needed_capacity = limit;

  app->top = realloc(app->top, (size_t)needed_capacity * sizeof(Match));
  if (!app->top)
    return 0;

  for (int i = 0; i < web_count && result_count < limit; i++) {
    char encoded_query[512];
    url_encode(encoded_query, app->ui.query);

    char url[1024];
    const char *url_template = web_configs[i].url;
    const char *percent_s = strstr(url_template, "%s");

    if (percent_s) {
      size_t prefix_len = (size_t)(percent_s - url_template);
      strncpy(url, url_template, prefix_len);
      url[prefix_len] = '\0';
      strncat(url, encoded_query, sizeof(url) - strlen(url) - 1);
      strncat(url, percent_s + 2, sizeof(url) - strlen(url) - 1);
    } else {
      strncpy(url, url_template, sizeof(url) - 1);
      url[sizeof(url) - 1] = '\0';
    }

    char *browser = config_get_std_browser(app->config);
    const char *browser_cmd = browser ? browser : "xdg-open";

    char exec_cmd[1152];
    snprintf(exec_cmd, sizeof(exec_cmd), "%s \"%s\"", browser_cmd, url);

    char display_name[512];
    snprintf(display_name, sizeof(display_name), "Search on %s", web_configs[i].name);

    app->top[result_count].name = malloc(strlen(display_name) + 1);
    if (app->top[result_count].name) {
      strcpy(app->top[result_count].name, display_name);
    }
    app->top[result_count].exec = malloc(strlen(exec_cmd) + 1);
    if (app->top[result_count].exec) {
      strcpy(app->top[result_count].exec, exec_cmd);
    }
    app->top[result_count].score = 1000 - i;
    result_count++;
  }

  app->top_n = result_count;
  app->has_more_results = 0;

  return 1;
}