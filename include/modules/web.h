#ifndef MODULES_WEB_H
#define MODULES_WEB_H

#include "../../include/module.h"
#include "../../include/types.h"
#include "../../include/config.h"

void web_module_set_config(Config *config);
Module *web_module_create(void);

#endif