#include "../../include/platform/platform_defines.h"

#ifndef _WIN32
  #include <sys/stat.h>
#endif

#include "../../include/app/app.h"
#include "../../include/app/app_state.h"
#include "../../include/ipc/bridge.h"
#include "../../include/platform/single_instance.h"
#include "../../include/config/config.h"
#include "../../include/app/assets_loader.h"
#include "../../include/discord/discord_rpc.h"
#include "../../include/discord/rpc_helpers.h"
#include "../../include/ui/ui.h"
#include "../../include/ipc/window_handlers.h"
#include "../../include/ui/tray.h"
#include "../../include/ipc/game_handlers.h"
#include "../../include/ipc/auth_handlers.h"
#include "../../include/ipc/settings_handlers.h"
#include "../../include/ipc/system_handlers.h"
#include "../../include/launch/launch_minecraft.h"
#include "../../include/platform/paths.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int launcher_parse_args(int argc, char** argv, LauncherOptions* opts) {
    opts->assets_path = "assets.dat";
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--customAssetsPath=", 19) == 0) {
            opts->assets_path = argv[i] + 19;
        }
    }
    return 1;
}

int launcher_init(const LauncherOptions* opts) {
    bridge_start();
    single_instance_write_port(local_bridge_port);

    init_config();

#ifndef _WIN32
    if (client_path[0] == '\0' || strcmp(client_path, "") == 0) {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(client_path, MAX_PATH_SIZE, "%s/.ceroclient", home);
        } else {
            snprintf(client_path, MAX_PATH_SIZE, "./.ceroclient");
        }
        mkdir(client_path, 0755);
    }
#endif

    install_cleanup_handlers();

    log_msg("info", "Loading Assets into RAM from %s...\n", opts->assets_path);

    char exe_path[1024];
    get_exe_path(exe_path, sizeof(exe_path));

    if (!assets_load(opts->assets_path, exe_path)) {
        log_msg("error", "Failed to load %s\n", opts->assets_path);
        return 0;
    }

    log_msg("info", "Starting CeroClient ...\n");

    g_start_timestamp = (int64_t)time(NULL);
    if (discord_rpc_init("1376179097113333881") == 0) {
        log_msg("info", "Discord RPC connected\n");
        rpc_set_launching();
    } else {
        log_msg("warn", "Discord not running or RPC failed\n");
    }

    return 1;
}

int launcher_create_ui(void) {
    char url[1024];
    snprintf(url, sizeof(url), "cero:///app/index.html");

    void* w = ui_create("CeroClient");
    if (!w) return 0;

#if defined(__linux__) || defined(__BSD__)
    ui_watch_system_theme(w);
#endif

    ui_set_frameless(w);

    const char* icon_path = NULL;
    ui_set_icon(w, icon_path);

    g_ui = w;
    ui_navigate(w, url);
    return 1;
}

void launcher_bind_ui(void) {
    void* w = g_ui;

    ui_bind(w, "launch_mc",     on_launch,        w);
    ui_bind(w, "drag_start",    on_drag_start,    w);
    ui_bind(w, "checkAccount",  on_check_account, w);
    ui_bind(w, "checkInternet", on_check_internet,w);
    ui_bind(w, "loginMicrosoft", on_login_microsoft, w);
    ui_bind(w, "logout_account", on_logout_account, w);
    ui_bind(w, "getAccount", on_get_account, w);
    ui_bind(w, "getMcToken", on_get_mc_token, w);
    ui_bind(w, "getVersion", on_get_version, w);
    ui_bind(w, "setVersion", on_set_version, w);
    ui_bind(w, "shell_open", on_shell_open, w);
    ui_bind(w, "get_settings",  on_get_settings,  w);
    ui_bind(w, "save_settings", on_save_settings, w);
    ui_bind(w, "minimize_window", on_minimize,      w);
    ui_bind(w, "show_window", on_show_window, w);
    ui_bind(w, "close_to_tray",   on_close_to_tray, w);
    ui_bind(w, "kill_game", on_kill_game, w);
    ui_bind(w, "quit_app", on_quit_app, w);

    ui_enable_js_console(w);
    ui_lockdown(w);
}

void launcher_start_services(void) {
    window_platform_init(g_ui, NULL);
}

void launcher_shutdown(void) {
    tray_shutdown();
    single_instance_release();
    discord_rpc_shutdown();
}
