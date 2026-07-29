#ifndef VERSION_MANIFEST_H
#define VERSION_MANIFEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "../include/logger.h"
#include "../include/utils/platform.h"
#include "../include/config.h"

typedef struct {
    char *id;
    char *type;
    char *url;
    char *sha1;
    char *release_time;
} VersionInfo;

typedef struct {
    char *name;
    char *hash;
    char *url;
    long  size;
} AssetEntry;

typedef struct {
    char *name;
    char *path;
    char *url;
    char *sha1;
    long  size;

    char *native_path;
    char *native_url;
    char *native_sha1;
    long  native_size;
} LibraryEntry;


typedef struct {
    char *url;
    char *sha1;
    long  size;
} DownloadEntry;

typedef struct { AssetEntry   *items; size_t count; } AssetList;
typedef struct { LibraryEntry *items; size_t count; } LibraryList;

typedef enum { VM_JNULL, VM_JBOOL, VM_JNUM, VM_JSTR, VM_JARR, VM_JOBJ } VmJType;
typedef struct VmJVal VmJVal;
typedef struct { char *k; VmJVal *v; } VmJPair;

struct VmJVal {
    VmJType t;
    union {
        int    b;
        double n;
        char  *s;
        struct { VmJVal **items; size_t count; } a;
        struct { VmJPair *pairs; size_t count; } o;
    };
};

typedef struct { const char *text; size_t pos; } VmJP;

static inline char *vm_strdup_n(const char *s, size_t n) {
    char *r = (char*)malloc(n + 1);
    if (!r) return NULL;
    memcpy(r, s, n); r[n] = 0;
    return r;
}
static inline char *vm_strdup_s(const char *s) {
    return s ? vm_strdup_n(s, strlen(s)) : NULL;
}

static inline void vm_skip(VmJP *p) {
    while (isspace((unsigned char)p->text[p->pos])) p->pos++;
}

static inline VmJVal *vm_new(VmJType t) {
    VmJVal *v = (VmJVal*)calloc(1, sizeof(VmJVal));
    v->t = t; return v;
}

static inline char *vm_parse_str(VmJP *p) {
    if (p->text[p->pos] != '"') return NULL;
    p->pos++;
    size_t cap = 32, len = 0;
    char *buf = (char*)malloc(cap);
    while (p->text[p->pos] && p->text[p->pos] != '"') {
        char c = p->text[p->pos++];
        if (c == '\\') {
            char e = p->text[p->pos++];
            switch (e) {
                case 'n': c='\n'; break;
                case 't': c='\t'; break;
                case 'r': c='\r'; break;
                case '"': c='"';  break;
                case '\\':c='\\'; break;
                case '/': c='/';  break;
                case 'b': c='\b'; break;
                case 'f': c='\f'; break;
                case 'u':
                    for (int i = 0; i < 4 && p->text[p->pos]; i++) p->pos++;
                    c = '?'; break;
                default: c = e;
            }
        }
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        buf[len++] = c;
    }
    if (p->text[p->pos] == '"') p->pos++;
    buf[len] = 0;
    return buf;
}

static inline VmJVal *vm_parse_val(VmJP *p);

static inline VmJVal *vm_parse_num(VmJP *p) {
    size_t start = p->pos;
    if (p->text[p->pos] == '-') p->pos++;
    while (isdigit((unsigned char)p->text[p->pos])) p->pos++;
    int is_float = 0;
    if (p->text[p->pos] == '.') {
        is_float = 1;
        p->pos++;
        while (isdigit((unsigned char)p->text[p->pos])) p->pos++;
    }
    if (p->text[p->pos] == 'e' || p->text[p->pos] == 'E') {
        is_float = 1;
        p->pos++;
        if (p->text[p->pos] == '+' || p->text[p->pos] == '-') p->pos++;
        while (isdigit((unsigned char)p->text[p->pos])) p->pos++;
    }
    char tmp[64];
    size_t n = p->pos - start;
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, p->text + start, n);
    tmp[n] = 0;
    VmJVal *v = vm_new(VM_JNUM);
    if (is_float) v->n = strtod(tmp, NULL);
    else          v->n = (double)strtoll(tmp, NULL, 10);
    return v;
}

