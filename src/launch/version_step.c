#include "../../include/launch/version_step.h"
#include "../../include/launch/manifest.h"
#include "../../include/utils/version_manifest.h"
#include "../../include/utils/file_utils.h"
#include "../../include/net/download.h"
#include "../../include/crypto/sha1.h"
#include "../../include/core/logger.h"
#include <stdio.h>

int resolve_version_json(const LaunchCtx* ctx, const char* client_dir,
                         const char* vanilla_version,
                         char* json_dest, size_t json_dest_sz,
                         char* jar_dest, size_t jar_dest_sz) {
    launch_report(ctx, "Récupération du manifest...", 12);
    ensure_directory_exists(client_dir);
    download_manifest();

    VersionInfo vi;
    if (!manifest_find_version(client_dir, vanilla_version, &vi)) {
        launch_report(ctx, "Erreur : version introuvable !", -1);
        log_msg("error", "Version %s not found\n", vanilla_version);
        return 0;
    }

    launch_report(ctx, "Téléchargement des métadonnées...", 18);
    snprintf(json_dest, json_dest_sz,
             "%s/versions/%s/%s.json", client_dir, vi.id, vi.id);
    download_file(vi.url, json_dest);
    version_info_free(&vi);

    launch_report(ctx, "Téléchargement du client.jar...", 28);
    DownloadEntry client_entry;
    if (!version_get_client_jar(client_dir, vanilla_version, &client_entry)) {
        launch_report(ctx, "Erreur : impossible d'obtenir client.jar !", -1);
        log_msg("error", "Cannot get client jar info\n");
        return 0;
    }
    snprintf(jar_dest, jar_dest_sz,
             "%s/versions/%s/%s.jar", client_dir, vanilla_version, vanilla_version);
    download_file(client_entry.url, jar_dest);

    if (!sha1_file_matches(jar_dest, client_entry.sha1)) {
        launch_report(ctx, "Erreur : client.jar corrompu ou falsifié (SHA-1 invalide) !", -1);
        log_msg("error", "client.jar SHA-1 mismatch for %s (attendu %s) — fichier supprimé\n",
            vanilla_version, client_entry.sha1 ? client_entry.sha1 : "(absent)");
        remove(jar_dest);
        return 0;
    }

    return 1;
}
