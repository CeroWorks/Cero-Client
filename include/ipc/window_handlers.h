#ifndef CERO_WINDOW_HANDLERS_H
#define CERO_WINDOW_HANDLERS_H

void show_main_window(void);
void quit_app(void);

void on_minimize(const char* id, const char* req, void* arg);
void on_close_to_tray(const char* id, const char* req, void* arg);
void on_show_window(const char* id, const char* req, void* arg);
void on_drag_start(const char* id, const char* req, void* arg);
void on_quit_app(const char* id, const char* req, void* arg);

/* Wires up the tray icon and (on Windows) subclasses the main window's
 * WndProc so tray messages and WM_CLOSE-to-tray behave correctly.
 * Called once from main() right after the UI window is created.
 * Equivalent to the platform block that used to sit at the end of main(). */
void window_platform_init(void* ui_window, const char* icon_path);

#endif /* CERO_WINDOW_HANDLERS_H */
