#ifndef CERO_LAUNCH_FABRIC_H
#define CERO_LAUNCH_FABRIC_H

#include <stddef.h>

/* Parses a version spec like "fabric:1.20.1:0.15.0" (loader optional).
 * Returns 1 if it's a Fabric spec (mc/loader filled), 0 otherwise. */
int parse_fabric_spec(const char* spec, char* mc, size_t mcsz,
                      char* loader, size_t loadersz);

/* Resolves (downloading the loader list if needed) and downloads the
 * Fabric profile json for `mc_version`/`loader_version_in`.
 * Returns 1 on success, 0 on failure. */
int fetch_fabric_profile(const char* client_dir,
                         const char* mc_version,
                         const char* loader_version_in,
                         char* out_id, size_t out_id_sz,
                         char* out_json_path, size_t out_json_sz);

#endif /* CERO_LAUNCH_FABRIC_H */
