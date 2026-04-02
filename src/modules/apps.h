#ifndef MODULES_APPS_H
#define MODULES_APPS_H

#include "../../include/module.h"
#include "../../include/types.h"

void apps_module_set_context(App *app, int limit);
Module *apps_module_create(void);

#endif
