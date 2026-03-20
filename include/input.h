#ifndef INPUT_H
#define INPUT_H

#include "types.h"

int readKey(App *app);
int waitForInputOrSignal(void);
int keyProcessing(int key, UIState *ui, Match *top, TermState *term);
#endif
