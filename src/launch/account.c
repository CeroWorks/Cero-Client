#include "../../include/launch/account.h"
#include "../../include/launch/mc_json.h"
#include "../../include/net/ms_auth.h"
#include "../../include/config/config.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int resolve_account(const LaunchCtx* ctx, const char* client_dir,
                    char* username, size_t username_sz,
                    char* uuid, size_t uuid_sz,
                    char* token, size_t token_sz) {
    launch_report(ctx, "Chargement du compte...", 5);

    char account_path[MAX_PATH_SIZE];
    snprintf(account_path, sizeof(account_path),
             "%s/account.json", client_dir);

    if (ms_auth_validate(account_path) != 0) {
        launch_report(ctx, "Rafraîchissement du token...", 8);
        if (ms_auth_refresh(account_path) != 0) {
            launch_report(ctx, "Erreur : compte invalide !", -1);
            log_msg("error", "Account invalid / refresh failed\n");
            return 0;
        }
    }

    snprintf(username, username_sz, "%s", "Player");
    snprintf(uuid, uuid_sz, "%s", "00000000-0000-0000-0000-000000000000");
    snprintf(token, token_sz, "%s", "0");

    char* acct_json = read_file(account_path);
    if (acct_json) {
        json_str(acct_json, "name",     username, username_sz);
        json_str(acct_json, "uuid",     uuid,     uuid_sz);
        json_str(acct_json, "mc_token", token,    token_sz);
        free(acct_json);
    }

    return 1;
}
