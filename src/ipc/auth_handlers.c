#include "../../include/ipc/auth_handlers.h"
#include "../../include/platform/paths.h"
#include "../../include/discord/rpc_helpers.h"
#include "../../include/config/config.h"
#include "../../include/net/ms_auth.h"
#include "../../include/ui/ui.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void on_check_account(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    int ok = 0;
    if (ms_auth_validate(path) == 0) {
        ok = 1;
    } else if (ms_auth_refresh(path) == 0) {
        ok = 1;
    }

    if (ok) rpc_set_idle();
    else    rpc_set_login();

    ui_return(arg, id, 0, ok ? "true" : "false");
}

void on_login_microsoft(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    int rc = ms_auth_login(path);
    if (rc == 0) {
        rpc_set_idle();
        ui_return(arg, id, 0, "\"ok\"");
    } else {
        char err[64];
        snprintf(err, sizeof(err), "\"error_%d\"", rc);
        ui_return(arg, id, 1, err);
    }
}

void on_get_account(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        ui_return(arg, id, 1, "\"not_found\"");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); ui_return(arg, id, 1, "\"alloc_error\""); return; }

    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    ui_return(arg, id, 0, buf);
    free(buf);
}

void on_get_mc_token(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    log_msg("info", "Getting Minecraft Account Token...\n");

    if (ms_auth_validate(path) != 0) {
        if (ms_auth_refresh(path) != 0) {
            ui_return(arg, id, 1, "\"refresh_failed\"");
            log_msg("error", "Refresh Failed...\n");
            return;
        }
    }

    FILE* f = fopen(path, "r");
    if (!f) { ui_return(arg, id, 1, "\"not_found\""); log_msg("error", "Not found\n"); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size <= 0 || size > 1024 * 1024) { fclose(f); ui_return(arg, id, 1, "\"invalid_size\""); log_msg("error", "Invalid Size\n"); return; }

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); ui_return(arg, id, 1, "\"alloc_error\""); log_msg("error", "Alloc Error\n"); return; }
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);

    const char* key = "\"mc_token\"";
    char* p = strstr(buf, key);
    if (!p) { free(buf); ui_return(arg, id, 1, "\"no_token\""); return; }
    p += strlen(key);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p != '"') { free(buf); ui_return(arg, id, 1, "\"parse_error\""); return; }
    p++;
    char* end = strchr(p, '"');
    if (!end) { free(buf); ui_return(arg, id, 1, "\"parse_error\""); return; }

    size_t tlen = (size_t)(end - p);
    char* out = malloc(tlen + 3);
    if (!out) { free(buf); ui_return(arg, id, 1, "\"alloc_error\""); return; }
    out[0] = '"';
    memcpy(out + 1, p, tlen);
    out[1 + tlen] = '"';
    out[2 + tlen] = '\0';

    ui_return(arg, id, 0, out);
    free(out);
    free(buf);
}

void on_logout_account(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    if (file_exists(path)) {
        if (remove(path) != 0) {
            log_msg("error", "logout_account: impossible de supprimer %s\n", path);
            ui_return(arg, id, 1, "\"remove_failed\"");
            return;
        }
    }

    log_msg("info", "Compte deconnecte (account.json supprime)\n");
    rpc_set_login();
    ui_return(arg, id, 0, "\"ok\"");
}
