#ifndef CERO_INSTANCES_INSTANCE_H
#define CERO_INSTANCES_INSTANCE_H

#include <stddef.h>
#include "../config/config.h"

/*
 * An "instance" is an isolated CeroClient install: its own version,
 * loader (vanilla / fabric / forge), its own .minecraft-style directory
 * (versions/, libraries/, assets/, saves/, mods/...), so multiple
 * instances never share state.
 */

typedef enum {
    LOADER_VANILLA  = 0,
    LOADER_FABRIC   = 1,
    LOADER_FORGE    = 2,
    LOADER_NEOFORGE = 3,
    LOADER_QUILT    = 4
} LoaderType;

typedef struct {
    char       id[64];
    char       name[128];
    char       mc_version[64];
    LoaderType loader;
    char       loader_version[64]; /* empty = "latest"/"recommended" */
    long       ram_mb;             /* 0 = use the global default */
    long       created_at;
    long       last_played;
} Instance;

const char* loader_type_to_str(LoaderType t);
LoaderType  loader_type_from_str(const char* s);

/* Ensures <client_path>/instances/ exists and the index file is valid. */
int instances_init(void);

/* Writes a JSON array of all instances into `out` (cJSON, unformatted). */
int instances_list_json(char* out, size_t out_sz);

/* Creates a new instance, returns 1 on success and fills out_id. */
int instance_create(const char* name, const char* mc_version,
                    LoaderType loader, const char* loader_version,
                    char* out_id, size_t out_id_sz);

int instance_delete(const char* id);
int instance_rename(const char* id, const char* new_name);
int instance_set_ram(const char* id, long ram_mb);
int instance_touch_last_played(const char* id);

/* Loads a single instance's metadata. Returns 1 if found. */
int instance_get(const char* id, Instance* out);

/* Fills `out` with the instance's root directory
 * (<client_path>/instances/<id>), creating it if needed. */
int instance_get_dir(const char* id, char* out, size_t out_sz);

#endif
