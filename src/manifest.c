#include <stdio.h>
#include "../include/download.h"
#include "../include/config.h"

void download_manifest(void) {
    const char manifest_url[] = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json";

    char manifest_path[MAX_PATH_SIZE];
    snprintf(manifest_path, sizeof(manifest_path), "%s/version_manifest.json", client_path);

    download_file(manifest_url, manifest_path);
}