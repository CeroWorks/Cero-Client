#ifndef CERO_PATHS_H
#define CERO_PATHS_H

#include <stddef.h>

int  build_account_path(char* out, size_t sz);
int  build_settings_path(char* out, size_t sz);
int  file_exists(const char* path);
void get_exe_path(char* out, size_t sz);

#endif /* CERO_PATHS_H */
