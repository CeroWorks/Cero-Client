#include "../../include/platform/platform_defines.h"

#ifdef _WIN32
  #include <windows.h>
#endif

#include "../../include/ipc/window_handlers.h"
#include "../../include/app/app_state.h"
#include "../../include/platform/single_instance.h"
#include "../../include/ui/tray.h"
#include "../../include/ui/ui.h"
#include "../../include/core/logger.h"

#ifdef _WIN32
static WNDPROC g_orig_wndproc = NULL;
#endif

void show_main_window(void) {
#ifdef _WIN32
    HWND hwnd = (HWND)ui_get_window(g_ui);
    if (!hwnd) return;
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
#else
    if (!g_ui) return;
    void* win = ui_get_window(g_ui);
    if (!win) return;
    extern void ui_show_window(void* w);
    ui_show_window(g_ui);
#endif
}

void quit_app(void) {
    g_really_quit = 1;
    if (g_ui) ui_terminate(g_ui);
}

void on_minimize(const char* id, const char* req, void* arg) {
    (void)req;
    ui_minimize_window(arg);
    ui_return(arg, id, 0, "\"ok\"");
}

void on_close_to_tray(const char* id, const char* req, void* arg) {
    (void)req;
    ui_hide_window(arg);
    ui_return(arg, id, 0, "\"ok\"");
}

void on_show_window(const char* id, const char* req, void* arg) {
    (void)req;
    show_main_window();
    ui_return(arg, id, 0, "\"ok\"");
}

void on_drag_start(const char* id, const char* req, void* arg) {
    (void)req;
    ui_drag_start(arg);
    ui_return(arg, id, 0, "\"ok\"");
}

void on_quit_app(const char* id, const char* req, void* arg) {
    (void)req;
    g_really_quit = 1;
    ui_return(arg, id, 0, "\"ok\"");
    ui_terminate(arg);
}

#ifdef _WIN32
static LRESULT CALLBACK subclass_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    LRESULT tr;
    if (tray_handle_message(hwnd, msg, wp, lp, &tr)) return tr;

    if (msg == CERO_MSG_SHOW) {
        show_main_window();
        return 0;
    }

    if (msg == WM_CLOSE && !g_really_quit) {
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }

    return CallWindowProcA(g_orig_wndproc, hwnd, msg, wp, lp);
}
#endif

void window_platform_init(void* ui_window, const char* icon_path) {
#ifdef _WIN32
    HWND hwnd = (HWND)ui_get_window(ui_window);
    if (hwnd) {
        g_orig_wndproc = (WNDPROC)SetWindowLongPtrA(
            hwnd, GWLP_WNDPROC, (LONG_PTR)subclass_wndproc);

        if (tray_init(hwnd, icon_path, show_main_window, quit_app) != 0) {
            log_msg("warn", "Tray icon init failed\n");
        }
    }
#else
    if (tray_init(ui_get_window(ui_window), icon_path, show_main_window, quit_app) != 0) {
        log_msg("warn", "Tray icon init failed\n");
    }
#endif
}
