#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include "../include/config.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #define PATH_SEP "\\"
#else
  #include <sys/stat.h>
  #include <unistd.h>
  #define PATH_SEP "/"
#endif

char client_path[MAX_PATH_SIZE] = {0};





static void config_path(char* out, size_t sz) {
    snprintf(out, sz, "%s%sconfig.json", client_path, PATH_SEP);
}

static int file_exists(const char* path) {
#ifdef _WIN32
    DWORD a = GetFileAttributesA(path);
    return (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

static char* read_file_all(const char* path, long* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size < 0 || size > 8 * 1024 * 1024) { fclose(f); return NULL; }
    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);
    if (out_size) *out_size = (long)n;
    return buf;
}

static int write_file_all(const char* path, const char* data, size_t len) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    size_t n = fwrite(data, 1, len, f);
    fclose(f);
    return (n == len) ? 0 : -1;
}






static int find_json_value(const char* json, const char* key,
                           const char** k_start, const char** k_end,
                           const char** v_start, const char** v_end) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return -1;

    *k_start = p;
    const char* q = p + strlen(pattern);
    while (*q && (*q == ' ' || *q == '\t' || *q == ':' || *q == '\n' || *q == '\r')) q++;
    if (!*q) return -1;

    const char* vs = q;
    const char* ve = q;

    if (*q == '"') {
        ve = q + 1;
        while (*ve && *ve != '"') {
            if (*ve == '\\' && ve[1]) ve++;
            ve++;
        }
        if (*ve == '"') ve++;
    } else if (*q == '{' || *q == '[') {
        char open = *q;
        char close = (open == '{') ? '}' : ']';
        int depth = 0;
        ve = q;
        while (*ve) {
            if (*ve == '"') {
                ve++;
                while (*ve && *ve != '"') {
                    if (*ve == '\\' && ve[1]) ve++;
                    ve++;
                }
                if (*ve) ve++;
                continue;
            }
            if (*ve == open) depth++;
            else if (*ve == close) {
                depth--;
                if (depth == 0) { ve++; break; }
            }
            ve++;
        }
    } else {
        while (*ve && *ve != ',' && *ve != '}' && *ve != '\n' && *ve != '\r')
            ve++;
        while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    }

    *v_start = vs;
    *v_end = ve;
    *k_end = ve;
    return 0;
}





void init_config(void) {
    
    if (client_path[0] == '\0') {
#ifdef _WIN32
        const char* appdata = getenv("APPDATA");
        if (appdata) snprintf(client_path, MAX_PATH_SIZE, "%s\\.ceroclient", appdata);
        else snprintf(client_path, MAX_PATH_SIZE, ".\\.ceroclient");
        _mkdir(client_path);
#else
        const char* home = getenv("HOME");
        if (home) snprintf(client_path, MAX_PATH_SIZE, "%s/.ceroclient", home);
        else snprintf(client_path, MAX_PATH_SIZE, "./.ceroclient");
        mkdir(client_path, 0755);
#endif
    }

    
    char path[MAX_PATH_SIZE];
    config_path(path, sizeof(path));

    if (!file_exists(path)) {
        const char* empty = "{}";
        if (write_file_all(path, empty, 2) == 0) {
            log_msg("info", "Created new config.json at %s\n", path);
        } else {
            log_msg("error", "Failed to create config.json at %s\n", path);
        }
    }
}





char* config_get(const char* key) {
    char path[MAX_PATH_SIZE];
    config_path(path, sizeof(path));

    char* json = read_file_all(path, NULL);
    if (!json) return NULL;

    const char *ks, *ke, *vs, *ve;
    if (find_json_value(json, key, &ks, &ke, &vs, &ve) != 0) {
        free(json);
        return NULL;
    }

    size_t len = (size_t)(ve - vs);
    char* out = malloc(len + 1);
    if (!out) { free(json); return NULL; }
    memcpy(out, vs, len);
    out[len] = '\0';
    free(json);
    return out;
}

