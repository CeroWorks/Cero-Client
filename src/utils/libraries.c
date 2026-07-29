#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/logger.h"
#include "../../include/download.h"
#include "../../include/utils/file_utils.h"
#include "../../include/utils/version_manifest.h"
#include "../../include/config.h"
#include "../../include/miniz.h"
#include "../../include/sha1.h"
#include "../../include/utils/platform.h"

static int is_blacklisted(const char* filename) {
    if (strncmp(filename, "META-INF", 8) == 0) return 1;

    const char* dot = strrchr(filename, '.');
    if (!dot) return 1;

#if defined(_WIN32)
    if (strcmp(dot, ".dll") != 0) return 1;

    #if defined(_WIN64)
        size_t len = strlen(filename);
        if (len >= 6 && strcmp(filename + len - 6, "32.dll") == 0) return 1;
        if (strstr(filename, "_x86.dll")) return 1;
    #else
        if (strstr(filename, "_x64.dll")) return 1;
    #endif
#elif defined(__APPLE__)
    if (strcmp(dot, ".dylib") != 0 && strcmp(dot, ".jnilib") != 0) return 1;
#else
    if (strcmp(dot, ".so") != 0) return 1;
#endif

    return 0;
}

static int extract_natives(const char* zip_path, const char* out_dir) {
    mz_zip_archive zip = {0};
    if (!mz_zip_reader_init_file(&zip, zip_path, 0)) {
        log_msg("error", "Cannot open native zip %s\n", zip_path);
        return 0;
    }

    int n = (int)mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < n; i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) continue;
        if (st.m_is_directory) continue;
        if (is_blacklisted(st.m_filename)) continue;

        char out_path[MAX_PATH_SIZE];
        snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, st.m_filename);
        ensure_parent_dirs(out_path);
        mz_zip_reader_extract_to_file(&zip, i, out_path, 0);
    }

    mz_zip_reader_end(&zip);
    return 1;
}

static int file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

int download_libraries(const char* client_dir, const char* version) {
    LibraryList libs;
    if (!version_get_libraries(client_dir, version, &libs)) {
        log_msg("error", "Cannot load libraries for %s\n", version);
        return 0;
    }

    download_job_t* jobs = malloc(sizeof(*jobs) * libs.count * 2);
    char** paths = malloc(sizeof(char*) * libs.count * 2);
    const char** expected_sha1 = malloc(sizeof(char*) * libs.count * 2);
    int njobs = 0;

    for (size_t i = 0; i < libs.count; i++) {
        LibraryEntry *e = &libs.items[i];

        if (e->url && e->path) {
            char* dest = malloc(MAX_PATH_SIZE);
            snprintf(dest, MAX_PATH_SIZE, "%s/libraries/%s", client_dir, e->path);
            if (!file_exists(dest)) {
                jobs[njobs].url  = e->url;
                jobs[njobs].path = dest;
                paths[njobs] = dest;
                expected_sha1[njobs] = e->sha1;
                njobs++;
            } else {
                free(dest);
            }
        }

        if (e->native_url && e->native_path) {
            char* dest = malloc(MAX_PATH_SIZE);
            snprintf(dest, MAX_PATH_SIZE, "%s/libraries/%s", client_dir, e->native_path);
            if (!file_exists(dest)) {
                jobs[njobs].url  = e->native_url;
                jobs[njobs].path = dest;
                paths[njobs] = dest;
                expected_sha1[njobs] = e->native_sha1;
                njobs++;
            } else {
                free(dest);
            }
        }
    }

    int ok = 0;
    if (njobs > 0) {
        ok = download_files_parallel(jobs, njobs, 16);
    }

    int verified = 0, rejected = 0;
    for (int i = 0; i < njobs; i++) {
        if (!file_exists(paths[i])) continue; 
        if (!sha1_file_matches(paths[i], expected_sha1[i])) {
            log_msg("error",
                "SHA-1 invalide pour la librairie %s (attendu %s) — fichier supprimé\n",
                jobs[i].url, expected_sha1[i] ? expected_sha1[i] : "(absent)");
            remove(paths[i]);
            rejected++;
        } else {
            verified++;
        }
    }

    log_msg("succes", "Libraries: %zu ready (%d downloaded, %d skipped, %d verifiees, %d rejetees)\n",
            libs.count, ok, (int)(libs.count * 2 - njobs), verified, rejected);

    for (int i = 0; i < njobs; i++) free(paths[i]);
    free(paths);
    free(expected_sha1);
    free(jobs);
    library_list_free(&libs);
    return rejected == 0;
}

static int is_native_for_current_os(const char* path) {
    if (!path) return 0;

#if defined(_WIN32)
    #if defined(_WIN64)
        if (strstr(path, "-natives-windows.jar"))        return 1;
        if (strstr(path, "-natives-windows-x86_64.jar")) return 1;
    #else
        if (strstr(path, "-natives-windows-x86.jar"))    return 1;
    #endif
#elif defined(__APPLE__)
    #if defined(__aarch64__)
        if (strstr(path, "-natives-macos-arm64.jar"))    return 1;
        if (strstr(path, "-natives-macos.jar"))          return 1;
    #else
        if (strstr(path, "-natives-macos.jar"))          return 1;
    #endif
#else
    #if defined(__aarch64__)
        if (strstr(path, "-natives-linux-aarch_64.jar")) return 1;
        if (strstr(path, "-natives-linux.jar"))          return 1;
    #else
        if (strstr(path, "-natives-linux.jar"))          return 1;
    #endif
#endif
    return 0;
}

int extract_all_natives(const char* client_dir, const char* version) {
    char natives_dir[MAX_PATH_SIZE];
    snprintf(natives_dir, sizeof(natives_dir),
             "%s/versions/%s/natives", client_dir, version);
    ensure_directory_exists(natives_dir);

    LibraryList libs;
    if (!version_get_libraries(client_dir, version, &libs)) return 0;

    int count = 0;

    for (size_t i = 0; i < libs.count; i++) {
        LibraryEntry *e = &libs.items[i];

        
        if (e->native_path) {
            char jar_path[MAX_PATH_SIZE];
            snprintf(jar_path, sizeof(jar_path),
                     "%s/libraries/%s", client_dir, e->native_path);
            if (file_exists(jar_path) && extract_natives(jar_path, natives_dir))
                count++;
            continue;
        }

        
        if (e->path && is_native_for_current_os(e->path)) {
            char jar_path[MAX_PATH_SIZE];
            snprintf(jar_path, sizeof(jar_path),
                     "%s/libraries/%s", client_dir, e->path);

            if (!file_exists(jar_path)) {
                log_msg("info", "Native jar missing, downloading: %s\n", e->path);
                if (e->url) {
                    ensure_parent_dirs(jar_path);
                    download_file(e->url, jar_path);
                    if (!sha1_file_matches(jar_path, e->sha1)) {
                        log_msg("error",
                            "SHA-1 invalide pour le native %s — fichier supprimé\n", e->path);
                        remove(jar_path);
                    }
                }
            }

            if (file_exists(jar_path)) {
                log_msg("info", "Extracting natives from %s\n", e->path);
                if (extract_natives(jar_path, natives_dir)) count++;
            } else {
                log_msg("error", "Native jar not found and cannot download: %s\n", e->path);
            }
        }
    }

    log_msg("succes", "Natives extracted (%d jars)\n", count);
    library_list_free(&libs);
    return 1;
}