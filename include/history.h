#ifndef HISTORY_H
#define HISTORY_H

int history_load(const char *path);
int history_get(const char *app_name);
void history_increment(const char *app_name);
void history_save(const char *path);
void history_record_launch(const char *app_name, const char *path);
void history_destroy(void);

#endif
