#ifndef CERO_LAUNCH_JAVA_STEP_H
#define CERO_LAUNCH_JAVA_STEP_H

#include <stddef.h>
#include "launch_ctx.h"

int resolve_java_runtime(const LaunchCtx* ctx, const char* client_dir,
                         const char* vanilla_version,
                         char* java_exe_out, size_t java_exe_sz);

#endif 
