#ifndef ASSETS_LOADER_H
#define ASSETS_LOADER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int assets_load(const char *dat_path, const char *exe_path);
int assets_get_file(const char *name, const uint8_t **out_data, size_t *out_size);
void assets_free_buffer(const uint8_t *data);
void assets_unload(void);
void install_cleanup_handlers(void);

#ifdef __cplusplus
}
#endif

#endif