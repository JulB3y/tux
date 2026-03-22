#ifndef UI_H
#define UI_H

#include "types.h"

void clearResUi(int rows);
void basicFrame(int *ui_changed, TermState *term);
void printResults(int rows, int cols, Match *top, int top_n, int scroll_offset, int max_rows);
void highlightSelected(Match *top, int selected, int scroll_offset, TermState *term, int max_rows);
void printQuery(UIState *ui, TermState *term);

#endif // !UI_H
