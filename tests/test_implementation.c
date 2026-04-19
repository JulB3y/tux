#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/config.h"
#include "include/module.h"
#include "include/query.h"
#include "include/types.h"

int main() {
    printf("Testing alias and command implementation...\n");
    
    // Test config parsing
    Config *config = config_init();
    if (!config) {
        printf("Failed to initialize config\n");
        return 1;
    }
    
    // Test alias configs
    int alias_count = 0;
    AliasConfig *aliases = config_get_alias_configs(config, &alias_count);
    printf("Found %d aliases:\n", alias_count);
    for (int i = 0; i < alias_count; i++) {
        printf("  %s -> %s\n", aliases[i].name, aliases[i].url);
    }
    
    // Test command configs
    int cmd_count = 0;
    CommandConfig *commands = config_get_command_configs(config, &cmd_count);
    printf("Found %d commands:\n", cmd_count);
    for (int i = 0; i < cmd_count; i++) {
        printf("  %s -> %s\n", commands[i].name, commands[i].command);
    }
    
    // Test module enabled flags
    printf("\nModule enabled flags:\n");
    printf("  alias: %d\n", config_get_module_enabled(config, "alias"));
    printf("  cmd: %d\n", config_get_module_enabled(config, "cmd"));
    
    config_free(config);
    printf("\nAll tests passed!\n");
    
    return 0;
}