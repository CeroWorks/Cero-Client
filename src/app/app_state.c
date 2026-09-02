#include "../../include/app/app_state.h"
#include <stddef.h>

volatile int game_running = 0;
int g_really_quit = 0;
void* g_ui = NULL;
int64_t g_start_timestamp = 0;