int config_add(const char* key, const char* value) {
    char path[MAX_PATH_SIZE];
    config_path(path, sizeof(path));

    char* json = read_file_all(path, NULL);
    if (!json) {
        
        size_t klen = strlen(key), vlen = strlen(value);
        size_t cap = klen + vlen + 8;
        char* nj = malloc(cap);
        size_t n = snprintf(nj, cap, "{\"%s\":%s}", key, value);
        int rc = write_file_all(path, nj, n);
        free(nj);
        return (rc == 0) ? 0 : -1;
    }

    const char *ks, *ke, *vs, *ve;
    if (find_json_value(json, key, &ks, &ke, &vs, &ve) == 0) {
        free(json);
        return 1; 
    }

    char* close = strrchr(json, '}');
    if (!close) { free(json); return -1; }

    size_t before = (size_t)(close - json);
    int empty = 1;
    for (size_t i = 1; i < before; i++) {
        char c = json[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') { empty = 0; break; }
    }

    size_t klen = strlen(key), vlen = strlen(value);
    size_t cap = before + klen + vlen + 16;
    char* nj = malloc(cap);
    if (!nj) { free(json); return -1; }

    memcpy(nj, json, before);
    size_t pos = before;
    if (!empty) nj[pos++] = ',';
    pos += snprintf(nj + pos, cap - pos, "\"%s\":%s}", key, value);

    int rc = write_file_all(path, nj, pos);
    free(nj);
    free(json);
    return (rc == 0) ? 0 : -1;
}

int config_set(const char* key, const char* value) {
    char path[MAX_PATH_SIZE];
    config_path(path, sizeof(path));

    char* json = read_file_all(path, NULL);
    if (!json) {
        return config_add(key, value);
    }

    const char *ks, *ke, *vs, *ve;
    if (find_json_value(json, key, &ks, &ke, &vs, &ve) != 0) {
        free(json);
        return config_add(key, value);
    }

    size_t before = (size_t)(vs - json);
    size_t after_len = strlen(ve);
    size_t vlen = strlen(value);

    char* nj = malloc(before + vlen + after_len + 1);
    if (!nj) { free(json); return -1; }

    memcpy(nj, json, before);
    memcpy(nj + before, value, vlen);
    memcpy(nj + before + vlen, ve, after_len);
    size_t total = before + vlen + after_len;
    nj[total] = '\0';

    int rc = write_file_all(path, nj, total);
    free(nj);
    free(json);
    return (rc == 0) ? 0 : -1;
}

int config_delete(const char* key) {
    char path[MAX_PATH_SIZE];
    config_path(path, sizeof(path));

    char* json = read_file_all(path, NULL);
    if (!json) return 1;

    const char *ks, *ke, *vs, *ve;
    if (find_json_value(json, key, &ks, &ke, &vs, &ve) != 0) {
        free(json);
        return 1; 
    }

    
    const char* del_start = ks;
    const char* del_end   = ve;

    
    while (del_end < json + strlen(json) &&
           (*del_end == ' ' || *del_end == '\t' || *del_end == '\n' || *del_end == '\r'))
        del_end++;

    
    if (*del_end == ',') {
        del_end++;
    } else {
        
        const char* p = del_start;
        while (p > json && (p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\n' || p[-1] == '\r'))
            p--;
        if (p > json && p[-1] == ',') {
            del_start = p - 1;
        }
    }

    size_t before = (size_t)(del_start - json);
    size_t after_len = strlen(del_end);
    size_t total = before + after_len;

    char* nj = malloc(total + 1);
    if (!nj) { free(json); return -1; }
    memcpy(nj, json, before);
    memcpy(nj + before, del_end, after_len);
    nj[total] = '\0';

    int rc = write_file_all(path, nj, total);
    free(nj);
    free(json);
    return (rc == 0) ? 0 : -1;
}
