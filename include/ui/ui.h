#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

void* ui_create(const char* title);
void  ui_navigate(void* w, const char* url);
void  ui_run(void* w);
void  ui_bind(void* w, const char* name,
              void (*fn)(const char* id, const char* req, void* arg),
              void* arg);
void  ui_return(void* w, const char* id, int status, const char* result);
void ui_set_frameless(void* w);
void ui_drag_start(void* w);
void ui_set_icon(void* w, const char* icon_path);
void* ui_get_window(void* w);
void  ui_terminate(void* w);
void ui_eval(void* w, const char* js);
void ui_enable_js_console(void* w);
void ui_lockdown(void* w);

void ui_show_window(void* w);
void ui_hide_window(void* w);
void ui_minimize_window(void* w);

#if defined(__linux__) || defined(__BSD__)
void ui_watch_system_theme(void* w);
#endif

#ifdef __cplusplus
}
#endif

#endif
