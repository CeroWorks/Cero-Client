#include "../../include/launch/fabric.h"
#include "../../include/launch/mc_json.h"
#include "../../include/config/config.h"
#include "../../include/net/download.h"
#include "../../include/utils/file_utils.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FABRIC_META "https://meta.fabricmc.net/v2"

int parse_fabric_spec(const char* spec, char* mc, size_t mcsz,
                      char* loader, size_t loadersz) {
    loader[0] = '\0';
    if (strncmp(spec, "fabric:", 7) != 0) return 0;
    const char* p = spec + 7;
    const char* colon = strchr(p, ':');
    if (colon) {
        size_t n = (size_t)(colon - p);
        if (n >= mcsz) n = mcsz - 1;
        memcpy(mc, p, n); mc[n] = '\0';
        snprintf(loader, loadersz, "%s", colon + 1);
    } else {
        snprintf(mc, mcsz, "%s", p);
    }
    return 1;
}

int fetch_fabric_profile(const char* client_dir,
                         const char* mc_version,
                         const char* loader_version_in,
                         char* out_id, size_t out_id_sz,
                         char* out_json_path, size_t out_json_sz) {
    char loader_version[64];
    snprintf(loader_version, sizeof(loader_version), "%s", loader_version_in);

    if (loader_version[0] == '\0') {
        char list_url[512];
        snprintf(list_url, sizeof(list_url),
                 "%s/versions/loader/%s", FABRIC_META, mc_version);

        char list_path[MAX_PATH_SIZE];
        snprintf(list_path, sizeof(list_path),
                 "%s/cache/fabric_loaders_%s.json", client_dir, mc_version);
        ensure_parent_dirs(list_path);

        if (download_file(list_url, list_path) == 0) {
            log_msg("error", "Cannot fetch Fabric loader list for %s\n", mc_version);
            return 0;
        }

        char* txt = read_file(list_path);
        if (!txt) return 0;

        const char* p = strstr(txt, "\"loader\"");
        if (p) p = strstr(p, "\"version\"");
        if (!p) { free(txt); log_msg("error", "No loader found in list\n"); return 0; }
        p += strlen("\"version\"");
        while (*p == ' ' || *p == ':' || *p == '\t') p++;
        if (*p != '"') { free(txt); return 0; }
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof(loader_version))
            loader_version[i++] = *p++;
        loader_version[i] = '\0';
        free(txt);

        log_msg("info", "Using latest Fabric loader: %s\n", loader_version);
    }

    snprintf(out_id, out_id_sz, "fabric-loader-%s-%s",
             loader_version, mc_version);

    char profile_url[512];
    snprintf(profile_url, sizeof(profile_url),
             "%s/versions/loader/%s/%s/profile/json",
             FABRIC_META, mc_version, loader_version);

    snprintf(out_json_path, out_json_sz,
             "%s/versions/%s/%s.json", client_dir, out_id, out_id);
    ensure_parent_dirs(out_json_path);

    if (download_file(profile_url, out_json_path) == 0) {
        log_msg("error", "Cannot download Fabric profile from %s\n", profile_url);
        return 0;
    }

    return 1;
}
