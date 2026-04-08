#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

typedef struct {
  char *app_name;
  char **keywords;
  int keyword_count;
  int keyword_capacity;
} AppConfig;

typedef struct {
  char *name;
  char *url;
} WebConfig;

typedef struct {
  char *string_pool;
  size_t pool_size;
  size_t pool_capacity;

  AppConfig *configs;
  int config_count;
  int config_capacity;

  WebConfig *web_configs;
  int web_config_count;
  int web_config_capacity;

  char *std_browser;
  char *std_terminal;

  int enabled_apps;
  int enabled_calc;
  int enabled_web;
  int enabled_shell;
} Config;

Config *config_init();
void config_free(Config *config);

char **config_get_keywords(Config *config, const char *app_name, int *keyword_count);
char *config_get_std_browser(Config *config);
char *config_get_std_terminal(Config *config);
WebConfig *config_get_web_configs(Config *config, int *count);
int config_get_module_enabled(Config *config, const char *module_name);

#endif
