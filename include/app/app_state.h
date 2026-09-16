#ifndef CERO_APP_STATE_H
#define CERO_APP_STATE_H

#include <stdint.h>

extern volatile int game_running;

extern int g_really_quit;

extern void* g_ui;

extern int64_t g_start_timestamp;

#endif
