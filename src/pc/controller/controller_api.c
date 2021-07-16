#include "controller_api.h"
#if defined(__ANDROID__)
    int curInputSource=touchScreen;
#else
    int curInputSource=keyboard;
#endif


void set_current_input(int in){
    curInputSource=in;
}

int get_current_input(){
    return curInputSource;
}