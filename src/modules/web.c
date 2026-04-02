#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../include/module.h"
#include "../../include/types.h"
#include "../../include/config.h"

typedef struct {
    Config *config;
} WebModuleContext;

static WebModuleContext *web_ctx = NULL;

void web_module_set_config(Config *config) {
    if (!web_ctx) {
        web_ctx = malloc(sizeof(WebModuleContext));
        memset(web_ctx, 0, sizeof(WebModuleContext));
    }
    web_ctx->config = config;
}

static void url_encode(char *dest, const char *src) {
    while (*src) {
        if ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') ||
            (*src >= '0' && *src <= '9') || *src == '-' || *src == '_' ||
            *src == '.' || *src == '~') {
            *dest++ = *src;
        } else {
            sprintf(dest, "%%%02X", (unsigned char)*src);
            dest += 3;
        }
        src++;
    }
    *dest = '\0';
}

static int web_match(const char *query) {
    if (!query || query[0] == '\0')
        return 0;

    if (strncmp(query, "http://", 7) == 0 || strncmp(query, "https://", 8) == 0)
        return 1;

    if (strncmp(query, "www.", 4) == 0 && query[4] != '\0' && query[4] != ' ')
        return 1;

    const char *p = query;
    while (*p) {
        if (*p == ':' && isalpha((unsigned char)p[1])) {
            size_t len = (size_t)(p - query);
            if (len > 2 && len < 10) {
                char scheme[16];
                strncpy(scheme, query, len);
                scheme[len] = '\0';
                for (char *s = scheme; *s; s++)
                    *s = (char)tolower((unsigned char)*s);
                if (strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0 ||
                    strcmp(scheme, "ftp") == 0 || strcmp(scheme, "file") == 0)
                    return 1;
            }
        }
        p++;
    }

    return 0;
}

static int web_search(const char *query, Result *results, int max) {
    if (!query || !results || !web_ctx || !web_ctx->config)
        return 0;

    int web_count = 0;
    WebConfig *web_configs = config_get_web_configs(web_ctx->config, &web_count);
    if (!web_configs || web_count == 0)
        return 0;

    int result_count = 0;
    for (int i = 0; i < web_count && result_count < max; i++) {
        char encoded_query[512];
        url_encode(encoded_query, query);

        char url[1024];
        const char *url_template = web_configs[i].url;
        const char *percent_s = strstr(url_template, "%s");

        if (percent_s) {
            size_t prefix_len = (size_t)(percent_s - url_template);
            size_t suffix_len = strlen(percent_s + 2);
            size_t available = sizeof(url) - prefix_len - 1;
            strncpy(url, url_template, prefix_len);
            url[prefix_len] = '\0';
            strncat(url, encoded_query, available);
            size_t encoded_len = strlen(url) - prefix_len;
            if (encoded_len < available) {
                size_t remaining = available - encoded_len;
                if (remaining > suffix_len)
                    remaining = suffix_len;
                strncat(url, percent_s + 2, remaining);
            }
        } else {
            strncpy(url, url_template, sizeof(url) - 1);
            url[sizeof(url) - 1] = '\0';
        }

        results[result_count].type = RESULT_URL;
        snprintf(results[result_count].title, sizeof(results[result_count].title),
                 "Search on %s", web_configs[i].name);
        snprintf(results[result_count].subtitle, sizeof(results[result_count].subtitle),
                 "%.*s", (int)(sizeof(results[result_count].subtitle) - 1), url);
        results[result_count].score = 1000 - i;
        results[result_count].payload = NULL;
        results[result_count].flags = 0;

        result_count++;
    }

    return result_count;
}

static void web_execute(Result *result) {
    if (!result || !web_ctx || !web_ctx->config)
        return;

    char *browser = config_get_std_browser(web_ctx->config);
    const char *browser_cmd = browser ? browser : "xdg-open";

    char exec_cmd[1152];
    snprintf(exec_cmd, sizeof(exec_cmd), "%s \"%s\"", browser_cmd, result->subtitle);

    system(exec_cmd);
}

static void web_destroy(Module *module) {
    (void)module;
    if (web_ctx) {
        free(web_ctx);
        web_ctx = NULL;
    }
}

Module *web_module_create(void) {
    Module *module = malloc(sizeof(Module));
    if (!module)
        return NULL;

    memset(module, 0, sizeof(Module));
    module->name = "web";
    module->match = web_match;
    module->search = web_search;
    module->execute = web_execute;
    module->destroy = web_destroy;

    return module;
}
