#include "../../include/launch/deobfuscation_step.h"
#include "../../include/launch/java.h"
#include "../../include/utils/deobfuscation.h"
#include "../../include/config/config.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <unistd.h>

int resolve_deobfuscated_jar(const LaunchCtx* ctx, const char* client_dir,
                             const char* vanilla_version,
                             const char* jar_dest, const char* json_dest,
                             char* mapped_jar_out, size_t mapped_jar_sz) {
    mapped_jar_out[0] = '\0';

    if (!version_needs_deobfuscation(vanilla_version)) {
        return 1;
    }

    launch_report(ctx, "Déobfuscation du client...", 34);
    snprintf(mapped_jar_out, mapped_jar_sz,
             "%s/versions/%s/%s.mapped.jar", client_dir, vanilla_version, vanilla_version);

    if (access(mapped_jar_out, F_OK) == 0) {
        log_msg("info", "Jar déobfusqué en cache: %s\n", mapped_jar_out);
        return 1;
    }

    if (!java_ensure(client_dir, vanilla_version)) {
        launch_report(ctx, "Erreur : Java introuvable pour la déobfuscation !", -1);
        return 0;
    }
    int dmajor = java_get_required_version(client_dir, vanilla_version);
    char djava_exe[MAX_PATH_SIZE];
    if (!java_resolve_executable(client_dir, dmajor, djava_exe, sizeof(djava_exe))) {
        launch_report(ctx, "Erreur : exécutable Java introuvable !", -1);
        return 0;
    }

    const char* extract_dir = client_path;
    char cero_jar_for_remap[MAX_PATH_SIZE];
    snprintf(cero_jar_for_remap, sizeof(cero_jar_for_remap),
             "%s/agent/CeroClient-MC.jar", extract_dir);

    if (!deobfuscate_client_jar(client_dir, vanilla_version,
                                 jar_dest, mapped_jar_out, json_dest,
                                 djava_exe, cero_jar_for_remap)) {
        launch_report(ctx, "Erreur : échec de la déobfuscation !", -1);
        mapped_jar_out[0] = '\0';
        return 0;
    }

    return 1;
}
