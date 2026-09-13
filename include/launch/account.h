#ifndef CERO_LAUNCH_ACCOUNT_H
#define CERO_LAUNCH_ACCOUNT_H

#include <stddef.h>
#include "launch_ctx.h"

int resolve_account(const LaunchCtx* ctx, const char* client_dir,
                    char* username, size_t username_sz,
                    char* uuid, size_t uuid_sz,
                    char* token, size_t token_sz);

#endif 
