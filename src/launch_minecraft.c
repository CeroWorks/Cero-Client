#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/logger.h"
#include "../include/manifest.h"
#include "../include/utils/file_utils.h"
#include "../include/download.h"
#include "../include/config.h"
#include "../include/utils/version_manifest.h"
#include "../include/sha1.h"
#include "../include/utils/libraries.h"
#include "../include/utils/assets.h"
#include "../include/java.h"
#include "../include/utils/process.h"
#include "../include/ms_auth.h"
#include "../include/launch_minecraft.h"
#include "../include/ui.h"
#include "../include/discord_rpc.h"
#include "../include/assets_loader.h"
#include <unistd.h>
#include "../include/utils/deobfuscation.h"
#include <time.h>

#define CLIENT_BRAND "ceroclient"
#define FABRIC_META  "https://meta.fabricmc.net/v2"

#if defined(_WIN32)
  #define CP_SEP ";"
  #include <windows.h>
#else
  #define CP_SEP ":"
#endif

#if defined(__x86_64__) || defined(_M_X64)
  #define CURRENT_ARCH "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define CURRENT_ARCH "aarch_64"
#elif defined(__i386__) || defined(_M_IX86)
  #define CURRENT_ARCH "x86"
#else
  #define CURRENT_ARCH "unknown"
#endif



static const char* json_str(const char* json, const char* key,
                            char* out, size_t outsz) {
    out[0] = '\0';
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char* p = strstr(json, needle);
    if (!p) return NULL;
    p += strlen(needle);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p != '"') return NULL;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < outsz) out[i++] = *p++;
    out[i] = '\0';
    return out;
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}


