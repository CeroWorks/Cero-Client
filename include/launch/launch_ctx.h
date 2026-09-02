#ifndef CERO_LAUNCH_CTX_H
#define CERO_LAUNCH_CTX_H

#include "launch_minecraft.h"

typedef struct {
    launch_progress_cb cb;
    void*               userdata;
} LaunchCtx;

/* Logs `step` and forwards it to ctx->cb (if any) with `pct`.
 * Equivalent to the old `PROGRESS(step, pct)` macro in launch_minecraft.c. */
void launch_report(const LaunchCtx* ctx, const char* step, int pct);

#endif /* CERO_LAUNCH_CTX_H */
