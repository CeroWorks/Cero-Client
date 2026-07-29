#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/stat.h>
#endif
#include "../../include/logger.h"
#include "../../include/download.h"
#include "../../include/utils/file_utils.h"
#include "../../include/utils/version_manifest.h"
#include "../../include/config.h"

static int file_exists_fast(const char* path) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(path);
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

int download_assets(const char* client_dir, const char* version) {
    log_msg("info", "[assets] === download_assets START ===\n");
    log_msg("info", "[assets] client_dir=%s version=%s\n", client_dir, version);

    char ai_url[512], ai_id[64];
    log_msg("info", "[assets] step 1: get asset index info\n");
    if (!version_get_asset_index(client_dir, version, ai_url, sizeof(ai_url),
                                                       ai_id,  sizeof(ai_id))) {
        log_msg("error", "[assets] Cannot get asset index info\n");
        return 0;
    }
    log_msg("info", "[assets] ai_id=%s ai_url=%s\n", ai_id, ai_url);

    char idx_path[MAX_PATH_SIZE];
    snprintf(idx_path, sizeof(idx_path),
             "%s/assets/indexes/%s.json", client_dir, ai_id);
    log_msg("info", "[assets] step 2: index path=%s\n", idx_path);

    if (!file_exists_fast(idx_path)) {
        log_msg("info", "[assets] index not found, downloading\n");
        download_file(ai_url, idx_path);
    } else {
        log_msg("info", "[assets] index already present\n");
    }

    log_msg("info", "[assets] step 3: loading asset list\n");
    AssetList assets;
    if (!assets_load_index(client_dir, ai_id, &assets)) {
        log_msg("error", "[assets] Cannot load asset index %s\n", ai_id);
        return 0;
    }
    log_msg("info", "[assets] loaded %zu entries\n", assets.count);

    for (size_t i = 0; i < assets.count && i < 3; i++) {
        AssetEntry *e = &assets.items[i];
        log_msg("info", "[assets] sample[%zu] hash=%s url=%s\n",
                i,
                e->hash ? e->hash : "(NULL)",
                e->url  ? e->url  : "(NULL)");
    }

    log_msg("info", "[assets] step 4: scanning + preparing jobs\n");

    download_job_t* jobs  = malloc(sizeof(*jobs)  * assets.count);
    char**          paths = malloc(sizeof(char*)  * assets.count);
    if (!jobs || !paths) {
        log_msg("erreur", "[assets] malloc failed\n");
        free(jobs); free(paths);
        asset_list_free(&assets);
        return 0;
    }

    int idx = 0;
    for (size_t i = 0; i < assets.count; i++) {
        AssetEntry *e = &assets.items[i];

        if (!e->hash || strlen(e->hash) < 2) {
            log_msg("erreur", "[assets] BAD hash at %zu\n", i);
            continue;
        }
        if (!e->url) {
            log_msg("erreur", "[assets] NULL url at %zu hash=%s\n", i, e->hash);
            continue;
        }

        char dest[MAX_PATH_SIZE];
        int n = snprintf(dest, sizeof(dest), "%s/assets/objects/%c%c/%s",
                         client_dir, e->hash[0], e->hash[1], e->hash);
        if (n <= 0 || (size_t)n >= sizeof(dest)) {
            log_msg("erreur", "[assets] path truncated at %zu\n", i);
            continue;
        }

        if (file_exists_fast(dest)) continue;

        paths[idx] = strdup(dest);
        if (!paths[idx]) {
            log_msg("erreur", "[assets] strdup failed at idx=%d\n", idx);
            continue;
        }
        jobs[idx].url  = e->url;
        jobs[idx].path = paths[idx];

        if (idx < 3) {
            log_msg("info", "[assets] job[%d] url=%s\n", idx, jobs[idx].url);
            log_msg("info", "[assets] job[%d] path=%s\n", idx, jobs[idx].path);
        }
        idx++;

        if (i % 500 == 0)
            log_msg("info", "[assets] scan progress %zu/%zu (missing=%d)\n",
                    i, assets.count, idx);
    }
    log_msg("info", "[assets] scan done, %d jobs to download\n", idx);

    log_msg("info", "[assets] step 5: calling download_files_parallel\n");
    int ok = download_files_parallel(jobs, idx, 32);
    log_msg("info", "[assets] download_files_parallel returned %d\n", ok);

    log_msg("succes", "Assets: %d/%zu ready (%d downloaded)\n",
            (int)assets.count, assets.count, ok);

    log_msg("info", "[assets] cleanup: freeing %d paths\n", idx);
    for (int i = 0; i < idx; i++) free(paths[i]);
    free(paths);
    free(jobs);
    log_msg("info", "[assets] cleanup: asset_list_free\n");
    asset_list_free(&assets);
    log_msg("info", "[assets] === download_assets END ===\n");
    return 1;
}
