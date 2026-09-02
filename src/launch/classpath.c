#include "../../include/launch/classpath.h"
#include "../../include/net/download.h"
#include "../../include/utils/file_utils.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64)
  #define CURRENT_ARCH "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define CURRENT_ARCH "aarch_64"
#elif defined(__i386__) || defined(_M_IX86)
  #define CURRENT_ARCH "x86"
#else
  #define CURRENT_ARCH "unknown"
#endif

int parse_maven_gav(const char* name, CpLib* out) {
    const char* p1 = strchr(name, ':');
    if (!p1) return 0;
    const char* p2 = strchr(p1 + 1, ':');
    if (!p2) return 0;
    size_t gl = (size_t)(p1 - name);
    size_t al = (size_t)(p2 - (p1 + 1));
    if (gl >= sizeof(out->group) || al >= sizeof(out->artifact)) return 0;
    memcpy(out->group, name, gl); out->group[gl] = '\0';
    memcpy(out->artifact, p1 + 1, al); out->artifact[al] = '\0';
    snprintf(out->version, sizeof(out->version), "%s", p2 + 1);
    return 1;
}

void cp_add(CpLib* arr, int* count, int cap, const CpLib* lib) {
    for (int i = 0; i < *count; i++) {
        if (strcmp(arr[i].group, lib->group) == 0 &&
            strcmp(arr[i].artifact, lib->artifact) == 0) {

            char base_existing[64] = "";
            char base_new[64] = "";

            const char* ev = strchr(arr[i].version, ':');
            snprintf(base_existing, sizeof(base_existing), "%.*s",
                     ev ? (int)(ev - arr[i].version) : (int)strlen(arr[i].version),
                     arr[i].version);

            const char* nv = strchr(lib->version, ':');
            snprintf(base_new, sizeof(base_new), "%.*s",
                     nv ? (int)(nv - lib->version) : (int)strlen(lib->version),
                     lib->version);

            if (strcmp(base_existing, base_new) != 0) {
                log_msg("info",
                    "Lib conflict %s:%s -> keeping %s, dropping %s\n",
                    lib->group, lib->artifact, arr[i].version, lib->version);
                return;
            }

            break;
        }
    }

    if (*count >= cap) return;
    arr[(*count)++] = *lib;
}

void collect_fabric_libs(const char* client_dir, VmJVal* fabric_json,
                         CpLib* arr, int* count, int cap) {
    VmJVal* libs = vm_get(fabric_json, "libraries");
    if (!libs || libs->t != VM_JARR) return;

    for (int i = 0; i < libs->a.count; i++) {
        VmJVal* lib = libs->a.items[i];
        const char* name = vm_gets(lib, "name");
        const char* base = vm_gets(lib, "url");
        if (!name) continue;
        if (!base) base = "https://maven.fabricmc.net/";

        CpLib c; memset(&c, 0, sizeof(c));
        if (!parse_maven_gav(name, &c)) continue;

        char group_path[256];
        snprintf(group_path, sizeof(group_path), "%s", c.group);
        for (char* q = group_path; *q; q++) if (*q == '.') *q = '/';

        char relpath[512];
        snprintf(relpath, sizeof(relpath), "%s/%s/%s/%s-%s.jar",
                 group_path, c.artifact, c.version, c.artifact, c.version);

        snprintf(c.path, sizeof(c.path), "%s/libraries/%s", client_dir, relpath);

        const char* sep = (base[strlen(base) - 1] == '/') ? "" : "/";
        snprintf(c.url, sizeof(c.url), "%s%s%s", base, sep, relpath);
        c.needs_download = 1;

        cp_add(arr, count, cap, &c);
    }
}

static int rules_allow_current_os(VmJVal* lib) {
    VmJVal* rules = vm_get(lib, "rules");
    if (!rules || rules->t != VM_JARR) return 1;

    int allowed = 0;
    for (int i = 0; i < rules->a.count; i++) {
        VmJVal* rule = rules->a.items[i];
        const char* action = vm_gets(rule, "action");
        VmJVal* os = vm_get(rule, "os");
        const char* os_name = vm_gets(os, "name");
        const char* os_arch = vm_gets(os, "arch");

        int matches_os = 1;
        if (os_name) {
#if defined(_WIN32)
            matches_os = (strcmp(os_name, "windows") == 0);
#elif defined(__APPLE__)
            matches_os = (strcmp(os_name, "osx") == 0);
#else
            matches_os = (strcmp(os_name, "linux") == 0);
#endif
        }

        if (matches_os && os_arch) {
            if (strcmp(os_arch, CURRENT_ARCH) != 0) {
                matches_os = 0;
            }
        }

        if (matches_os) {
            allowed = (action && strcmp(action, "allow") == 0);
        }
    }
    return allowed;
}

void collect_vanilla_libs(const char* client_dir, VmJVal* version_json,
                          CpLib* arr, int* count, int cap) {
    VmJVal* libs = vm_get(version_json, "libraries");
    if (!libs || libs->t != VM_JARR) return;

    for (int i = 0; i < libs->a.count; i++) {
        VmJVal* lib = libs->a.items[i];
        if (!rules_allow_current_os(lib)) continue;
        const char* name = vm_gets(lib, "name");
        if (!name) continue;

        VmJVal* downloads = vm_get(lib, "downloads");
        VmJVal* artifact  = downloads ? vm_get(downloads, "artifact") : NULL;
        const char* path  = artifact ? vm_gets(artifact, "path") : NULL;

        CpLib c; memset(&c, 0, sizeof(c));
        if (!parse_maven_gav(name, &c)) continue;

        if (path) {
            snprintf(c.path, sizeof(c.path), "%s/libraries/%s", client_dir, path);
            c.needs_download = 0;
        } else {
            char group_path[256];
            snprintf(group_path, sizeof(group_path), "%s", c.group);
            for (char* q = group_path; *q; q++) if (*q == '.') *q = '/';

            char relpath[512];
            snprintf(relpath, sizeof(relpath), "%s/%s/%s/%s-%s.jar",
                     group_path, c.artifact, c.version, c.artifact, c.version);

            snprintf(c.path, sizeof(c.path), "%s/libraries/%s", client_dir, relpath);
            snprintf(c.url, sizeof(c.url), "https://repo1.maven.org/maven2/%s", relpath);
            c.needs_download = 1;
        }

        cp_add(arr, count, cap, &c);
    }
}

void download_pending_libraries(const CpLib* arr, int count) {
    for (int i = 0; i < count; i++) {
        if (!arr[i].needs_download) continue;
        FILE* f = fopen(arr[i].path, "rb");
        if (f) { fclose(f); continue; }
        ensure_parent_dirs(arr[i].path);
        log_msg("info", "Downloading Fabric lib: %s\n", arr[i].url);
        download_file(arr[i].url, arr[i].path);
    }
}
