#include "../../include/platform/platform_defines.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <pthread.h>
#endif

#include "../../include/ipc/game_handlers.h"
#include "../../include/app/app_state.h"
#include "../../include/launch/launch_minecraft.h"
#include "../../include/utils/process.h"
#include "../../include/ui/ui.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char  version[64];
    void* ui;
    int* game_running;
} LaunchArgs;

static void launch_progress(const char* step, int pct, void* userdata) {
    LaunchUserdata* ud = (LaunchUserdata*)userdata;
    void* w = ud ? ud->ui : NULL;
    if (!w) return;

    char step_safe[256];
    size_t si = 0, di = 0;
    while (step[si] && di + 2 < sizeof(step_safe)) {
        if (step[si] == '"' || step[si] == '\\') step_safe[di++] = '\\';
        step_safe[di++] = step[si++];
    }
    step_safe[di] = '\0';

    char js[512];
    if (pct < 0) {
        snprintf(js, sizeof(js),
            "if(window.onLaunchError) onLaunchError(\"%s\")", step_safe);
    } else {
        snprintf(js, sizeof(js),
            "if(window.onLaunchProgress) onLaunchProgress(\"%s\",%d)",
            step_safe, pct);
    }
    ui_eval(w, js);
}

#ifdef _WIN32
static DWORD WINAPI launch_thread(LPVOID arg) {
#else
static void* launch_thread(void* arg) {
#endif
    LaunchArgs* la = (LaunchArgs*)arg;
    LaunchUserdata ud = { la->ui, la->game_running };
    launch_minecraft(la->version, launch_progress, &ud);
    free(la);
    return 0;
}

void on_launch(const char* id, const char* req, void* arg) {
    if (game_running) {
        ui_return(arg, id, 1, "\"already_running\"");
        return;
    }

    char version[64] = "";
    const char* p = req;
    while (*p && *p != '"') p++;
    if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof(version))
            version[i++] = *p++;
        version[i] = '\0';
    }

    if (version[0] == '\0') {
        ui_return(arg, id, 1, "\"no_version\"");
        return;
    }

    log_msg("info", "Launch requested for version: %s\n", version);

    LaunchArgs* la = malloc(sizeof(LaunchArgs));
    strncpy(la->version, version, sizeof(la->version) - 1);
    la->version[sizeof(la->version)-1] = '\0';
    la->ui = arg;
    la->game_running = (int*)&game_running;

#ifdef _WIN32
    HANDLE h = CreateThread(NULL, 0, launch_thread, la, 0, NULL);
    if (h) CloseHandle(h);
#else
    pthread_t t;
    pthread_create(&t, NULL, launch_thread, la);
    pthread_detach(t);
#endif

    ui_return(arg, id, 0, "\"ok\"");
}

void on_kill_game(const char* id, const char* req, void* arg) {
    (void)req;
    process_kill();
    ui_return(arg, id, 0, "\"ok\"");
}

void on_game_status(const char* id, const char* req, void* arg) {
    (void)req;
    ui_return(arg, id, 0, game_running ? "true" : "false");
}
