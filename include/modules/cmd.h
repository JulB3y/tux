#ifndef CMD_MODULE_H
#define CMD_MODULE_H

#include "../../include/module.h"
#include "../../include/config.h"

void cmd_module_set_config(Config *config);
Module *cmd_module_create(void);

#endif