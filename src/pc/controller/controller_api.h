#ifndef CONTROLLER_API
#define CONTROLLER_API

#include <ultra64.h>

struct ControllerAPI {
    void (*init)(void);
    void (*read)(OSContPad *pad);
};

enum Input{
    keyboard,
    emscriptenKeyboard,
    sdlGameController,
    touchScreen,
    wup,
    xinput
};

extern void set_current_input(int in);
extern int get_current_input();

#endif
