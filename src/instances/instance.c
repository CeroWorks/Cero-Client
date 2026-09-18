#include "../../include/instances/instance.h"
#include "../../include/vendor/cJSON.h"
#include "../../include/utils/file_utils.h"
#include "../../include/platform/paths.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

const char* loader_type_to_str(LoaderType t) {
    switch (t) {
        case LOADER_FABRIC:   return "fabric";
        case LOADER_FORGE:    return "forge";
        case LOADER_NEOFORGE: return "neoforge";
        case LOADER_QUILT:    return "quilt";
        default:              return "vanilla";
    }
}

LoaderType loader_type_from_str(const char* s) {
    if (!s) return LOADER_VANILLA;
    if (strcmp(s, "fabric")   == 0) return LOADER_FABRIC;
    if (strcmp(s, "forge")    == 0) return LOADER_FORGE;
    if (strcmp(s, "neoforge") == 0) return LOADER_NEOFORGE;
    if (strcmp(s, "quilt")    == 0) return LOADER_QUILT;
    return LOADER_VANILLA;
}

static void instances_root(char* out, size_t sz) {
    snprintf(out, sz, "%s%sinstances", client_path, PATH_SEP);
}

static void instances_index_path(char* out, size_t sz) {
    char root[MAX_PATH_SIZE];
    instances_root(root, sizeof(root));
    snprintf(out, sz, "%s%sinstances.json", root, PATH_SEP);
}

static char* read_whole_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    if (sz < 0 || sz > 16 * 1024 * 1024) { fclose(f); return NULL; }
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static int write_whole_file(const char* path, const char* data) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    fputs(data, f);
    fclose(f);
    return 1;
}

static cJSON* load_index(void) {
    char path[MAX_PATH_SIZE];
    instances_index_path(path, sizeof(path));

    char* txt = read_whole_file(path);
    cJSON* root = txt ? cJSON_Parse(txt) : NULL;
    free(txt);

    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        root = cJSON_CreateArray();
    }
    return root;
}

static int save_index(cJSON* root) {
    char path[MAX_PATH_SIZE];
    instances_index_path(path, sizeof(path));

    char* out = cJSON_PrintUnformatted(root);
    if (!out) return 0;
    int ok = write_whole_file(path, out);
    free(out);
    return ok;
}

static cJSON* instance_to_json(const Instance* inst) {
    cJSON* o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "id", inst->id);
    cJSON_AddStringToObject(o, "name", inst->name);
    cJSON_AddStringToObject(o, "mcVersion", inst->mc_version);
    cJSON_AddStringToObject(o, "loader", loader_type_to_str(inst->loader));
    cJSON_AddStringToObject(o, "loaderVersion", inst->loader_version);
    cJSON_AddNumberToObject(o, "ramMb", (double)inst->ram_mb);
    cJSON_AddNumberToObject(o, "createdAt", (double)inst->created_at);
    cJSON_AddNumberToObject(o, "lastPlayed", (double)inst->last_played);
    return o;
}

static void json_to_instance(cJSON* o, Instance* out) {
    memset(out, 0, sizeof(*out));
    cJSON* v;

    v = cJSON_GetObjectItem(o, "id");
    if (cJSON_IsString(v)) snprintf(out->id, sizeof(out->id), "%s", v->valuestring);

    v = cJSON_GetObjectItem(o, "name");
    if (cJSON_IsString(v)) snprintf(out->name, sizeof(out->name), "%s", v->valuestring);

    v = cJSON_GetObjectItem(o, "mcVersion");
    if (cJSON_IsString(v)) snprintf(out->mc_version, sizeof(out->mc_version), "%s", v->valuestring);

    v = cJSON_GetObjectItem(o, "loader");
    out->loader = cJSON_IsString(v) ? loader_type_from_str(v->valuestring) : LOADER_VANILLA;

    v = cJSON_GetObjectItem(o, "loaderVersion");
    if (cJSON_IsString(v)) snprintf(out->loader_version, sizeof(out->loader_version), "%s", v->valuestring);

    v = cJSON_GetObjectItem(o, "ramMb");
    out->ram_mb = cJSON_IsNumber(v) ? (long)v->valuedouble : 0;

    v = cJSON_GetObjectItem(o, "createdAt");
    out->created_at = cJSON_IsNumber(v) ? (long)v->valuedouble : 0;

    v = cJSON_GetObjectItem(o, "lastPlayed");
    out->last_played = cJSON_IsNumber(v) ? (long)v->valuedouble : 0;
}

int instances_init(void) {
    char root[MAX_PATH_SIZE];
    instances_root(root, sizeof(root));
    ensure_directory_exists(root);

    char path[MAX_PATH_SIZE];
    instances_index_path(path, sizeof(path));
    if (!file_exists(path)) {
        write_whole_file(path, "[]");
        log_msg("info", "Created instances index at %s\n", path);
    }
    return 1;
}

