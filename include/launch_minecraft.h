#ifndef LAUNCH_MINECRAFT_H
#define LAUNCH_MINECRAFT_H

extern int local_bridge_port;

typedef void (*launch_progress_cb)(const char* step, int percent, void* userdata);

typedef struct {
    void* ui;
    int*  game_running;
} LaunchUserdata;

void launch_minecraft(const char* version,
                      launch_progress_cb cb, void* userdata);

#endif
