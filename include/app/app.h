#ifndef CERO_APP_H
#define CERO_APP_H

typedef struct {
    const char* assets_path;
} LauncherOptions;

/* Parses argv into opts. Currently only reads --customAssetsPath=...
 * Always succeeds (kept as int to mirror the other lifecycle functions
 * and leave room for future validation). */
int launcher_parse_args(int argc, char** argv, LauncherOptions* opts);

/* Starts the local Java bridge, writes its port for single-instance
 * hand-off, sets up config/paths, installs cleanup handlers, loads the
 * embedded assets and connects Discord RPC. Returns 1 on success,
 * 0 if assets failed to load (fatal). */
int launcher_init(const LauncherOptions* opts);

/* Creates the native UI window and sets its base properties
 * (frameless, icon, theme watching). Returns 1 on success, 0 if window
 * creation failed. */
int launcher_create_ui(void);

/* Binds all the JS<->native IPC handlers, enables the JS console,
 * navigates to the app UI and locks the window down. */
void launcher_bind_ui(void);

/* Starts the tray icon / window-message hooks now that the window and
 * its bindings exist. */
void launcher_start_services(void);

/* Tears down the tray, single-instance lock and Discord RPC. */
void launcher_shutdown(void);

#endif /* CERO_APP_H */
