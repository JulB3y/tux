#ifndef ALIAS_MODULE_H
#define ALIAS_MODULE_H

#include "../../include/module.h"
#include "../../include/config.h"

void alias_module_set_config(Config *config);
Module *alias_module_create(void);

#endif