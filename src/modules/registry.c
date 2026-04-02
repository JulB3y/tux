#include <stdlib.h>
#include <string.h>

#include "module.h"

Module *module_create(const char *name) {
    Module *module = malloc(sizeof(Module));
    if (!module)
        return NULL;

    memset(module, 0, sizeof(Module));
    module->name = name;
    return module;
}

void module_free(Module *module) {
    if (!module)
        return;

    if (module->destroy)
        module->destroy(module);

    free(module);
}

ModuleRegistry *registry_create(void) {
    ModuleRegistry *registry = malloc(sizeof(ModuleRegistry));
    if (!registry)
        return NULL;

    registry->modules = NULL;
    registry->count = 0;
    registry->capacity = 0;
    return registry;
}

void registry_add_module(ModuleRegistry *registry, Module *module) {
    if (!registry || !module)
        return;

    if (registry->count >= registry->capacity) {
        int new_capacity = registry->capacity == 0 ? 8 : registry->capacity * 2;
        Module **new_modules = realloc(registry->modules, (size_t)new_capacity * sizeof(Module *));
        if (!new_modules)
            return;

        registry->modules = new_modules;
        registry->capacity = new_capacity;
    }

    registry->modules[registry->count++] = module;
}

Module *registry_find_by_name(ModuleRegistry *registry, const char *name) {
    if (!registry || !name)
        return NULL;

    for (int i = 0; i < registry->count; i++) {
        if (registry->modules[i] && strcmp(registry->modules[i]->name, name) == 0)
            return registry->modules[i];
    }
    return NULL;
}

Module *registry_find_by_query(ModuleRegistry *registry, const char *query) {
    if (!registry || !query)
        return NULL;

    for (int i = 0; i < registry->count; i++) {
        Module *module = registry->modules[i];
        if (module && module->match && module->match(query))
            return module;
    }
    return NULL;
}

void registry_destroy(ModuleRegistry *registry) {
    if (!registry)
        return;

    for (int i = 0; i < registry->count; i++) {
        module_free(registry->modules[i]);
    }
    free(registry->modules);
    free(registry);
}