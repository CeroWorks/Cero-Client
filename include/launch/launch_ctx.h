#ifndef CERO_LAUNCH_CTX_H
#define CERO_LAUNCH_CTX_H

#include "launch_minecraft.h"

typedef struct {
    launch_progress_cb cb;
    void*               userdata;
} LaunchCtx;

void launch_report(const LaunchCtx* ctx, const char* step, int pct);

#endif 
