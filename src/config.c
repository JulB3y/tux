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

static void add_web_config(Config *config, const char *name, const char *url) {
  if (config->web_config_count >= config->web_config_capacity) {
    int new_capacity = config->web_config_capacity == 0 ? INITIAL_CAPACITY : config->web_config_capacity * 2;
    config->web_configs = realloc(config->web_configs, (size_t)new_capacity * sizeof(WebConfig));
    config->web_config_capacity = new_capacity;
  }

  config->web_configs[config->web_config_count].name = pool_strdup(config, name);
  config->web_configs[config->web_config_count].url = pool_strdup(config, url);
  config->web_config_count++;
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
  config->web_configs = NULL;
  config->web_config_count = 0;
  config->web_config_capacity = 0;
  config->std_browser = NULL;
  config->enabled_apps = 1;
  config->enabled_calc = 1;
  config->enabled_web = 1;
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
      int is_double = (trimmed[1] == '[');
      trimmed++;
      if (is_double) trimmed++;
      size_t end_offset = is_double ? 2 : 1;
      trimmed[strlen(trimmed) - end_offset] = '\0';

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
      } else if (strcmp(trimmed, "web.modes") == 0) {
        char current_web_name[256] = {0};
        char current_web_url[512] = {0};

        int in_array_section = is_double;

        line = strtok_r(NULL, "\n", &saveptr);
        while (line) {
          strncpy(line_copy, line, sizeof(line_copy) - 1);
          line_copy[sizeof(line_copy) - 1] = '\0';
          trimmed = trim_whitespace(line_copy);

          if (trimmed[0] == '[') {
            if (in_array_section && trimmed[1] == '[') {
              if (current_web_name[0] != '\0' && current_web_url[0] != '\0') {
                add_web_config(config, current_web_name, current_web_url);
              }
              current_web_name[0] = '\0';
              current_web_url[0] = '\0';
            } else {
              if (current_web_name[0] != '\0' && current_web_url[0] != '\0') {
                add_web_config(config, current_web_name, current_web_url);
              }
              current_web_name[0] = '\0';
              current_web_url[0] = '\0';
              break;
            }
          }

          if (strchr(trimmed, '=')) {
            char *eq = strchr(trimmed, '=');
            *eq = '\0';
            char *key = trim_whitespace(trimmed);
            char *value = trim_whitespace(eq + 1);

            if (strcmp(key, "name") == 0) {
              char *unquoted = remove_quotes(trim_whitespace(value));
              strncpy(current_web_name, unquoted, sizeof(current_web_name) - 1);
              current_web_name[sizeof(current_web_name) - 1] = '\0';
            } else if (strcmp(key, "url") == 0) {
              char *unquoted = remove_quotes(trim_whitespace(value));
              strncpy(current_web_url, unquoted, sizeof(current_web_url) - 1);
              current_web_url[sizeof(current_web_url) - 1] = '\0';
            }
          }

          line = strtok_r(NULL, "\n", &saveptr);
        }

        if (current_web_name[0] != '\0' && current_web_url[0] != '\0') {
          add_web_config(config, current_web_name, current_web_url);
        }
      } else if (strcmp(trimmed, "settings") == 0) {
        line = strtok_r(NULL, "\n", &saveptr);
        while (line) {
          strncpy(line_copy, line, sizeof(line_copy) - 1);
          line_copy[sizeof(line_copy) - 1] = '\0';
          trimmed = trim_whitespace(line_copy);

          if (trimmed[0] == '[') {
            break;
          }

          if (strchr(trimmed, '=')) {
            char *eq = strchr(trimmed, '=');
            *eq = '\0';
            char *key = trim_whitespace(trimmed);
            char *value = trim_whitespace(eq + 1);

            if (strcmp(key, "std.browser") == 0) {
              char *unquoted = remove_quotes(value);
              config->std_browser = pool_strdup(config, unquoted);
            }
          }

          line = strtok_r(NULL, "\n", &saveptr);
        }
      } else if (strcmp(trimmed, "features") == 0) {
        line = strtok_r(NULL, "\n", &saveptr);
        while (line) {
          strncpy(line_copy, line, sizeof(line_copy) - 1);
          line_copy[sizeof(line_copy) - 1] = '\0';
          trimmed = trim_whitespace(line_copy);

          if (trimmed[0] == '[') {
            break;
          }

          if (strchr(trimmed, '=')) {
            char *eq = strchr(trimmed, '=');
            *eq = '\0';
            char *key = trim_whitespace(trimmed);
            char *value = trim_whitespace(eq + 1);

            int enabled = strcmp(value, "true") == 0 || strcmp(value, "1") == 0;

            if (strcmp(key, "apps") == 0) {
              config->enabled_apps = enabled;
            } else if (strcmp(key, "calc") == 0) {
              config->enabled_calc = enabled;
            } else if (strcmp(key, "web") == 0) {
              config->enabled_web = enabled;
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

char *config_get_std_browser(Config *config) {
  if (!config)
    return NULL;
  return config->std_browser;
}

WebConfig *config_get_web_configs(Config *config, int *count) {
  if (!config || !count) {
    if (count)
      *count = 0;
    return NULL;
  }

  *count = config->web_config_count;
  return config->web_configs;
}

int config_get_module_enabled(Config *config, const char *module_name) {
  if (!config || !module_name)
    return 1;

  if (strcmp(module_name, "apps") == 0)
    return config->enabled_apps;
  if (strcmp(module_name, "calc") == 0)
    return config->enabled_calc;
  if (strcmp(module_name, "web") == 0)
    return config->enabled_web;

  return 1;
}
