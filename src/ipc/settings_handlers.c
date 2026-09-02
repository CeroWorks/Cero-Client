#include "../../include/ipc/settings_handlers.h"
#include "../../include/platform/paths.h"
#include "../../include/config/config.h"
#include "../../include/vendor/cJSON.h"
#include "../../include/ui/ui.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>

void on_get_settings(const char* id, const char* req, void* arg) {
    (void)req;
    char path[MAX_PATH_SIZE + 32];
    if (!build_settings_path(path, sizeof(path))) {
        ui_return(arg, id, 0, "null");
        return;
    }

    FILE* f = fopen(path, "r");
    if (!f) { ui_return(arg, id, 0, "null"); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size <= 0 || size > 64 * 1024) { fclose(f); ui_return(arg, id, 0, "null"); return; }

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); ui_return(arg, id, 0, "null"); return; }

    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);

    cJSON* json = cJSON_Parse(buf);
    if (!json) { free(buf); ui_return(arg, id, 0, "null"); return; }
    cJSON_Delete(json);

    ui_return(arg, id, 0, buf);
    free(buf);
}

void on_save_settings(const char* id, const char* req, void* arg) {
    const char* p = req;
    while (*p && *p != '{') p++;
    if (!*p) { ui_return(arg, id, 1, "\"parse_error\""); return; }

    cJSON* json = cJSON_Parse(p);
    if (!json) { ui_return(arg, id, 1, "\"invalid_json\""); return; }

    char path[MAX_PATH_SIZE + 32];
    if (!build_settings_path(path, sizeof(path))) {
        cJSON_Delete(json);
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    FILE* f = fopen(path, "r");
    cJSON* existing = NULL;
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);
        if (size > 0 && size < 64 * 1024) {
            char* buf = malloc(size + 1);
            if (buf) {
                size_t n = fread(buf, 1, size, f);
                buf[n] = '\0';
                existing = cJSON_Parse(buf);
                free(buf);
            }
        }
        fclose(f);
    }
    if (!existing) existing = cJSON_CreateObject();

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, json) {
        cJSON* clone = cJSON_Duplicate(item, 1);
        cJSON_DeleteItemFromObject(existing, item->string);
        cJSON_AddItemToObject(existing, item->string, clone);
    }
    cJSON_Delete(json);

    char* out = cJSON_PrintUnformatted(existing);
    cJSON_Delete(existing);
    if (!out) { ui_return(arg, id, 1, "\"alloc_error\""); return; }

    f = fopen(path, "w");
    if (!f) { free(out); ui_return(arg, id, 1, "\"write_error\""); return; }
    fputs(out, f);
    fclose(f);
    free(out);

    ui_return(arg, id, 0, "\"ok\"");
}

void on_get_version(const char* id, const char* req, void* arg) {
    (void)req;
    char* version = config_get("lastVersion");
    if (!version) {
        log_msg("debug", "No version saved\n");
        ui_return(arg, id, 0, "null");
        return;
    }
    log_msg("debug", "Last version: %s\n", version);
    ui_return(arg, id, 0, version);
    free(version);
}

void on_set_version(const char* id, const char* req, void* arg) {
    char version[64] = "";
    const char* p = req;
    while (*p && *p != '"') p++;
    if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof(version))
            version[i++] = *p++;
        version[i] = '\0';
    }

    if (version[0] == '\0') {
        ui_return(arg, id, 1, "\"no_version\"");
        return;
    }

    log_msg("debug", "Version set: %s\n", version);

    char json_val[80];
    snprintf(json_val, sizeof(json_val), "\"%s\"", version);
    config_set("lastVersion", json_val);

    ui_return(arg, id, 0, "\"ok\"");
}
