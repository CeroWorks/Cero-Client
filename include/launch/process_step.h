#ifndef CERO_LAUNCH_PROCESS_STEP_H
#define CERO_LAUNCH_PROCESS_STEP_H

#include "launch_ctx.h"
#include "launch_minecraft.h"

void run_game_process(const LaunchCtx* ctx, LaunchUserdata* ud,
                      const char* java_exe, const char** argv,
                      const char* version, const char* username, int is_fabric);

#endif 
