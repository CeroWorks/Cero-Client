#ifndef CERO_LAUNCH_FABRIC_H
#define CERO_LAUNCH_FABRIC_H

#include <stddef.h>

int parse_fabric_spec(const char* spec, char* mc, size_t mcsz,
                      char* loader, size_t loadersz);

int fetch_fabric_profile(const char* client_dir,
                         const char* mc_version,
                         const char* loader_version_in,
                         char* out_id, size_t out_id_sz,
                         char* out_json_path, size_t out_json_sz);

#endif 
