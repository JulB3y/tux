#ifndef UI_H
#define UI_H

#include "types.h"

void clearResUi(int rows);
void basicFrame(int *ui_changed, TermState *term);
void printResults(int rows, int cols, Match *top, int top_n);
void highlightSelected(Match *top, int selected, TermState *term);
void printQuery(UIState *ui, TermState *term);

#endif // !UI_H
