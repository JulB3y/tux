#ifndef MODULES_SHELL_H
#define MODULES_SHELL_H

#include "../../include/module.h"
#include "../../include/config.h"

Module *shell_module_create(void);
void shell_module_set_config(Config *config);

#endif
