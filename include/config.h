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
  char *string_pool;
  size_t pool_size;
  size_t pool_capacity;

  AppConfig *configs;
  int config_count;
  int config_capacity;
} Config;

Config *config_init();
void config_free(Config *config);

char **config_get_keywords(Config *config, const char *app_name, int *keyword_count);

#endif