static int parse_fabric_spec(const char* spec, char* mc, size_t mcsz,
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


static int fetch_fabric_profile(const char* client_dir,
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



typedef struct {
    char group[256];     
    char artifact[128];
    char version[64];
    char path[MAX_PATH_SIZE];
    char url[1024];
    int  needs_download; 
} CpLib;

static int parse_maven_gav(const char* name, CpLib* out) {
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

static void cp_add(CpLib* arr, int* count, int cap, const CpLib* lib) {
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

static void collect_fabric_libs(const char* client_dir, VmJVal* fabric_json,
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

static void collect_vanilla_libs(const char* client_dir, VmJVal* version_json,
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



void launch_minecraft(const char* version,
                      launch_progress_cb cb, void* userdata) {

#define PROGRESS(step, pct) do { \
    log_msg("info", "%s\n", step); \
    if (cb) cb(step, pct, userdata); \
} while(0)

    char client_dir[MAX_PATH_SIZE];
    snprintf(client_dir, sizeof(client_dir), "%s", client_path);

    
    char mc_version[64] = "";
    char fabric_loader[64] = "";
    int is_fabric = parse_fabric_spec(version, mc_version, sizeof(mc_version),
                                      fabric_loader, sizeof(fabric_loader));
    const char* vanilla_version = is_fabric ? mc_version : version;

    PROGRESS("Chargement du compte...", 5);

    char account_path[MAX_PATH_SIZE];
    snprintf(account_path, sizeof(account_path),
             "%s/account.json", client_dir);

    if (ms_auth_validate(account_path) != 0) {
        PROGRESS("Rafraîchissement du token...", 8);
        if (ms_auth_refresh(account_path) != 0) {
            PROGRESS("Erreur : compte invalide !", -1);
            log_msg("error", "Account invalid / refresh failed\n");
            return;
        }
    }

    char* acct_json = read_file(account_path);
    char username[128] = "Player";
    char uuid[64]      = "00000000-0000-0000-0000-000000000000";
    char token[2048]   = "0";

    if (acct_json) {
        json_str(acct_json, "name",     username, sizeof(username));
        json_str(acct_json, "uuid",     uuid,     sizeof(uuid));
        json_str(acct_json, "mc_token", token,    sizeof(token));
        free(acct_json);
    }

    PROGRESS("Récupération du manifest...", 12);
    ensure_directory_exists(client_dir);
    download_manifest();

    VersionInfo vi;
    if (!manifest_find_version(client_dir, vanilla_version, &vi)) {
        PROGRESS("Erreur : version introuvable !", -1);
        log_msg("error", "Version %s not found\n", vanilla_version);
        return;
    }

    PROGRESS("Téléchargement des métadonnées...", 18);
    char json_dest[MAX_PATH_SIZE];
    snprintf(json_dest, sizeof(json_dest),
             "%s/versions/%s/%s.json", client_dir, vi.id, vi.id);
    download_file(vi.url, json_dest);
    version_info_free(&vi);

    PROGRESS("Téléchargement du client.jar...", 28);
    DownloadEntry client_entry;
    if (!version_get_client_jar(client_dir, vanilla_version, &client_entry)) {
        PROGRESS("Erreur : impossible d'obtenir client.jar !", -1);
        log_msg("error", "Cannot get client jar info\n");
        return;
    }
    char jar_dest[MAX_PATH_SIZE];
    snprintf(jar_dest, sizeof(jar_dest),
             "%s/versions/%s/%s.jar", client_dir, vanilla_version, vanilla_version);
    download_file(client_entry.url, jar_dest);

    if (!sha1_file_matches(jar_dest, client_entry.sha1)) {
        PROGRESS("Erreur : client.jar corrompu ou falsifié (SHA-1 invalide) !", -1);
        log_msg("error", "client.jar SHA-1 mismatch for %s (attendu %s) — fichier supprimé\n",
            vanilla_version, client_entry.sha1 ? client_entry.sha1 : "(absent)");
        remove(jar_dest);
        return;
    }

    
    char mapped_jar[MAX_PATH_SIZE];
    mapped_jar[0] = '\0';

    if (version_needs_deobfuscation(vanilla_version)) {
        PROGRESS("Déobfuscation du client...", 34);
        snprintf(mapped_jar, sizeof(mapped_jar),
                 "%s/versions/%s/%s.mapped.jar", client_dir, vanilla_version, vanilla_version);

        if (access(mapped_jar, F_OK) != 0) {
            
            if (!java_ensure(client_dir, vanilla_version)) {
                PROGRESS("Erreur : Java introuvable pour la déobfuscation !", -1);
                return;
            }
            int dmajor = java_get_required_version(client_dir, vanilla_version);
            char djava_exe[MAX_PATH_SIZE];
            if (!java_resolve_executable(client_dir, dmajor, djava_exe, sizeof(djava_exe))) {
                PROGRESS("Erreur : exécutable Java introuvable !", -1);
                return;
            }

            const char* extract_dir = client_path;
            char cero_jar_for_remap[MAX_PATH_SIZE];
            snprintf(cero_jar_for_remap, sizeof(cero_jar_for_remap),
                     "%s/agent/CeroClient-MC.jar", extract_dir);

            if (!deobfuscate_client_jar(client_dir, vanilla_version,
                                         jar_dest, mapped_jar, json_dest,
                                         djava_exe, cero_jar_for_remap)) {
                PROGRESS("Erreur : échec de la déobfuscation !", -1);
                mapped_jar[0] = '\0'; 
            }
        } else {
            log_msg("info", "Jar déobfusqué en cache: %s\n", mapped_jar);
        }
    }

    PROGRESS("Téléchargement des librairies...", 42);
    download_libraries(client_dir, vanilla_version);

    PROGRESS("Extraction des natives...", 55);
    extract_all_natives(client_dir, vanilla_version);

    PROGRESS("Téléchargement des assets...", 68);
    download_assets(client_dir, vanilla_version);

    PROGRESS("Vérification de Java...", 80);
    if (!java_ensure(client_dir, vanilla_version)) {
        PROGRESS("Erreur : Java introuvable !", -1);
        log_msg("error", "java_ensure failed for %s\n", vanilla_version);
        return;
    }

    int major = java_get_required_version(client_dir, vanilla_version);
    char java_exe[MAX_PATH_SIZE];
    if (!java_resolve_executable(client_dir, major, java_exe, sizeof(java_exe))) {
        PROGRESS("Erreur : exécutable Java introuvable !", -1);
        log_msg("error",
            "Java %d executable introuvable. Sur FreeBSD : pkg install openjdk%d\n",
            major, major);
        return;
    }
    log_msg("info", "Using Java executable: %s\n", java_exe);

    PROGRESS("Préparation du lancement...", 90);
    VmJVal* root = vm_load_json(json_dest);
    if (!root) {
        PROGRESS("Erreur : lecture version JSON !", -1);
        log_msg("error", "Cannot read version json for launch\n");
        return;
    }

    
    VmJVal* fabric_root = NULL;
    char fabric_id[128] = "";
    char fabric_main[128] = "";

    if (is_fabric) {
        PROGRESS("Téléchargement du profil Fabric...", 92);
        char fabric_json_path[MAX_PATH_SIZE];
        if (!fetch_fabric_profile(client_dir, mc_version, fabric_loader,
                                  fabric_id, sizeof(fabric_id),
                                  fabric_json_path, sizeof(fabric_json_path))) {
            PROGRESS("Erreur : profil Fabric introuvable !", -1);
            log_msg("error", "Cannot fetch Fabric profile for %s\n", mc_version);
            vm_free(root);
            return;
        }
        fabric_root = vm_load_json(fabric_json_path);
        if (!fabric_root) {
            PROGRESS("Erreur : lecture profil Fabric !", -1);
            vm_free(root);
            return;
        }
        const char* fmc = vm_gets(fabric_root, "mainClass");
        if (fmc) snprintf(fabric_main, sizeof(fabric_main), "%s", fmc);
    }

    const char* game_main_class = (fabric_main[0]) ? fabric_main
                                                   : vm_gets(root, "mainClass");
    const char* asset_index = NULL;
    VmJVal* ai = vm_get(root, "assetIndex");
    if (ai) asset_index = vm_gets(ai, "id");
    if (!asset_index) asset_index = vm_gets(root, "assets");
    if (!asset_index) asset_index = "legacy";

    if (!game_main_class) {
        PROGRESS("Erreur : mainClass manquant !", -1);
        log_msg("error", "No mainClass in version json\n");
        if (fabric_root) vm_free(fabric_root);
        vm_free(root);
        return;
    }

    
    char cero_jar_path[MAX_PATH_SIZE];
    cero_jar_path[0] = '\0';
    int has_cero = 0;
    const uint8_t* agent_data = NULL;
    size_t agent_size = 0;
    if (assets_get_file("agent/CeroClient-MC.jar", &agent_data, &agent_size) && agent_data && agent_size > 0) {
        snprintf(cero_jar_path, sizeof(cero_jar_path), "%s/agent/CeroClient-MC.jar", client_dir);
        ensure_parent_dirs(cero_jar_path);
        FILE* f = fopen(cero_jar_path, "wb");
        if (f) {
            fwrite(agent_data, 1, agent_size, f);
            fclose(f);
            has_cero = 1;
            log_msg("info", "CeroClient agent extracted to: %s\n", cero_jar_path);
        } else {
            log_msg("warn", "Failed to write CeroClient agent to disk!\n");
        }
        assets_free_buffer(agent_data);
    } else {
        log_msg("warn", "CeroClient-MC.jar not found in RAM assets. Launching vanilla.\n");
    }

    
    static CpLib libs_arr[512];
    int libs_count = 0;

    if (fabric_root) {
        PROGRESS("Téléchargement des librairies Fabric...", 95);
        collect_fabric_libs(client_dir, fabric_root,
                            libs_arr, &libs_count, 512);
    }
    collect_vanilla_libs(client_dir, root,
                         libs_arr, &libs_count, 512);

    
    for (int i = 0; i < libs_count; i++) {
        if (!libs_arr[i].needs_download) continue;
        FILE* f = fopen(libs_arr[i].path, "rb");
        if (f) { fclose(f); continue; }
        ensure_parent_dirs(libs_arr[i].path);
        log_msg("info", "Downloading Fabric lib: %s\n", libs_arr[i].url);
        download_file(libs_arr[i].url, libs_arr[i].path);
    }

    
    static char classpath[65536];
    classpath[0] = '\0';
    size_t cplen = 0;

    
    if (has_cero) {
        int wn = snprintf(classpath + cplen, sizeof(classpath) - cplen,
                          "%s%s", cero_jar_path, CP_SEP);
        if (wn > 0 && (size_t)wn < sizeof(classpath) - cplen) {
            cplen += (size_t)wn;
        }
    }

    for (int i = 0; i < libs_count; i++) {
        if (cplen >= sizeof(classpath)) break;
        int wn = snprintf(classpath + cplen, sizeof(classpath) - cplen,
                          "%s%s", libs_arr[i].path, CP_SEP);
        if (wn > 0) {
            if ((size_t)wn >= sizeof(classpath) - cplen) {
                log_msg("error", "Classpath buffer overflow!\n");
                cplen = sizeof(classpath) - 1;
                break;
            }
            cplen += (size_t)wn;
        }
    }

    
    char client_jar[MAX_PATH_SIZE];
    if (mapped_jar[0]) {
        snprintf(client_jar, sizeof(client_jar), "%s", mapped_jar);
    } else {
        snprintf(client_jar, sizeof(client_jar),
                 "%s/versions/%s/%s.jar", client_dir, vanilla_version, vanilla_version);
    }

    snprintf(classpath + cplen, sizeof(classpath) - cplen, "%s", client_jar);

    char natives_dir[MAX_PATH_SIZE];
    snprintf(natives_dir, sizeof(natives_dir),
             "%s/versions/%s/natives", client_dir, vanilla_version);

    char assets_dir_path[MAX_PATH_SIZE];
    snprintf(assets_dir_path, sizeof(assets_dir_path),
             "%s/assets", client_dir);

    char arg_libpath[MAX_PATH_SIZE + 32];
    snprintf(arg_libpath, sizeof(arg_libpath),
             "-Djava.library.path=%s", natives_dir);

    char arg_launcher_brand[64];
    snprintf(arg_launcher_brand, sizeof(arg_launcher_brand),
             "-Dminecraft.launcher.brand=%s", CLIENT_BRAND);

    char arg_client_brand[64];
    snprintf(arg_client_brand, sizeof(arg_client_brand),
             "-Dminecraft.client.brand=%s", CLIENT_BRAND);

    char asset_index_buf[64];
    snprintf(asset_index_buf, sizeof(asset_index_buf), "%s", asset_index);

    const char* version_id_for_args = is_fabric ? fabric_id : vanilla_version;

    const char* argv[96];
    int n = 0;
    argv[n++] = java_exe;

    argv[n++] = "-Xms2G";
    argv[n++] = "-Xmx6G";

    argv[n++] = "-XX:+UnlockExperimentalVMOptions";
    argv[n++] = "-XX:+UseG1GC";
    argv[n++] = "-XX:+ParallelRefProcEnabled";
    argv[n++] = "-XX:MaxGCPauseMillis=200";
    argv[n++] = "-XX:+DisableExplicitGC";
    argv[n++] = "-XX:+AlwaysPreTouch";
    argv[n++] = "-XX:G1NewSizePercent=30";
    argv[n++] = "-XX:G1MaxNewSizePercent=40";
    argv[n++] = "-XX:G1HeapRegionSize=8M";
    argv[n++] = "-XX:G1ReservePercent=20";
    argv[n++] = "-XX:G1HeapWastePercent=5";
    argv[n++] = "-XX:G1MixedGCCountTarget=4";
    argv[n++] = "-XX:InitiatingHeapOccupancyPercent=15";
    argv[n++] = "-XX:G1MixedGCLiveThresholdPercent=90";
    argv[n++] = "-XX:G1RSetUpdatingPauseTimePercent=5";
    argv[n++] = "-XX:SurvivorRatio=32";
    argv[n++] = "-XX:+PerfDisableSharedMem";
    argv[n++] = "-XX:MaxTenuringThreshold=1";

    argv[n++] = "-Dfile.encoding=UTF-8";
    argv[n++] = "-Dstdout.encoding=UTF-8";
    argv[n++] = "-Dstderr.encoding=UTF-8";

    argv[n++] = arg_libpath;
    argv[n++] = arg_launcher_brand;
    argv[n++] = "-Dminecraft.launcher.version=1.0";
    argv[n++] = arg_client_brand;

    argv[n++] = "-cp";
    argv[n++] = classpath;

    if (has_cero) {
        argv[n++] = "fr.cerostudio.Main";
        log_msg("info", "Using CeroLoader as main class\n");
    } else {
        argv[n++] = (char*)game_main_class;
        log_msg("info", "Using vanilla main class: %s\n", game_main_class);
    }

    if (has_cero) {
        argv[n++] = "--realMainClass";
        argv[n++] = (char*)game_main_class;
    }

    argv[n++] = "--username";    argv[n++] = username;
    argv[n++] = "--version";     argv[n++] = (char*)version_id_for_args;
    argv[n++] = "--gameDir";     argv[n++] = client_dir;
    argv[n++] = "--assetsDir";   argv[n++] = assets_dir_path;
    argv[n++] = "--assetIndex";  argv[n++] = asset_index_buf;
    argv[n++] = "--uuid";        argv[n++] = uuid;
    argv[n++] = "--accessToken"; argv[n++] = token;
    argv[n++] = "--userProperties";argv[n++] = "{}";
    argv[n++] = "--userType";    argv[n++] = "msa";
    argv[n++] = "--versionType"; argv[n++] = "release";
    argv[n++] = "--ceroMcVersion"; argv[n++] = vanilla_version;
    
    char bridge_port_str[16];
    snprintf(bridge_port_str, sizeof(bridge_port_str), "%d", local_bridge_port);
    argv[n++] = "--ceroPort";
    argv[n++] = bridge_port_str;

    argv[n] = NULL;

    PROGRESS("Lancement !", 100);
    log_msg("info", "Launching Minecraft %s as %s%s\n",
            version, username, is_fabric ? " (Fabric)" : "");

    LaunchUserdata* ud = (LaunchUserdata*)userdata;
    if (ud && ud->ui && ud->game_running) {
        *ud->game_running = 1;
        ui_eval(ud->ui, "window._onGameStart && window._onGameStart()");
    }

    {
        char details[128];
        snprintf(details, sizeof(details), "Playing %s", version);
        discord_rpc_update(NULL, details, "logo", "CeroClient",
                           NULL, NULL, (int64_t)time(NULL));
    }

    int rc = process_run(java_exe, argv);

    if (ud && ud->ui) {
        if (ud->game_running) *ud->game_running = 0;
        ui_eval(ud->ui, "window._onGameStop && window._onGameStop()");
    }


    discord_rpc_update(NULL, "Idling in menu", "logo", "CeroClient",
                       NULL, NULL, (int64_t)time(NULL));

    if (rc != 0) log_msg("error", "Game exited with code %d\n", rc);
    else         log_msg("succes", "Game exited cleanly\n");

    if (fabric_root) vm_free(fabric_root);
    vm_free(root);

#undef PROGRESS
}