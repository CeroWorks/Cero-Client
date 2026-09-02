#ifndef CERO_LAUNCH_VERSION_STEP_H
#define CERO_LAUNCH_VERSION_STEP_H

#include <stddef.h>
#include "launch_ctx.h"

/* Downloads the version manifest, resolves `vanilla_version`'s json,
 * downloads client.jar and verifies its SHA-1. Fills json_dest/jar_dest
 * (both at least MAX_PATH_SIZE). Returns 0 on failure (progress already
 * reported), 1 on success. */
int resolve_version_json(const LaunchCtx* ctx, const char* client_dir,
                         const char* vanilla_version,
                         char* json_dest, size_t json_dest_sz,
                         char* jar_dest, size_t jar_dest_sz);

#endif /* CERO_LAUNCH_VERSION_STEP_H */
