#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "config.h"

#define INITIAL_CAPACITY 8
#define INITIAL_POOL_SIZE 4096

static char *trim_whitespace(char *str) {
  while (isspace((unsigned char)*str))
    str++;

  char *end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;

  *(end + 1) = '\0';
  return str;
}

static char *remove_quotes(char *str) {
  size_t len = strlen(str);
  if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
    str[len - 1] = '\0';
    return str + 1;
  }
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

static void tolower_str(char *str) {
  for (; *str; str++)
    *str = (char)tolower((unsigned char)*str);
}

static char *pool_strdup(Config *config, const char *str) {
  size_t len = strlen(str) + 1;

  if (config->pool_size + len > config->pool_capacity) {
    size_t new_capacity = config->pool_capacity * 2;
    while (config->pool_size + len > new_capacity)
      new_capacity *= 2;

    char *new_pool = realloc(config->string_pool, new_capacity);
    if (!new_pool)
      return NULL;

    config->string_pool = new_pool;
    config->pool_capacity = new_capacity;
  }

  char *result = config->string_pool + config->pool_size;
  memcpy(result, str, len);
  config->pool_size += len;

  return result;
}

static void add_config(Config *config, const char *app_name) {
  if (config->config_count >= config->config_capacity) {
    int new_capacity = config->config_capacity == 0 ? INITIAL_CAPACITY : config->config_capacity * 2;
    config->configs = realloc(config->configs, (size_t)new_capacity * sizeof(AppConfig));
    config->config_capacity = new_capacity;
  }

  char app_lower[256];
  strncpy(app_lower, app_name, sizeof(app_lower) - 1);
  app_lower[sizeof(app_lower) - 1] = '\0';
  tolower_str(app_lower);

  config->configs[config->config_count].app_name = pool_strdup(config, app_lower);
  config->configs[config->config_count].keywords = NULL;
  config->configs[config->config_count].keyword_count = 0;
  config->configs[config->config_count].keyword_capacity = 0;
  config->config_count++;
}

static void add_keyword(Config *config, const char *keyword) {
  if (config->config_count == 0)
    return;

  AppConfig *app = &config->configs[config->config_count - 1];

  if (app->keyword_count >= app->keyword_capacity) {
    int new_capacity = app->keyword_capacity == 0 ? INITIAL_CAPACITY : app->keyword_capacity * 2;
    app->keywords = realloc(app->keywords, (size_t)new_capacity * sizeof(char *));
    app->keyword_capacity = new_capacity;
  }

  app->keywords[app->keyword_count] = pool_strdup(config, keyword);
  app->keyword_count++;
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
    char keyword_copy[256];
    size_t trimmed_len = strlen(trimmed);

    strncpy(keyword_copy, trimmed, sizeof(keyword_copy) - 1);
    keyword_copy[sizeof(keyword_copy) - 1] = '\0';

    char *keyword_to_add = keyword_copy;

    if (keyword_copy[0] == '"' && trimmed_len > 1 && keyword_copy[trimmed_len - 1] == '"') {
      keyword_copy[trimmed_len - 1] = '\0';
      keyword_to_add = keyword_copy + 1;
    }

    tolower_str(keyword_to_add);
    add_keyword(config, keyword_to_add);

    token = strtok(NULL, ",");
  }
}

Config *config_init() {
  Config *config = malloc(sizeof(Config));
  config->configs = NULL;
  config->config_count = 0;
  config->config_capacity = 0;
  config->string_pool = malloc(INITIAL_POOL_SIZE);
  config->pool_size = 0;
  config->pool_capacity = INITIAL_POOL_SIZE;

  if (!config->string_pool) {
    free(config);
    return NULL;
  }

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
              char *trimmed_app_name = trim_whitespace(line_copy2);

              int has_quotes = (trimmed_app_name[0] == '"' &&
                               trimmed_app_name[strlen(trimmed_app_name) - 1] == '"');

              if (strchr(trimmed_app_name, ' ') && !has_quotes) {
                line = strtok_r(NULL, "\n", &saveptr);
                continue;
              }

              char *app_name = remove_quotes(trim_whitespace(line_copy2));
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

  free(config->string_pool);
  free(config->configs);
  free(config);
}

char **config_get_keywords(Config *config, const char *app_name, int *keyword_count) {
  if (!config || !app_name || !keyword_count) {
    *keyword_count = 0;
    return NULL;
  }

  char app_lower[256];
  strncpy(app_lower, app_name, sizeof(app_lower) - 1);
  app_lower[sizeof(app_lower) - 1] = '\0';
  tolower_str(app_lower);

  for (int i = 0; i < config->config_count; i++) {
    if (strcmp(config->configs[i].app_name, app_lower) == 0) {
      *keyword_count = config->configs[i].keyword_count;
      return config->configs[i].keywords;
    }
  }

  *keyword_count = 0;
  return NULL;
}
