#include "../../include/discord/rpc_helpers.h"
#include "../../include/app/app_state.h"
#include "../../include/discord/discord_rpc.h"
#include <stdio.h>
#include <time.h>

void rpc_set_launching(void) {
    discord_rpc_update(NULL, "Launching...", "logo", "CeroClient",
                       NULL, NULL, g_start_timestamp);
}

void rpc_set_login(void) {
    discord_rpc_update(NULL, "In login menu", "logo", "CeroClient",
                       NULL, NULL, g_start_timestamp);
}

void rpc_set_idle(void) {
    discord_rpc_update(NULL, "Idling in menu", "logo", "CeroClient",
                       NULL, NULL, g_start_timestamp);
}

void rpc_set_playing(const char* version) {
    char details[128];
    snprintf(details, sizeof(details), "Playing %s", version ? version : "Minecraft");
    discord_rpc_update(NULL, details, "logo", "CeroClient",
                       NULL, NULL, (int64_t)time(NULL));
}
