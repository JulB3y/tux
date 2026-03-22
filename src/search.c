#include <stddef.h>
#include <stdlib.h>

#include "config.h"
#include "fuzzy.h"
#include "search.h"
#include "types.h"
#include "ui.h"

typedef struct {
  Match *data;
  int size;
  int capacity;
} MinHeap;

static void heapify_down(MinHeap *heap, int idx) {
  while (1) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->data[left].score < heap->data[smallest].score)
      smallest = left;

    if (right < heap->size && heap->data[right].score < heap->data[smallest].score)
      smallest = right;

    if (smallest == idx)
      break;

    Match tmp = heap->data[idx];
    heap->data[idx] = heap->data[smallest];
    heap->data[smallest] = tmp;

    idx = smallest;
  }
}

static void heapify_up(MinHeap *heap, int idx) {
  while (idx > 0) {
    int parent = (idx - 1) / 2;
    if (heap->data[parent].score <= heap->data[idx].score)
      break;

    Match tmp = heap->data[idx];
    heap->data[idx] = heap->data[parent];
    heap->data[parent] = tmp;

    idx = parent;
  }
}

static int heap_insert(MinHeap *heap, Match match) {
  if (heap->size >= heap->capacity)
    return 0;

  heap->data[heap->size] = match;
  heapify_up(heap, heap->size);
  heap->size++;
  return 1;
}

static void heap_replace_min(MinHeap *heap, Match match) {
  if (heap->size == 0)
    return;

  heap->data[0] = match;
  heapify_down(heap, 0);
}

static int compare_match(const void *a, const void *b) {
  const Match *match_a = (const Match *)a;
  const Match *match_b = (const Match *)b;
  return match_b->score - match_a->score;
}

static void sortTopN(Match *top, int n) {
  qsort(top, (size_t)n, sizeof(Match), compare_match);
}

int search(App *app, int limit) {
  clearResUi(app->term.rows);
  if (!app->top)
    return 0;

  if (limit <= 0)
    limit = app->search_limit;

  int max_rows = app->term.rows - 3;
  if (max_rows < 0)
    max_rows = 0;

  int result_count = 0;

  if (app->ui.query[0] == '\0') {
    for (int i = 0; i < app->app_count && i < limit; i++) {
      app->top[i].name = app->apps.nameList[i];
      app->top[i].exec = app->apps.execCmdList[i];
      app->top[i].score = 1;
      result_count++;
    }
    app->has_more_results = (result_count < app->app_count);
  } else {
    MinHeap heap = {0};
    heap.data = app->top;
    heap.capacity = limit;
    heap.size = 0;

    int total_matches = 0;

    for (int i = 0; i < app->app_count; i++) {
      int keyword_count = app->apps.keywords ? app->apps.keywords[i].keyword_count : 0;
      char **keywords = app->apps.keywords ? app->apps.keywords[i].keywords : NULL;

      int score = fuzzyScore((const char **)keywords, keyword_count, app->ui.query_lower,
                             app->apps.nameLowerList[i], app->ui.query_len,
                             app->apps.nameLenList[i]);

      if (score > 0) {
        total_matches++;
        Match match = {app->apps.nameList[i], app->apps.execCmdList[i], score};

        if (heap.size < heap.capacity) {
          heap_insert(&heap, match);
          result_count++;
        } else if (score > heap.data[0].score) {
          heap_replace_min(&heap, match);
        }
      }
    }

    sortTopN(app->top, heap.size);
    result_count = heap.size;
    app->has_more_results = (total_matches > limit);
  }

  app->top_n = result_count;
  return 1;
}
