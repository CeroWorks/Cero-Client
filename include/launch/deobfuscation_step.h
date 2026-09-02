#ifndef CERO_LAUNCH_DEOBFUSCATION_STEP_H
#define CERO_LAUNCH_DEOBFUSCATION_STEP_H

#include <stddef.h>
#include "launch_ctx.h"

/* If `vanilla_version` needs deobfuscation, ensures Java is available,
 * runs the remap (using cached output if present), and fills
 * mapped_jar_out with the path to the mapped jar. If deobfuscation isn't
 * needed, mapped_jar_out[0] is set to '\0'. Returns 0 on hard failure
 * (progress already reported), 1 otherwise. */
int resolve_deobfuscated_jar(const LaunchCtx* ctx, const char* client_dir,
                             const char* vanilla_version,
                             const char* jar_dest, const char* json_dest,
                             char* mapped_jar_out, size_t mapped_jar_sz);

#endif /* CERO_LAUNCH_DEOBFUSCATION_STEP_H */
