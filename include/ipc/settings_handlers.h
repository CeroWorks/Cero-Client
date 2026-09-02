#ifndef CERO_SETTINGS_HANDLERS_H
#define CERO_SETTINGS_HANDLERS_H

void on_get_settings(const char* id, const char* req, void* arg);
void on_save_settings(const char* id, const char* req, void* arg);
void on_get_version(const char* id, const char* req, void* arg);
void on_set_version(const char* id, const char* req, void* arg);

#endif /* CERO_SETTINGS_HANDLERS_H */
