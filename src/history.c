#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"

#define HISTORY_MAX_ENTRIES 1000

typedef struct {
    char *name;
    int frequency;
} HistoryEntry;

static HistoryEntry *hist_entries = NULL;
static int hist_count = 0;
static int hist_capacity = 0;

int history_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;

    hist_capacity = 64;
    hist_entries = malloc((size_t)hist_capacity * sizeof(HistoryEntry));
    if (!hist_entries) {
        fclose(f);
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *tab = strchr(line, '\t');
        if (!tab)
            continue;
        *tab = '\0';
        char *freq_str = tab + 1;
        char *nl = strchr(freq_str, '\n');
        if (nl)
            *nl = '\0';

        if (hist_count >= hist_capacity) {
            hist_capacity *= 2;
            HistoryEntry *tmp = realloc(hist_entries,
                                        (size_t)hist_capacity * sizeof(HistoryEntry));
            if (!tmp)
                break;
            hist_entries = tmp;
        }

        hist_entries[hist_count].name = strdup(line);
        hist_entries[hist_count].frequency = atoi(freq_str);
        hist_count++;
    }

    fclose(f);
    return 1;
}

int history_get(const char *app_name) {
    if (!hist_entries)
        return 0;
    for (int i = 0; i < hist_count; i++) {
        if (strcmp(hist_entries[i].name, app_name) == 0)
            return hist_entries[i].frequency;
    }
    return 0;
}

void history_increment(const char *app_name) {
    for (int i = 0; i < hist_count; i++) {
        if (strcmp(hist_entries[i].name, app_name) == 0) {
            hist_entries[i].frequency++;
            return;
        }
    }

    if (hist_count >= hist_capacity) {
        int new_cap = hist_capacity == 0 ? 64 : hist_capacity * 2;
        HistoryEntry *tmp = realloc(hist_entries,
                                    (size_t)new_cap * sizeof(HistoryEntry));
        if (!tmp)
            return;
        hist_entries = tmp;
        hist_capacity = new_cap;
    }

    hist_entries[hist_count].name = strdup(app_name);
    hist_entries[hist_count].frequency = 1;
    hist_count++;
}

static int cmp_by_freq_desc(const void *a, const void *b) {
    int fa = ((const HistoryEntry *)a)->frequency;
    int fb = ((const HistoryEntry *)b)->frequency;
    return (fb > fa) - (fb < fa);
}

void history_save(const char *path) {
    if (hist_count > HISTORY_MAX_ENTRIES) {
        qsort(hist_entries, (size_t)hist_count, sizeof(HistoryEntry),
              cmp_by_freq_desc);
        for (int i = HISTORY_MAX_ENTRIES; i < hist_count; i++)
            free(hist_entries[i].name);
        hist_count = HISTORY_MAX_ENTRIES;
    }

    FILE *f = fopen(path, "w");
    if (!f)
        return;

    for (int i = 0; i < hist_count; i++)
        fprintf(f, "%s\t%d\n", hist_entries[i].name, hist_entries[i].frequency);

    fclose(f);
}

void history_record_launch(const char *app_name, const char *path) {
    history_increment(app_name);
    history_save(path);
}

void history_destroy(void) {
    if (!hist_entries)
        return;
    for (int i = 0; i < hist_count; i++)
        free(hist_entries[i].name);
    free(hist_entries);
    hist_entries = NULL;
    hist_count = 0;
    hist_capacity = 0;
}
