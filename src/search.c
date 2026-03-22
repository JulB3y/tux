#include <stddef.h>

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

static void sortTopN(Match *top, int n) {
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (top[j].score > top[i].score) {
        Match tmp = top[i];
        top[i] = top[j];
        top[j] = tmp;
      }
    }
  }
}

int search(App *app) {
  clearResUi(app->term.rows);
  if (!app->top)
    return 0;

  int max_rows = app->term.rows - 3;
  if (max_rows < 0)
    max_rows = 0;

  int result_count = 0;

  if (app->ui.query[0] == '\0') {
    for (int i = 0; i < app->app_count; i++) {
      app->top[i].name = app->apps.nameList[i];
      app->top[i].exec = app->apps.execCmdList[i];
      app->top[i].score = 1;
      result_count++;
    }
  } else {
    MinHeap heap = {0};
    heap.data = app->top;
    heap.capacity = max_rows > 0 ? max_rows : 1;
    heap.size = 0;

    for (int i = 0; i < app->app_count; i++) {
      int score = fuzzyScore(app->ui.query_lower, app->apps.nameLowerList[i],
                             app->ui.query_len, app->apps.nameLenList[i]);

      if (score > 0) {
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
  }

  app->top_n = result_count;
  return 1;
}
