#ifndef CERO_LAUNCH_CLASSPATH_H
#define CERO_LAUNCH_CLASSPATH_H

#include "../config/config.h"
#include "../utils/version_manifest.h"

typedef struct {
    char group[256];
    char artifact[128];
    char version[64];
    char path[MAX_PATH_SIZE];
    char url[1024];
    int  needs_download;
} CpLib;

/* Parses a Maven "group:artifact:version" coordinate into `out`.
 * Returns 0 if `name` isn't a valid GAV string. */
int parse_maven_gav(const char* name, CpLib* out);

/* Adds `lib` to `arr` (dedup by group:artifact, keeping the first version
 * seen and logging a conflict warning on mismatch). */
void cp_add(CpLib* arr, int* count, int cap, const CpLib* lib);

/* Collects the libraries listed in a Fabric profile json into `arr`. */
void collect_fabric_libs(const char* client_dir, VmJVal* fabric_json,
                         CpLib* arr, int* count, int cap);

/* Collects the libraries listed in a vanilla version json into `arr`,
 * filtering out ones whose OS/arch rules don't match the current platform. */
void collect_vanilla_libs(const char* client_dir, VmJVal* version_json,
                          CpLib* arr, int* count, int cap);

/* Downloads every entry in `arr` flagged needs_download and not already
 * present on disk. */
void download_pending_libraries(const CpLib* arr, int count);

#endif /* CERO_LAUNCH_CLASSPATH_H */
