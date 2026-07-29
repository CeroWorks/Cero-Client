#ifndef DISCORD_RPC_H
#define DISCORD_RPC_H

#include <stdint.h>

int  discord_rpc_init(const char* client_id);
void discord_rpc_update(const char* state, const char* details,
                        const char* large_image, const char* large_text,
                        const char* small_image, const char* small_text,
                        int64_t start_timestamp);
void discord_rpc_clear(void);
void discord_rpc_shutdown(void);

#endif
