#ifndef CERO_LAUNCH_ACCOUNT_H
#define CERO_LAUNCH_ACCOUNT_H

#include <stddef.h>
#include "launch_ctx.h"

/* Validates/refreshes the account at client_dir/account.json and fills
 * username/uuid/token from it. Returns 0 on failure (progress already
 * reported), 1 on success. */
int resolve_account(const LaunchCtx* ctx, const char* client_dir,
                    char* username, size_t username_sz,
                    char* uuid, size_t uuid_sz,
                    char* token, size_t token_sz);

#endif /* CERO_LAUNCH_ACCOUNT_H */
