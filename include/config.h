#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
  char *app_name;
  char **keywords;
  int keyword_count;
} AppConfig;

struct Config {
  AppConfig *configs;
  int config_count;
};

typedef struct Config Config;

Config *config_init();
void config_free(Config *config);
int config_get_keyword_score(Config *config, const char *app_name, const char *query_lower, int query_len);

#endif
