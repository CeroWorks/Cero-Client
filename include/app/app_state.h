#ifndef CERO_APP_STATE_H
#define CERO_APP_STATE_H

#include <stdint.h>

/* 1 while a Minecraft process launched via on_launch is running. */
extern volatile int game_running;

/* Set to 1 when the user actually wants to quit (vs. close-to-tray). */
extern int g_really_quit;

/* The main UI window handle, set once in main() after ui_create(). */
extern void* g_ui;

/* Timestamp used as the Discord RPC "elapsed" start. */
extern int64_t g_start_timestamp;

#endif /* CERO_APP_STATE_H */
