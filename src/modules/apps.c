#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/module.h"
#include "../../include/types.h"
#include "../../include/fuzzy.h"
#include "../../include/ui.h"
#include "../../include/modules/apps.h"

typedef struct {
    App *app;
    int limit;
} AppSearchContext;

#define HISTORY_WEIGHT 50

static AppSearchContext *app_ctx = NULL;

void apps_module_set_context(App *app, int limit) {
    if (!app_ctx) {
        app_ctx = malloc(sizeof(AppSearchContext));
        memset(app_ctx, 0, sizeof(AppSearchContext));
    }
    app_ctx->app = app;
    app_ctx->limit = limit;
}

static int cmp_match_desc(const void *a, const void *b) {
    int sa = ((const Match *)a)->score;
    int sb = ((const Match *)b)->score;
    return (sb > sa) - (sb < sa);
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
        int max_results = app->search_limit;
        for (int i = 0; i < app->app_count && result_count < max_results; i++) {
            int keyword_count = app->apps.keywords ? app->apps.keywords[i].keyword_count : 0;
            char **keywords = app->apps.keywords ? app->apps.keywords[i].keywords : NULL;

            int score = fuzzyScore((const char **)keywords, keyword_count,
                                   app->ui.query_lower,
                                   app->apps.nameLowerList[i],
                                   app->ui.query_len,
                                   app->apps.nameLenList[i],
                                   app->ui.query,
                                   app->apps.nameList[i]);

            if (score > 0) {
                int freq = app->apps.launchCounts ? app->apps.launchCounts[i] : 0;
                if (freq > 0)
                    score += (int)(log((double)freq + 1) * HISTORY_WEIGHT);

                app->top[result_count].name = app->apps.nameList[i];
                app->top[result_count].exec = app->apps.execCmdList[i];
                app->top[result_count].score = score;
                result_count++;
            }
        }
        qsort(app->top, (size_t)result_count, sizeof(Match), cmp_match_desc);
        app->has_more_results = (result_count >= max_results);
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