int instances_list_json(char* out, size_t out_sz) {
    cJSON* root = load_index();
    char* txt = cJSON_PrintUnformatted(root);
    if (!txt) { cJSON_Delete(root); return 0; }
    snprintf(out, out_sz, "%s", txt);
    free(txt);
    cJSON_Delete(root);
    return 1;
}

int instance_create(const char* name, const char* mc_version,
                    LoaderType loader, const char* loader_version,
                    char* out_id, size_t out_id_sz) {
    if (!name || !name[0] || !mc_version || !mc_version[0]) return 0;

    Instance inst;
    memset(&inst, 0, sizeof(inst));
    long now = (long)time(NULL);
    snprintf(inst.id, sizeof(inst.id), "%ld%04d", now, rand() % 10000);
    snprintf(inst.name, sizeof(inst.name), "%s", name);
    snprintf(inst.mc_version, sizeof(inst.mc_version), "%s", mc_version);
    inst.loader = loader;
    if (loader_version) snprintf(inst.loader_version, sizeof(inst.loader_version), "%s", loader_version);
    inst.created_at = now;
    inst.last_played = 0;
    inst.ram_mb = 0;

    cJSON* root = load_index();
    cJSON_AddItemToArray(root, instance_to_json(&inst));
    int ok = save_index(root);
    cJSON_Delete(root);

    if (ok) {
        char dir[MAX_PATH_SIZE];
        instance_get_dir(inst.id, dir, sizeof(dir));
        snprintf(out_id, out_id_sz, "%s", inst.id);
        log_msg("succes", "Created instance '%s' (%s) -> %s\n", inst.name, inst.id, dir);
    }
    return ok;
}

int instance_delete(const char* id) {
    cJSON* root = load_index();
    int idx = -1, i = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, root) {
        cJSON* jid = cJSON_GetObjectItem(item, "id");
        if (cJSON_IsString(jid) && strcmp(jid->valuestring, id) == 0) { idx = i; break; }
        i++;
    }
    if (idx < 0) { cJSON_Delete(root); return 0; }
    cJSON_DeleteItemFromArray(root, idx);
    int ok = save_index(root);
    cJSON_Delete(root);

    /* Note: the instance's on-disk directory (versions/mods/saves) is
     * intentionally left in place here; the frontend can offer a separate
     * "delete files too" confirmation before we ever rm -rf user data. */
    return ok;
}

int instance_rename(const char* id, const char* new_name) {
    if (!new_name || !new_name[0]) return 0;
    cJSON* root = load_index();
    int found = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, root) {
        cJSON* jid = cJSON_GetObjectItem(item, "id");
        if (cJSON_IsString(jid) && strcmp(jid->valuestring, id) == 0) {
            cJSON_DeleteItemFromObject(item, "name");
            cJSON_AddStringToObject(item, "name", new_name);
            found = 1;
            break;
        }
    }
    int ok = found && save_index(root);
    cJSON_Delete(root);
    return ok;
}

int instance_set_ram(const char* id, long ram_mb) {
    cJSON* root = load_index();
    int found = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, root) {
        cJSON* jid = cJSON_GetObjectItem(item, "id");
        if (cJSON_IsString(jid) && strcmp(jid->valuestring, id) == 0) {
            cJSON_DeleteItemFromObject(item, "ramMb");
            cJSON_AddNumberToObject(item, "ramMb", (double)ram_mb);
            found = 1;
            break;
        }
    }
    int ok = found && save_index(root);
    cJSON_Delete(root);
    return ok;
}

int instance_touch_last_played(const char* id) {
    cJSON* root = load_index();
    int found = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, root) {
        cJSON* jid = cJSON_GetObjectItem(item, "id");
        if (cJSON_IsString(jid) && strcmp(jid->valuestring, id) == 0) {
            cJSON_DeleteItemFromObject(item, "lastPlayed");
            cJSON_AddNumberToObject(item, "lastPlayed", (double)time(NULL));
            found = 1;
            break;
        }
    }
    int ok = found && save_index(root);
    cJSON_Delete(root);
    return ok;
}

int instance_get(const char* id, Instance* out) {
    cJSON* root = load_index();
    int found = 0;
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, root) {
        cJSON* jid = cJSON_GetObjectItem(item, "id");
        if (cJSON_IsString(jid) && strcmp(jid->valuestring, id) == 0) {
            json_to_instance(item, out);
            found = 1;
            break;
        }
    }
    cJSON_Delete(root);
    return found;
}

int instance_get_dir(const char* id, char* out, size_t out_sz) {
    char root[MAX_PATH_SIZE];
    instances_root(root, sizeof(root));
    snprintf(out, out_sz, "%s%s%s", root, PATH_SEP, id);
    ensure_directory_exists(out);
    return 1;
}
