#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/config.h"

int main() {
    printf("Debugging config parsing...\n");
    
    // Read the config file manually to see what's in it
    char *home = getenv("HOME");
    if (!home) {
        printf("HOME not set\n");
        return 1;
    }
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.config/tux/config.toml", home);
    
    FILE *file = fopen(config_path, "r");
    if (!file) {
        printf("Failed to open config file\n");
        return 1;
    }
    
    char line[512];
    int line_num = 0;
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        printf("%3d: %s", line_num, line);
        
        // Check for section headers
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (trimmed[0] == '[') {
            printf("     -> Section detected\n");
            
            int is_double = (trimmed[1] == '[');
            char *section_start = trimmed + 1;
            if (is_double) section_start++;
            
            char *section_end = strrchr(trimmed, ']');
            if (section_end) {
                *section_end = '\0';
                printf("     -> Section name: '%s' (double: %d)\n", section_start, is_double);
            }
        }
    }
    
    fclose(file);
    
    // Now test the actual config parsing
    Config *config = config_init();
    if (!config) {
        printf("Failed to initialize config\n");
        return 1;
    }
    
    int cmd_count = 0;
    CommandConfig *commands = config_get_command_configs(config, &cmd_count);
    printf("\nParsed %d commands:\n", cmd_count);
    for (int i = 0; i < cmd_count; i++) {
        printf("  %s -> %s\n", commands[i].name, commands[i].command);
    }
    
    config_free(config);
    
    return 0;
}