#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "config.h"

static char *trim_whitespace(char *str) {
  while (isspace((unsigned char)*str))
    str++;

  char *end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;

  *(end + 1) = '\0';
  return str;
}

static char *read_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file)
    return NULL;

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *content = malloc((size_t)size + 1);
  if (!content) {
    fclose(file);
    return NULL;
  }

  fread(content, 1, (size_t)size, file);
  content[(size_t)size] = '\0';
  fclose(file);

  return content;
}

static void add_config(Config *config, const char *app_name) {
  config->configs = realloc(config->configs, (size_t)(config->config_count + 1) * sizeof(AppConfig));
  config->configs[config->config_count].app_name = strdup(app_name);
  config->configs[config->config_count].keywords = NULL;
  config->configs[config->config_count].keyword_count = 0;
  config->config_count++;
}

static void add_keyword(Config *config, const char *keyword) {
  if (config->config_count == 0)
    return;

  AppConfig *app = &config->configs[config->config_count - 1];
  app->keywords = realloc(app->keywords, (size_t)(app->keyword_count + 1) * sizeof(char *));
  app->keywords[app->keyword_count] = strdup(keyword);
  app->keyword_count++;
}

static void tolower_str(char *str) {
  for (; *str; str++)
    *str = (char)tolower((unsigned char)*str);
}

static void parse_array(char *value, Config *config) {
  char *start = strchr(value, '[');
  char *end = strrchr(value, ']');

  if (!start || !end)
    return;

  start++;
  *end = '\0';

  char *token = strtok(start, ",");
  while (token) {
    char *trimmed = trim_whitespace(token);
    char *unquoted = trimmed;

    if (trimmed[0] == '"' && trimmed[strlen(trimmed) - 1] == '"') {
      unquoted = strdup(trimmed + 1);
      unquoted[strlen(unquoted) - 1] = '\0';
      tolower_str(unquoted);
      add_keyword(config, unquoted);
      free(unquoted);
    } else {
      char *lower = strdup(trimmed);
      tolower_str(lower);
      add_keyword(config, lower);
      free(lower);
    }

    token = strtok(NULL, ",");
  }
}

Config *config_init() {
  Config *config = malloc(sizeof(Config));
  config->configs = NULL;
  config->config_count = 0;

  char *home = getenv("HOME");
  if (!home) {
    return config;
  }

  char config_path[512];
  snprintf(config_path, sizeof(config_path), "%s/.config/tux/config.toml", home);

  char *content = read_file(config_path);
  if (!content) {
    return config;
  }

  char *saveptr;
  char *line = strtok_r(content, "\n", &saveptr);

  while (line) {
    char line_copy[512];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char *trimmed = trim_whitespace(line_copy);

    if (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']') {
      trimmed++;
      trimmed[strlen(trimmed) - 1] = '\0';
      if (strcmp(trimmed, "apps") == 0) {
        line = strtok_r(NULL, "\n", &saveptr);
        while (line) {
          strncpy(line_copy, line, sizeof(line_copy) - 1);
          line_copy[sizeof(line_copy) - 1] = '\0';
          trimmed = trim_whitespace(line_copy);

          if (trimmed[0] == '[') {
            break;
          }

          if (strchr(trimmed, '=')) {
            char line_copy2[512];
            strncpy(line_copy2, trimmed, sizeof(line_copy2) - 1);
            line_copy2[sizeof(line_copy2) - 1] = '\0';

            char *dot = strchr(line_copy2, '.');
            char *eq = strchr(line_copy2, '=');

            if (dot && dot < eq) {
              *dot = '\0';
              *eq = '\0';
              char *app_name = trim_whitespace(line_copy2);
              char *key = trim_whitespace(dot + 1);

              if (strcmp(key, "keywords") == 0) {
                char *value = trim_whitespace(eq + 1);
                add_config(config, app_name);
                parse_array(value, config);
              }
            }
          }

          line = strtok_r(NULL, "\n", &saveptr);
        }
      } else {
        line = strtok_r(NULL, "\n", &saveptr);
      }
    } else {
      line = strtok_r(NULL, "\n", &saveptr);
    }
  }

  free(content);
  return config;
}

void config_free(Config *config) {
  if (!config)
    return;

  for (int i = 0; i < config->config_count; i++) {
    free(config->configs[i].app_name);
    for (int j = 0; j < config->configs[i].keyword_count; j++) {
      free(config->configs[i].keywords[j]);
    }
    free(config->configs[i].keywords);
  }

  free(config->configs);
  free(config);
}

static int startsWithLower(const char *s, const char *prefix) {
  while (*prefix && *s) {
    if (*s != *prefix)
      return 0;
    s++;
    prefix++;
  }
  return *prefix == '\0';
}

static int containsLower(const char *haystack, const char *needle) {
  if (!*needle)
    return 1;

  while (*haystack) {
    const char *h = haystack;
    const char *n = needle;

    while (*h && *n && *h == *n) {
      h++;
      n++;
    }

    if (!*n)
      return 1;

    haystack++;
  }

  return 0;
}

static int isSubsequenceLower(const char *q, const char *s) {
  while (*q && *s) {
    if (*q == *s)
      q++;
    s++;
  }
  return *q == '\0';
}

int config_get_keyword_score(Config *config, const char *app_name, const char *query_lower, int query_len) {
  if (!config)
    return 0;

  char app_lower[256];
  strncpy(app_lower, app_name, sizeof(app_lower) - 1);
  app_lower[sizeof(app_lower) - 1] = '\0';
  for (char *p = app_lower; *p; p++)
    *p = (char)tolower((unsigned char)*p);

  int best_score = 0;

  for (int i = 0; i < config->config_count; i++) {
    if (strcmp(config->configs[i].app_name, app_lower) == 0) {
      for (int j = 0; j < config->configs[i].keyword_count; j++) {
        const char *keyword = config->configs[i].keywords[j];
        int keyword_len = (int)strlen(keyword);
        int keyword_score = 0;

        if (strcmp(query_lower, keyword) == 0)
          keyword_score += 1000;
        else if (startsWithLower(keyword, query_lower))
          keyword_score += 300;
        else if (containsLower(keyword, query_lower))
          keyword_score += 180;
        else if (isSubsequenceLower(query_lower, keyword))
          keyword_score += 100;

        int len_diff = keyword_len - query_len;
        if (len_diff < 0)
          len_diff = -len_diff;
        keyword_score -= len_diff;

        if (keyword_score > best_score)
          best_score = keyword_score;
      }
    }
  }

  return best_score;
}
