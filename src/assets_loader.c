#include "../include/assets_loader.h"
#include "../include/logger.h"
#include "../include/miniz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define MAX_DAT_SIZE  (size_t)(512ULL * 1024 * 1024)
#define MAX_FILE_SIZE (size_t)(64ULL  * 1024 * 1024)

typedef struct {
    uint8_t        *zip_data;
    size_t          zip_size;
    mz_zip_archive  zip;
    int             zip_open;
} asset_state_t;

static asset_state_t g_state;

static void secure_wipe(void *p, size_t n) {
    if (!p) return;
    volatile uint8_t *v = (volatile uint8_t *)p;
    while (n--) *v++ = 0;
}

static uint8_t *read_file_bounded(const char *path, size_t max, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0 || (size_t)sz > max) { fclose(f); return NULL; }
    rewind(f);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz ? (size_t)sz : 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return NULL; }
    *out_len = (size_t)sz;
    return buf;
}

int assets_load(const char *dat_path, const char *exe_path) {
    (void)exe_path;
    if (g_state.zip_open) return 1;

    size_t raw_len = 0;
    uint8_t *raw = read_file_bounded(dat_path, MAX_DAT_SIZE, &raw_len);
    if (!raw) {
        log_msg("error", "Cannot read %s\n", dat_path);
        return 0;
    }

    memset(&g_state.zip, 0, sizeof(g_state.zip));
    if (!mz_zip_reader_init_mem(&g_state.zip, raw, raw_len, 0)) {
        log_msg("error", "Invalid ZIP archive in %s\n", dat_path);
        free(raw);
        return 0;
    }

    g_state.zip_data = raw;
    g_state.zip_size = raw_len;
    g_state.zip_open = 1;

    log_msg("succes", "Assets loaded in RAM (%zu bytes, %u entries)\n",
            g_state.zip_size,
            (unsigned)mz_zip_reader_get_num_files(&g_state.zip));
    return 1;
}

int assets_get_file(const char *name, const uint8_t **out_data, size_t *out_size) {
    if (!g_state.zip_open || !name || !out_data || !out_size) return 0;

    int idx = mz_zip_reader_locate_file(&g_state.zip, name, NULL, 0);
    if (idx < 0) return 0;

    mz_zip_archive_file_stat st;
    if (!mz_zip_reader_file_stat(&g_state.zip, (mz_uint)idx, &st)) return 0;
    if (st.m_uncomp_size > MAX_FILE_SIZE) return 0;
    if (mz_zip_reader_is_file_a_directory(&g_state.zip, (mz_uint)idx)) return 0;

    size_t sz  = 0;
    void  *buf = mz_zip_reader_extract_to_heap(&g_state.zip, (mz_uint)idx, &sz, 0);
    if (!buf) return 0;

    *out_data = (const uint8_t *)buf;
    *out_size = sz;
    return 1;
}

void assets_free_buffer(const uint8_t *data) {
    if (data) mz_free((void *)data);
}

void assets_unload(void) {
    if (g_state.zip_open) {
        mz_zip_reader_end(&g_state.zip);
        g_state.zip_open = 0;
    }
    if (g_state.zip_data) {
        secure_wipe(g_state.zip_data, g_state.zip_size);
        free(g_state.zip_data);
        g_state.zip_data = NULL;
        g_state.zip_size = 0;
    }
}

static void signal_handler(int sig) {
    assets_unload();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void atexit_cb(void) { assets_unload(); }

void install_cleanup_handlers(void) {
    atexit(atexit_cb);
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
#ifdef SIGHUP
    signal(SIGHUP,  signal_handler);
#endif
#ifdef SIGABRT
    signal(SIGABRT, signal_handler);
#endif
}