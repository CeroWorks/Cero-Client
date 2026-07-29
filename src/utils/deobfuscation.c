
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../include/logger.h"
#include "../../include/utils/file_utils.h"
#include "../../include/utils/version_manifest.h"
#include "../../include/download.h"
#include "../../include/utils/process.h"
#include "../../include/utils/deobfuscation.h"



static int version_ge(const char* v, int maj, int min, int patch) {
    int a = 0, b = 0, c = 0;
    sscanf(v, "%d.%d.%d", &a, &b, &c);
    if (a != maj) return a > maj;
    if (b != min) return b > min;
    return c >= patch;
}



typedef struct {
    const char* version;
    const char* mcp_config_url;
} McpConfigEntry;

static const McpConfigEntry MCP_CONFIGS[] = {
    { "1.7.10", "https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp_config/1.7.10/mcp_config-1.7.10.zip" },
    { "1.8.9",  "https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp_config/1.8.9/mcp_config-1.8.9.zip" },
    { "1.12.2", "https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp_config/1.12.2/mcp_config-1.12.2.zip" },
};
static const size_t MCP_CONFIGS_COUNT = sizeof(MCP_CONFIGS) / sizeof(MCP_CONFIGS[0]);

static const char* find_mcp_config_url(const char* version) {
    for (size_t i = 0; i < MCP_CONFIGS_COUNT; i++) {
        if (strcmp(MCP_CONFIGS[i].version, version) == 0) {
            return MCP_CONFIGS[i].mcp_config_url;
        }
    }
    return NULL;
}


int version_needs_deobfuscation(const char* version) {
    int year = 0, drop = 0;
    if (sscanf(version, "%d.%d", &year, &drop) == 2 && year >= 26) {
        return 0; 
    }
    if (find_mcp_config_url(version) != NULL) {
        return 2; 
    }
    if (!version_ge(version, 1, 14, 4)) {
        return 0; 
    }
    return 1; 
}



static int extract_mappings_url(const char* json_path, char* out_url, size_t out_sz) {
    VmJVal* root = vm_load_json(json_path);
    if (!root) return 0;

    VmJVal* downloads = vm_get(root, "downloads");
    VmJVal* mappings = downloads ? vm_get(downloads, "client_mappings") : NULL;
    const char* url = mappings ? vm_gets(mappings, "url") : NULL;

    int ok = 0;
    if (url) {
        snprintf(out_url, out_sz, "%s", url);
        ok = 1;
    }
    vm_free(root);
    return ok;
}

static int deobfuscate_client_jar_mojang(const char* client_dir, const char* version,
                                          const char* input_jar, const char* output_jar,
                                          const char* json_path, const char* java_exe,
                                          const char* cero_jar_path) {

    char mapping_url[1024];
    if (!extract_mappings_url(json_path, mapping_url, sizeof(mapping_url))) {
        log_msg("error", "Pas de client_mappings disponible pour %s\n", version);
        return 0;
    }

    char proguard_path[MAX_PATH_SIZE];
    snprintf(proguard_path, sizeof(proguard_path),
             "%s/mappings/%s.proguard.txt", client_dir, version);
    ensure_parent_dirs(proguard_path);

    if (access(proguard_path, F_OK) != 0) {
        log_msg("info", "Téléchargement du mapping officiel pour %s...\n", version);
        if (download_file(mapping_url, proguard_path) == 0) {
            log_msg("error", "Échec du téléchargement du mapping (%s)\n", mapping_url);
            return 0;
        }
    }

    char tiny_path[MAX_PATH_SIZE];
    snprintf(tiny_path, sizeof(tiny_path),
             "%s/mappings/%s.tiny", client_dir, version);

    if (access(tiny_path, F_OK) != 0) {
        log_msg("info", "Conversion ProGuard -> tiny pour %s...\n", version);

        const char* conv_argv[8];
        int i = 0;
        conv_argv[i++] = java_exe;
        conv_argv[i++] = "-cp";
        conv_argv[i++] = cero_jar_path;
        conv_argv[i++] = "fr.cerostudio.remap.ProguardToTiny";
        conv_argv[i++] = proguard_path;
        conv_argv[i++] = tiny_path;
        conv_argv[i++] = NULL;

        if (process_run(java_exe, conv_argv) != 0) {
            log_msg("error", "Échec de la conversion ProGuard->tiny pour %s\n", version);
            return 0;
        }
    }

    log_msg("info", "Remapping du jar pour %s (Mojang)...\n", version);

    const char* remap_argv[8];
    int i = 0;
    remap_argv[i++] = java_exe;
    remap_argv[i++] = "-cp";
    remap_argv[i++] = cero_jar_path;
    remap_argv[i++] = "fr.cerostudio.remap.JarRemapper";
    remap_argv[i++] = input_jar;
    remap_argv[i++] = output_jar;
    remap_argv[i++] = tiny_path;
    remap_argv[i++] = NULL;

    if (process_run(java_exe, remap_argv) != 0) {
        log_msg("error", "Échec du remapping pour %s\n", version);
        return 0;
    }

    log_msg("succes", "Jar déobfusqué prêt (Mojang): %s\n", output_jar);
    return 1;
}



