#ifndef CERO_WINDOW_HANDLERS_H
#define CERO_WINDOW_HANDLERS_H

void show_main_window(void);
void quit_app(void);

void on_minimize(const char* id, const char* req, void* arg);
void on_close_to_tray(const char* id, const char* req, void* arg);
void on_show_window(const char* id, const char* req, void* arg);
void on_drag_start(const char* id, const char* req, void* arg);
void on_quit_app(const char* id, const char* req, void* arg);

void window_platform_init(void* ui_window, const char* icon_path);

#endif 
