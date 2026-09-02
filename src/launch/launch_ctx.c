#include "../../include/launch/launch_ctx.h"
#include "../../include/core/logger.h"

void launch_report(const LaunchCtx* ctx, const char* step, int pct) {
    log_msg("info", "%s\n", step);
    if (ctx && ctx->cb) ctx->cb(step, pct, ctx->userdata);
}
