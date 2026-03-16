#ifndef TERM_H
#define TERM_H

#include "types.h"

void actAltScreen();
void deactAltScreen();
void actRaw(App *app);
void deactRaw(App *app);

int getTermSize(App *app);

#endif