static inline VmJVal *vm_parse_arr(VmJP *p) {
    p->pos++;
    VmJVal *v = vm_new(VM_JARR);
    vm_skip(p);
    if (p->text[p->pos] == ']') { p->pos++; return v; }
    size_t cap = 4;
    v->a.items = (VmJVal**)malloc(cap * sizeof(VmJVal*));
    while (1) {
        vm_skip(p);
        VmJVal *it = vm_parse_val(p);
        if (v->a.count >= cap) { cap *= 2; v->a.items = (VmJVal**)realloc(v->a.items, cap*sizeof(VmJVal*)); }
        v->a.items[v->a.count++] = it;
        vm_skip(p);
        if (p->text[p->pos] == ',') { p->pos++; continue; }
        if (p->text[p->pos] == ']') { p->pos++; break; }
        break;
    }
    return v;
}

static inline VmJVal *vm_parse_obj(VmJP *p) {
    p->pos++;
    VmJVal *v = vm_new(VM_JOBJ);
    vm_skip(p);
    if (p->text[p->pos] == '}') { p->pos++; return v; }
    size_t cap = 4;
    v->o.pairs = (VmJPair*)malloc(cap * sizeof(VmJPair));
    while (1) {
        vm_skip(p);
        char *k = vm_parse_str(p);
        vm_skip(p);
        if (p->text[p->pos] == ':') p->pos++;
        vm_skip(p);
        VmJVal *val = vm_parse_val(p);
        if (v->o.count >= cap) { cap *= 2; v->o.pairs = (VmJPair*)realloc(v->o.pairs, cap*sizeof(VmJPair)); }
        v->o.pairs[v->o.count].k = k;
        v->o.pairs[v->o.count].v = val;
        v->o.count++;
        vm_skip(p);
        if (p->text[p->pos] == ',') { p->pos++; continue; }
        if (p->text[p->pos] == '}') { p->pos++; break; }
        log_msg("debug", "vm_parse_obj UNEXPECTED char='%c'(0x%02x) at pos=%zu after key='%s', context='%.40s'\n",
                p->text[p->pos], (unsigned char)p->text[p->pos], p->pos, k, p->text + p->pos);
        break;
    }
    return v;
}

static inline VmJVal *vm_parse_val(VmJP *p) {
    vm_skip(p);
    char c = p->text[p->pos];
    if (c == '"') { VmJVal *v = vm_new(VM_JSTR); v->s = vm_parse_str(p); return v; }
    if (c == '{') return vm_parse_obj(p);
    if (c == '[') return vm_parse_arr(p);
    if (c == 't' && !strncmp(p->text+p->pos,"true",4))  { p->pos+=4; VmJVal *v=vm_new(VM_JBOOL); v->b=1; return v; }
    if (c == 'f' && !strncmp(p->text+p->pos,"false",5)) { p->pos+=5; VmJVal *v=vm_new(VM_JBOOL); v->b=0; return v; }
    if (c == 'n' && !strncmp(p->text+p->pos,"null",4))  { p->pos+=4; return vm_new(VM_JNULL); }
    if (c == '-' || isdigit((unsigned char)c)) return vm_parse_num(p);
    return vm_new(VM_JNULL);
}

static inline void vm_free(VmJVal *v) {
    if (!v) return;
    if (v->t == VM_JSTR) free(v->s);
    else if (v->t == VM_JARR) {
        for (size_t i = 0; i < v->a.count; i++) vm_free(v->a.items[i]);
        free(v->a.items);
    } else if (v->t == VM_JOBJ) {
        for (size_t i = 0; i < v->o.count; i++) { free(v->o.pairs[i].k); vm_free(v->o.pairs[i].v); }
        free(v->o.pairs);
    }
    free(v);
}

static inline VmJVal *vm_get(VmJVal *o, const char *k) {
    if (!o || o->t != VM_JOBJ) return NULL;
    for (size_t i = 0; i < o->o.count; i++)
        if (!strcmp(o->o.pairs[i].k, k)) return o->o.pairs[i].v;
    return NULL;
}
static inline const char *vm_gets(VmJVal *o, const char *k) {
    VmJVal *v = vm_get(o, k); return (v && v->t == VM_JSTR) ? v->s : NULL;
}
static inline long vm_getn(VmJVal *o, const char *k) {
    VmJVal *v = vm_get(o, k); return (v && v->t == VM_JNUM) ? (long)v->n : 0;
}