static int extract_mcp_config_zip(const char* zip_path, const char* dest_dir,
                                    const char* java_exe, const char* cero_jar_path) {
    
    const char* extract_argv[8];
    int i = 0;
    extract_argv[i++] = java_exe;
    extract_argv[i++] = "-cp";
    extract_argv[i++] = cero_jar_path;
    extract_argv[i++] = "fr.cerostudio.remap.McpConfigExtractor";
    extract_argv[i++] = zip_path;
    extract_argv[i++] = dest_dir;
    extract_argv[i++] = NULL;

    return process_run(java_exe, extract_argv) == 0;
}

static int deobfuscate_client_jar_mcp(const char* client_dir, const char* version,
                                       const char* input_jar, const char* output_jar,
                                       const char* java_exe, const char* cero_jar_path) {

    const char* mcp_url = find_mcp_config_url(version);
    if (!mcp_url) {
        log_msg("error", "Pas de mcp_config disponible pour %s\n", version);
        return 0;
    }

    char zip_path[MAX_PATH_SIZE];
    snprintf(zip_path, sizeof(zip_path),
             "%s/mappings/%s_mcp_config.zip", client_dir, version);
    ensure_parent_dirs(zip_path);

    if (access(zip_path, F_OK) != 0) {
        log_msg("info", "Téléchargement du mcp_config pour %s...\n", version);
        if (download_file(mcp_url, zip_path) == 0) {
            log_msg("error", "Échec du téléchargement du mcp_config (%s)\n", mcp_url);
            return 0;
        }
    }

    char mcp_dir[MAX_PATH_SIZE];
    snprintf(mcp_dir, sizeof(mcp_dir),
             "%s/mappings/%s_mcp", client_dir, version);

    char tiny_path[MAX_PATH_SIZE];
    snprintf(tiny_path, sizeof(tiny_path),
             "%s/mappings/%s.tiny", client_dir, version);

    if (access(tiny_path, F_OK) != 0) {
        log_msg("info", "Extraction du mcp_config pour %s...\n", version);
        ensure_parent_dirs(mcp_dir);
        if (!extract_mcp_config_zip(zip_path, mcp_dir, java_exe, cero_jar_path)) {
            log_msg("error", "Échec de l'extraction du mcp_config pour %s\n", version);
            return 0;
        }

        log_msg("info", "Conversion TSRG+CSV -> tiny pour %s...\n", version);
        const char* conv_argv[8];
        int i = 0;
        conv_argv[i++] = java_exe;
        conv_argv[i++] = "-cp";
        conv_argv[i++] = cero_jar_path;
        conv_argv[i++] = "fr.cerostudio.remap.McpToTiny";
        conv_argv[i++] = mcp_dir;
        conv_argv[i++] = input_jar;
        conv_argv[i++] = tiny_path;
        conv_argv[i++] = NULL;

        if (process_run(java_exe, conv_argv) != 0) {
            log_msg("error", "Échec de la conversion MCP->tiny pour %s\n", version);
            return 0;
        }
    }

    log_msg("info", "Remapping du jar pour %s (MCP)...\n", version);
    const char* remap_argv[8];
    int i = 0;
    remap_argv[i++] = java_exe;
    remap_argv[i++] = "-cp";
    remap_argv[i++] = cero_jar_path;
    remap_argv[i++] = "fr.cerostudio.remap.JarRemapper";
    remap_argv[i++] = input_jar;
    remap_argv[i++] = output_jar;
    remap_argv[i++] = tiny_path;
    remap_argv[i++] = NULL;

    if (process_run(java_exe, remap_argv) != 0) {
        log_msg("error", "Échec du remapping pour %s\n", version);
        return 0;
    }

    log_msg("succes", "Jar déobfusqué prêt (MCP): %s\n", output_jar);
    return 1;
}



int deobfuscate_client_jar(const char* client_dir, const char* version,
                            const char* input_jar, const char* output_jar,
                            const char* json_path, const char* java_exe,
                            const char* cero_jar_path) {

    int needs = version_needs_deobfuscation(version);

    if (needs == 0) {
        log_msg("debug", "Aucune déobfuscation nécessaire pour %s\n", version);
        return 1;
    }

    if (needs == 2) {
        return deobfuscate_client_jar_mcp(client_dir, version, input_jar, output_jar,
                                           java_exe, cero_jar_path);
    }

    
    return deobfuscate_client_jar_mojang(client_dir, version, input_jar, output_jar,
                                          json_path, java_exe, cero_jar_path);
}