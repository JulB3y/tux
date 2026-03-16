// tux - main.c
//    basic c app launcher with fzf

// usefull info
//      activate   alternate screen with "\x1b[?1049h"
//      deactivate alternate screen with "\x1b[?1049l"

#include "app.h"
#include "types.h"

int main() {

  App *app = app_init();
  app_run(app);
}
