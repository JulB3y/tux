#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/module.h"
#include "../../include/types.h"
#include "../../include/config.h"
#include "../../include/exec.h"
#include "shell.h"

typedef struct {
    Config *config;
} ShellModuleContext;

static ShellModuleContext *shell_ctx = NULL;

void shell_module_set_config(Config *config) {
    if (!shell_ctx) {
        shell_ctx = malloc(sizeof(ShellModuleContext));
        memset(shell_ctx, 0, sizeof(ShellModuleContext));
    }
    shell_ctx->config = config;
}

const char *shell_detect_terminal(void) {
    if (shell_ctx && shell_ctx->config) {
        char *configured = config_get_std_terminal(shell_ctx->config);
        if (configured)
            return configured;
    }

    const char *term = getenv("TERM");
    if (term && strncmp(term, "xterm-", 6) == 0) {
        const char *name = term + 6;
        if (strcmp(name, "kitty") == 0)
            return "kitty";
        if (strcmp(name, "ghostty") == 0)
            return "ghostty";
        if (strcmp(name, "alacritty") == 0)
            return "alacritty";
        return name;
    }

    const char *term_program = getenv("TERM_PROGRAM");
    if (term_program) {
        if (strcmp(term_program, "ghostty") == 0)
            return "ghostty";
        if (strcmp(term_program, "Alacritty") == 0 ||
            strcmp(term_program, "alacritty") == 0)
            return "alacritty";
    }

    return "xterm";
}

static int shell_match(const char *query) {
    if (!query || query[0] == '\0')
        return 0;

    return query[0] == '$';
}

static int shell_search(const char *query, Result *results, int max) {
    (void)max;
    if (!query || !results)
        return 0;

    if (query[0] != '$')
        return 0;

    const char *cmd = query + 1;
    while (*cmd == ' ')
        cmd++;

    if (*cmd == '\0')
        return 0;

    results[0].type = RESULT_COMMAND;
    snprintf(results[0].title, sizeof(results[0].title), "%s", cmd);
    snprintf(results[0].subtitle, sizeof(results[0].subtitle), "Run in terminal");
    results[0].score = 1000;
    results[0].payload = NULL;
    results[0].flags = 0;

    return 1;
}

static void shell_execute(Result *result) {
    if (!result)
        return;

    const char *terminal = shell_detect_terminal();
    exec_in_terminal(terminal, result->title);
}

static void shell_destroy(Module *module) {
    (void)module;
    if (shell_ctx) {
        free(shell_ctx);
        shell_ctx = NULL;
    }
}

Module *shell_module_create(void) {
    Module *module = malloc(sizeof(Module));
    if (!module)
        return NULL;

    memset(module, 0, sizeof(Module));
    module->name = "shell";
    module->match = shell_match;
    module->search = shell_search;
    module->execute = shell_execute;
    module->destroy = shell_destroy;

    return module;
}
