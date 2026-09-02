#include "../include/platform/platform_defines.h"

#include "../include/app/app.h"
#include "../include/app/app_state.h"
#include "../include/platform/single_instance.h"
#include "../include/ui/ui.h"
#include <locale.h>

#ifdef __ANDROID__
int cero_main(int argc, char** argv) {

}
#else
int main(int argc, char** argv) {
    setlocale(LC_NUMERIC, "C");

    LauncherOptions opts;
    if (!launcher_parse_args(argc, argv, &opts))
        return 1;

    if (single_instance_check())
        return 0;

    if (!launcher_init(&opts))
        return 1;

    if (!launcher_create_ui())
        goto cleanup;

    launcher_bind_ui();
    launcher_start_services();
    ui_run(g_ui);

cleanup:
    launcher_shutdown();
    return 0;
}
#endif
