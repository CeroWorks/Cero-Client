#ifndef CERO_LAUNCH_DEOBFUSCATION_STEP_H
#define CERO_LAUNCH_DEOBFUSCATION_STEP_H

#include <stddef.h>
#include "launch_ctx.h"

int resolve_deobfuscated_jar(const LaunchCtx* ctx, const char* client_dir,
                             const char* vanilla_version,
                             const char* jar_dest, const char* json_dest,
                             char* mapped_jar_out, size_t mapped_jar_sz);

#endif 
