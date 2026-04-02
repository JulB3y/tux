#include <stdlib.h>
#include <string.h>

#include "../../include/module.h"
#include "../../include/types.h"
#include "../../include/fuzzy.h"
#include "../../include/ui.h"

typedef struct {
    App *app;
    int limit;
} AppSearchContext;

static AppSearchContext *app_ctx = NULL;

void apps_module_set_context(App *app, int limit) {
    if (!app_ctx) {
        app_ctx = malloc(sizeof(AppSearchContext));
        memset(app_ctx, 0, sizeof(AppSearchContext));
    }
    app_ctx->app = app;
    app_ctx->limit = limit;
}

static int apps_match(const char *query) {
    if (!query)
        return 0;
    if (query[0] == '\0')
        return 1;
    for (const char *p = query; *p; p++) {
        if (*p == ' ' || *p == '/' || *p == '$')
            return 0;
    }
    return 1;
}

static int apps_search(const char *query, Result *results, int max) {
    (void)results;
    (void)max;
    if (!app_ctx || !app_ctx->app)
        return 0;

    App *app = app_ctx->app;
    int limit = app_ctx->limit;

    if (limit <= 0)
        limit = app->search_limit;

    if (!app->top)
        return 0;

    clearResUi(app->term.rows);

    int result_count = 0;

    if (query[0] == '\0') {
        for (int i = 0; i < app->app_count && i < limit; i++) {
            app->top[i].name = app->apps.nameList[i];
            app->top[i].exec = app->apps.execCmdList[i];
            app->top[i].score = 1;
            result_count++;
        }
        app->has_more_results = (result_count < app->app_count);
    } else {
        for (int i = 0; i < app->app_count; i++) {
            int keyword_count = app->apps.keywords ? app->apps.keywords[i].keyword_count : 0;
            char **keywords = app->apps.keywords ? app->apps.keywords[i].keywords : NULL;

            int score = fuzzyScore((const char **)keywords, keyword_count,
                                   app->ui.query_lower,
                                   app->apps.nameLowerList[i],
                                   app->ui.query_len,
                                   app->apps.nameLenList[i]);

            if (score > 0) {
                app->top[result_count].name = app->apps.nameList[i];
                app->top[result_count].exec = app->apps.execCmdList[i];
                app->top[result_count].score = score;
                result_count++;
                if (result_count >= limit)
                    break;
            }
        }
        app->has_more_results = (result_count < app->app_count);
    }

    app->top_n = result_count;
    return result_count;
}

static void apps_execute(Result *result) {
    (void)result;
}

static void apps_destroy(Module *module) {
    (void)module;
    if (app_ctx) {
        free(app_ctx);
        app_ctx = NULL;
    }
}

Module *apps_module_create(void) {
    Module *module = malloc(sizeof(Module));
    if (!module)
        return NULL;

    memset(module, 0, sizeof(Module));
    module->name = "apps";
    module->match = apps_match;
    module->search = apps_search;
    module->execute = apps_execute;
    module->destroy = apps_destroy;

    return module;
}
