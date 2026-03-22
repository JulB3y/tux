#ifndef INPUT_H
#define INPUT_H

#include "types.h"

int readKey(void);
int waitForInputOrSignal(void);
int keyProcessing(App *app, int key);
#endif
