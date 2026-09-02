#ifndef TRAY_H
#define TRAY_H

typedef void (*tray_show_cb)(void);
typedef void (*tray_quit_cb)(void);

#ifdef _WIN32
#include <windows.h>

int  tray_init(HWND main_hwnd, const char* icon_path,
               tray_show_cb on_show, tray_quit_cb on_quit);
void tray_shutdown(void);

int  tray_handle_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT* out);

#else

int  tray_init(void* main_window, const char* icon_path,
               tray_show_cb on_show, tray_quit_cb on_quit);
void tray_shutdown(void);

#endif

#endif
