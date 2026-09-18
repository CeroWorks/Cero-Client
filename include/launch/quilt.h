#ifndef CERO_LAUNCH_QUILT_H
#define CERO_LAUNCH_QUILT_H

#include <stddef.h>

/*
 * Quilt is a Fabric fork that kept an (almost) identical meta API and the
 * same "fully resolved launch json" profile endpoint, so this mirrors
 * fabric.c closely — only the base URL and the resulting id prefix
 * differ.
 */

int parse_quilt_spec(const char* spec, char* mc, size_t mcsz,
                     char* loader, size_t loadersz);

int fetch_quilt_profile(const char* client_dir,
                        const char* mc_version,
                        const char* loader_version_in,
                        char* out_id, size_t out_id_sz,
                        char* out_json_path, size_t out_json_sz);

#endif