static inline char *vm_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { log_msg("error", "Cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}

static inline VmJVal *vm_load_json(const char *path) {
    char *txt = vm_read_file(path);
    if (!txt) return NULL;
    size_t total = strlen(txt);
    log_msg("debug", "Loaded %s: %zu bytes\n", path, total);
    VmJP p = { txt, 0 };
    VmJVal *v = vm_parse_val(&p);
    log_msg("debug", "Parsed %s: v=%p, stopped at pos=%zu/%zu, next chars='%.20s'\n",
            path, (void*)v, p.pos, total, txt + p.pos);
    free(txt);
    return v;
}

static inline void version_info_free(VersionInfo *v) {
    if (!v) return;
    free(v->id); free(v->type); free(v->url); free(v->sha1); free(v->release_time);
    memset(v, 0, sizeof(*v));
}
static inline void asset_list_free(AssetList *l) {
    if (!l) return;
    for (size_t i = 0; i < l->count; i++) {
        free(l->items[i].name); free(l->items[i].hash); free(l->items[i].url);
    }
    free(l->items); l->items = NULL; l->count = 0;
}
static inline void library_list_free(LibraryList *l) {
    if (!l) return;
    for (size_t i = 0; i < l->count; i++) {
        free(l->items[i].name);  free(l->items[i].path);
        free(l->items[i].url);   free(l->items[i].sha1);
        free(l->items[i].native_path); free(l->items[i].native_url);
        free(l->items[i].native_sha1);
    }
    free(l->items); l->items = NULL; l->count = 0;
}


static inline int manifest_find_version(const char *client_dir, const char *version_id, VersionInfo *out) {
    char path[MAX_PATH_SIZE];
    snprintf(path, sizeof(path), "%s/version_manifest.json", client_dir);

    log_msg("info", "Searching version %s in manifest ...\n", version_id);

    VmJVal *root = vm_load_json(path);
    if (!root) return 0;

    VmJVal *versions = vm_get(root, "versions");
    int found = 0;
    if (versions && versions->t == VM_JARR) {
        for (size_t i = 0; i < versions->a.count; i++) {
            VmJVal *v = versions->a.items[i];
            const char *id = vm_gets(v, "id");
            if (id && !strcmp(id, version_id)) {
                memset(out, 0, sizeof(*out));
                out->id           = vm_strdup_s(id);
                out->type         = vm_strdup_s(vm_gets(v, "type"));
                out->url          = vm_strdup_s(vm_gets(v, "url"));
                out->sha1         = vm_strdup_s(vm_gets(v, "sha1"));
                out->release_time = vm_strdup_s(vm_gets(v, "releaseTime"));
                found = 1;
                break;
            }
        }
    }

    vm_free(root);

    if (found) log_msg("succes", "Version %s found\n", version_id);
    else       log_msg("error",  "Version %s not found in manifest\n", version_id);

    return found;
}

static inline int manifest_version_exists(const char *client_dir, const char *version_id) {
    VersionInfo info;
    if (manifest_find_version(client_dir, version_id, &info)) {
        version_info_free(&info);
        return 1;
    }
    return 0;
}

static inline int manifest_get_latest(const char *client_dir, const char *type,
                                      char *out, size_t out_size) {
    char path[MAX_PATH_SIZE];
    snprintf(path, sizeof(path), "%s/version_manifest.json", client_dir);
    VmJVal *root = vm_load_json(path);
    if (!root) return 0;
    const char *s = vm_gets(vm_get(root, "latest"), type);
    int ok = 0;
    if (s) { snprintf(out, out_size, "%s", s); ok = 1; }
    vm_free(root);
    return ok;
}

static inline int vm_rules_allow(VmJVal *rules) {
    if (!rules || rules->t != VM_JARR) return 1;

#if defined(_WIN32)
    const char *my_os = "windows";
#elif defined(__APPLE__)
    const char *my_os = "osx";
#else
    const char *my_os = "linux";
#endif

    int allowed = 0;
    for (size_t i = 0; i < rules->a.count; i++) {
        VmJVal *r = rules->a.items[i];
        const char *action = vm_gets(r, "action");
        VmJVal *os = vm_get(r, "os");

        int os_match = 1;
        if (os) {
            const char *name = vm_gets(os, "name");
            if (name) os_match = (strcmp(name, my_os) == 0);
        }

        if (os_match && action) {
            allowed = (strcmp(action, "allow") == 0);
        }
    }
    return allowed;
}

static inline int version_get_libraries(const char *client_dir, const char *version_id, LibraryList *out) {
    char path[MAX_PATH_SIZE];
    snprintf(path, sizeof(path), "%s/versions/%s/%s.json", client_dir, version_id, version_id);
    VmJVal *root = vm_load_json(path);
    if (!root) return 0;

    memset(out, 0, sizeof(*out));
    VmJVal *libs = vm_get(root, "libraries");
    if (libs && libs->t == VM_JARR) {
        out->items = (LibraryEntry*)calloc(libs->a.count, sizeof(LibraryEntry));
        for (size_t i = 0; i < libs->a.count; i++) {
            VmJVal *lib = libs->a.items[i];

            VmJVal *rules = vm_get(lib, "rules");
            if (!vm_rules_allow(rules)) continue;

            VmJVal *downloads = vm_get(lib, "downloads");
            VmJVal *art = vm_get(downloads, "artifact");

            LibraryEntry *e = &out->items[out->count++];
            e->name = vm_strdup_s(vm_gets(lib, "name"));

            if (art) {
                e->path = vm_strdup_s(vm_gets(art, "path"));
                e->url  = vm_strdup_s(vm_gets(art, "url"));
                e->sha1 = vm_strdup_s(vm_gets(art, "sha1"));
                e->size = vm_getn(art, "size");
            }

            VmJVal *classifiers = vm_get(downloads, "classifiers");
            if (classifiers) {
                VmJVal *native = vm_get(classifiers, NATIVES_KEY);
                if (native) {
                    e->native_path = vm_strdup_s(vm_gets(native, "path"));
                    e->native_url  = vm_strdup_s(vm_gets(native, "url"));
                    e->native_sha1 = vm_strdup_s(vm_gets(native, "sha1"));
                    e->native_size = vm_getn(native, "size");
                }
            }
        }
    }
    vm_free(root);
    log_msg("info", "Loaded %zu libraries for %s\n", out->count, version_id);
    return 1;
}

static inline int version_get_client_jar(const char *client_dir, const char *version_id, DownloadEntry *out) {
    char path[MAX_PATH_SIZE];
    snprintf(path, sizeof(path), "%s/versions/%s/%s.json", client_dir, version_id, version_id);
    log_msg("info", "Reading client jar from: '%s'\n", path);
    VmJVal *root = vm_load_json(path);
    if (!root) return 0;
    VmJVal *client = vm_get(vm_get(root, "downloads"), "client");
    log_msg("info", "client node: %p\n", (void*)client);
    int ok = 0;
    if (client) {
        memset(out, 0, sizeof(*out));
        out->url  = vm_strdup_s(vm_gets(client, "url"));
        out->sha1 = vm_strdup_s(vm_gets(client, "sha1"));
        out->size = vm_getn(client, "size");
        ok = 1;
    }
    log_msg("info", "root type: %d (expected %d=VM_JOBJ)\n", root->t, VM_JOBJ);
    log_msg("info", "root pairs count: %zu\n", root->o.count);
    for (size_t i = 0; i < root->o.count && i < 10; i++)
        log_msg("info", "  key[%zu] = '%s'\n", i, root->o.pairs[i].k);
    vm_free(root);
    return ok;
}

static inline int version_get_asset_index(const char *client_dir, const char *version_id,
                                          char *url_out, size_t url_size,
                                          char *id_out,  size_t id_size) {
    char path[MAX_PATH_SIZE];
    snprintf(path, sizeof(path), "%s/versions/%s/%s.json", client_dir, version_id, version_id);
    VmJVal *root = vm_load_json(path);
    if (!root) return 0;
    VmJVal *ai = vm_get(root, "assetIndex");
    int ok = 0;
    if (ai) {
        const char *u  = vm_gets(ai, "url");
        const char *id = vm_gets(ai, "id");
        if (u && id) {
            snprintf(url_out, url_size, "%s", u);
            snprintf(id_out,  id_size,  "%s", id);
            ok = 1;
        }
    }
    vm_free(root);
    return ok;
}

static inline int assets_load_index(const char *client_dir, const char *index_id, AssetList *out) {
    char path[MAX_PATH_SIZE];
    snprintf(path, sizeof(path), "%s/assets/indexes/%s.json", client_dir, index_id);
    VmJVal *root = vm_load_json(path);
    if (!root) return 0;

    memset(out, 0, sizeof(*out));
    VmJVal *objects = vm_get(root, "objects");
    if (objects && objects->t == VM_JOBJ) {
        out->items = (AssetEntry*)calloc(objects->o.count, sizeof(AssetEntry));
        for (size_t i = 0; i < objects->o.count; i++) {
            VmJPair *p = &objects->o.pairs[i];
            const char *hash = vm_gets(p->v, "hash");
            if (!hash) continue;
            AssetEntry *e = &out->items[out->count++];
            e->name = vm_strdup_s(p->k);
            e->hash = vm_strdup_s(hash);
            e->size = vm_getn(p->v, "size");
            char url[256];
            snprintf(url, sizeof(url),
                "https://resources.download.minecraft.net/%c%c/%s",
                hash[0], hash[1], hash);
            e->url = vm_strdup_s(url);
        }
    }
    vm_free(root);
    log_msg("info", "Loaded %zu assets from index %s\n", out->count, index_id);
    return 1;
}

#endif
