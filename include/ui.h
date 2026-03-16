#ifndef UI_H
#define UI_H

#include "types.h"

void clearResUi(App *app);
void basicFrame(App *app);
void printResults(App *app);
void highlightSelected(Match *top, int selected, TermState *term);
void printQuery(UIState *ui, TermState *term);

#endif // !UI_H
