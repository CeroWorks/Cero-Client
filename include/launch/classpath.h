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

int parse_maven_gav(const char* name, CpLib* out);

void cp_add(CpLib* arr, int* count, int cap, const CpLib* lib);

void collect_fabric_libs(const char* client_dir, VmJVal* fabric_json,
                         CpLib* arr, int* count, int cap);

void collect_vanilla_libs(const char* client_dir, VmJVal* version_json,
                          CpLib* arr, int* count, int cap);

void download_pending_libraries(const CpLib* arr, int count);

#endif 
