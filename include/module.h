#ifndef MODULE_H
#define MODULE_H

#include <stddef.h>

typedef enum {
    RESULT_APP,
    RESULT_FILE,
    RESULT_DIR,
    RESULT_WINDOW,
    RESULT_COMMAND,
    RESULT_CALC,
    RESULT_URL
} ResultType;

typedef struct {
    ResultType type;
    char title[256];
    char subtitle[512];
    int score;
    void *payload;
    unsigned flags;
} Result;

typedef struct Module Module;

struct Module {
    const char *name;
    int (*init)(void);
    int (*match)(const char *query);
    int (*search)(const char *query, Result *results, int max);
    void (*execute)(Result *result);
    void (*destroy)(Module *module);
    int initialized;
};

Module *module_create(const char *name);
void module_free(Module *module);

typedef struct {
    Module **modules;
    int count;
    int capacity;
} ModuleRegistry;

ModuleRegistry *registry_create(void);
void registry_add_module(ModuleRegistry *registry, Module *module);
Module *registry_find_by_name(ModuleRegistry *registry, const char *name);
Module *registry_find_by_query(ModuleRegistry *registry, const char *query);
void registry_destroy(ModuleRegistry *registry);

#endif