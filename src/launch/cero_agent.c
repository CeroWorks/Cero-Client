#include "../../include/launch/cero_agent.h"
#include "../../include/app/assets_loader.h"
#include "../../include/utils/file_utils.h"
#include "../../include/core/logger.h"
#include <stdint.h>
#include <stdio.h>

int extract_cero_agent(const char* client_dir, char* out_path, size_t out_path_sz) {
    out_path[0] = '\0';
    int has_cero = 0;

    const uint8_t* agent_data = NULL;
    size_t agent_size = 0;
    if (assets_get_file("agent/CeroClient-MC.jar", &agent_data, &agent_size) && agent_data && agent_size > 0) {
        snprintf(out_path, out_path_sz, "%s/agent/CeroClient-MC.jar", client_dir);
        ensure_parent_dirs(out_path);
        FILE* f = fopen(out_path, "wb");
        if (f) {
            fwrite(agent_data, 1, agent_size, f);
            fclose(f);
            has_cero = 1;
            log_msg("info", "CeroClient agent extracted to: %s\n", out_path);
        } else {
            log_msg("warn", "Failed to write CeroClient agent to disk!\n");
        }
        assets_free_buffer(agent_data);
    } else {
        log_msg("warn", "CeroClient-MC.jar not found in RAM assets. Launching vanilla.\n");
    }

    return has_cero;
}
