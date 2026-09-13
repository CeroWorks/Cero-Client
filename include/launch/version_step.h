#ifndef CERO_LAUNCH_VERSION_STEP_H
#define CERO_LAUNCH_VERSION_STEP_H

#include <stddef.h>
#include "launch_ctx.h"

int resolve_version_json(const LaunchCtx* ctx, const char* client_dir,
                         const char* vanilla_version,
                         char* json_dest, size_t json_dest_sz,
                         char* jar_dest, size_t jar_dest_sz);

#endif 
