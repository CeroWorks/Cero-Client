#ifndef CERO_LAUNCH_PROCESS_STEP_H
#define CERO_LAUNCH_PROCESS_STEP_H

#include "launch_ctx.h"
#include "launch_minecraft.h"

/* Signals game start (UI/Discord), runs java_exe with argv until it
 * exits, then signals game stop. Logs the exit code. `version` and
 * `username`/`is_fabric` are only used for log/RPC text. */
void run_game_process(const LaunchCtx* ctx, LaunchUserdata* ud,
                      const char* java_exe, const char** argv,
                      const char* version, const char* username, int is_fabric);

#endif /* CERO_LAUNCH_PROCESS_STEP_H */
