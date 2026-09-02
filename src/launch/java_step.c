#include "../../include/launch/java_step.h"
#include "../../include/launch/java.h"
#include "../../include/core/logger.h"

int resolve_java_runtime(const LaunchCtx* ctx, const char* client_dir,
                         const char* vanilla_version,
                         char* java_exe_out, size_t java_exe_sz) {
    launch_report(ctx, "Vérification de Java...", 80);
    if (!java_ensure(client_dir, vanilla_version)) {
        launch_report(ctx, "Erreur : Java introuvable !", -1);
        log_msg("error", "java_ensure failed for %s\n", vanilla_version);
        return 0;
    }

    int major = java_get_required_version(client_dir, vanilla_version);
    if (!java_resolve_executable(client_dir, major, java_exe_out, java_exe_sz)) {
        launch_report(ctx, "Erreur : exécutable Java introuvable !", -1);
        log_msg("error",
            "Java %d executable introuvable. Sur FreeBSD : pkg install openjdk%d\n",
            major, major);
        return 0;
    }
    log_msg("info", "Using Java executable: %s\n", java_exe_out);

    return major;
}
