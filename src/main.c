#define _POSIX_C_SOURCE 200112L

#if defined(__FreeBSD__) || defined(__OpenBSD__) || \
      defined(__NetBSD__)   || defined(__DragonFly__)
  #ifndef __BSD_VISIBLE
    #define __BSD_VISIBLE 1
  #endif
  #ifndef _BSD_SOURCE
    #define _BSD_SOURCE 1
  #endif
  #ifndef _NETBSD_SOURCE
    #define _NETBSD_SOURCE 1
  #endif
#elif defined(__linux__)
  #ifndef _DEFAULT_SOURCE
    #define _DEFAULT_SOURCE 1
  #endif
#endif

#if defined(_WIN32)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || \
      defined(__NetBSD__)   || defined(__DragonFly__)
  #define __BSD__ 1
#elif defined(__linux__)
#endif

#include "../include/logger.h"
#include "../include/launch_minecraft.h"
#include "../include/config.h"
#include "../include/assets_loader.h"
#include "../include/ms_auth.h"
#include "../include/utils/process.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "discord_rpc.h"
#include "ui.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <wininet.h>
  #include <shellapi.h>
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "wininet.lib")
  #define PATH_SEP "\\"
#else
  #include <unistd.h>
  #include <limits.h>
  #include <sys/stat.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <sys/time.h>
  #include <sys/wait.h>
  #include <signal.h>
  #define PATH_SEP "/"
  #ifdef __BSD__
    #include <sys/sysctl.h>
  #endif
#endif

#include "../include/single_instance.h"
#include "../include/tray.h"
#include "../include/cJSON.h"
#include "../include/sha256.h"

volatile int game_running = 0;
int g_really_quit = 0;
static void* g_ui = NULL;

static int64_t g_start_timestamp = 0;

static int build_account_path(char *out, size_t sz) {
    int n = snprintf(out, sz, "%s%saccount.json", client_path, PATH_SEP);
    return (n > 0 && (size_t)n < sz);
}

static void rpc_set_launching(void) {
    discord_rpc_update(NULL, "Launching...", "logo", "CeroClient",
                       NULL, NULL, g_start_timestamp);
}

static void rpc_set_login(void) {
    discord_rpc_update(NULL, "In login menu", "logo", "CeroClient",
                       NULL, NULL, g_start_timestamp);
}

static void rpc_set_idle(void) {
    discord_rpc_update(NULL, "Idling in menu", "logo", "CeroClient",
                       NULL, NULL, g_start_timestamp);
}

static void rpc_set_playing(const char* version) {
    char details[128];
    snprintf(details, sizeof(details), "Playing %s", version ? version : "Minecraft");
    discord_rpc_update(NULL, details, "logo", "CeroClient",
                       NULL, NULL, (int64_t)time(NULL));
}

#ifdef _WIN32
static WNDPROC g_orig_wndproc = NULL;
#endif

static void on_kill_game(const char* id, const char* req, void* arg) {
    (void)req;
    process_kill();
    ui_return(arg, id, 0, "\"ok\"");
}

static void on_game_status(const char* id, const char* req, void* arg) {
    (void)req;
    ui_return(arg, id, 0, game_running ? "true" : "false");
}

static void on_drag_start(const char* id, const char* req, void* arg) {
    (void)req;
    ui_drag_start(arg);
    ui_return(arg, id, 0, "\"ok\"");
}

typedef struct {
    char  version[64];
    void* ui;
    int* game_running;
} LaunchArgs;

static void launch_progress(const char* step, int pct, void* userdata) {
    LaunchUserdata* ud = (LaunchUserdata*)userdata;
    void* w = ud ? ud->ui : NULL;
    if (!w) return;

    char step_safe[256];
    size_t si = 0, di = 0;
    while (step[si] && di + 2 < sizeof(step_safe)) {
        if (step[si] == '"' || step[si] == '\\') step_safe[di++] = '\\';
        step_safe[di++] = step[si++];
    }
    step_safe[di] = '\0';

    char js[512];
    if (pct < 0) {
        snprintf(js, sizeof(js),
            "if(window.onLaunchError) onLaunchError(\"%s\")", step_safe);
    } else {
        snprintf(js, sizeof(js),
            "if(window.onLaunchProgress) onLaunchProgress(\"%s\",%d)",
            step_safe, pct);
    }
    ui_eval(w, js);
}

#ifdef _WIN32
static DWORD WINAPI launch_thread(LPVOID arg) {
#else
#include <pthread.h>
static void* launch_thread(void* arg) {
#endif
    LaunchArgs* la = (LaunchArgs*)arg;
    LaunchUserdata ud = { la->ui, la->game_running };
    launch_minecraft(la->version, launch_progress, &ud);
    free(la);
    return 0;
}

static void on_minimize(const char* id, const char* req, void* arg) {
    (void)req;
    ui_minimize_window(arg);
    ui_return(arg, id, 0, "\"ok\"");
}

static void on_close_to_tray(const char* id, const char* req, void* arg) {
    (void)req;
    ui_hide_window(arg);
    ui_return(arg, id, 0, "\"ok\"");
}

static void show_main_window(void) {
#ifdef _WIN32
    HWND hwnd = (HWND)ui_get_window(g_ui);
    if (!hwnd) return;
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
#else
    if (!g_ui) return;
    void* win = ui_get_window(g_ui);
    if (!win) return;
    extern void ui_show_window(void* w);
    ui_show_window(g_ui);
#endif
}

static void on_show_window(const char* id, const char* req, void* arg) {
    (void)req;
    show_main_window();
    ui_return(arg, id, 0, "\"ok\"");
}

static int build_settings_path(char *out, size_t sz) {
    int n = snprintf(out, sz, "%s%ssettings.json", client_path, PATH_SEP);
    return (n > 0 && (size_t)n < sz);
}

static void on_get_settings(const char* id, const char* req, void* arg) {
    (void)req;
    char path[MAX_PATH_SIZE + 32];
    if (!build_settings_path(path, sizeof(path))) {
        ui_return(arg, id, 0, "null");
        return;
    }

    FILE* f = fopen(path, "r");
    if (!f) { ui_return(arg, id, 0, "null"); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size <= 0 || size > 64 * 1024) { fclose(f); ui_return(arg, id, 0, "null"); return; }

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); ui_return(arg, id, 0, "null"); return; }

    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);

    cJSON* json = cJSON_Parse(buf);
    if (!json) { free(buf); ui_return(arg, id, 0, "null"); return; }
    cJSON_Delete(json);

    ui_return(arg, id, 0, buf);
    free(buf);
}

static void on_save_settings(const char* id, const char* req, void* arg) {
    const char* p = req;
    while (*p && *p != '{') p++;
    if (!*p) { ui_return(arg, id, 1, "\"parse_error\""); return; }

    cJSON* json = cJSON_Parse(p);
    if (!json) { ui_return(arg, id, 1, "\"invalid_json\""); return; }

    char path[MAX_PATH_SIZE + 32];
    if (!build_settings_path(path, sizeof(path))) {
        cJSON_Delete(json);
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    FILE* f = fopen(path, "r");
    cJSON* existing = NULL;
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);
        if (size > 0 && size < 64 * 1024) {
            char* buf = malloc(size + 1);
            if (buf) {
                size_t n = fread(buf, 1, size, f);
                buf[n] = '\0';
                existing = cJSON_Parse(buf);
                free(buf);
            }
        }
        fclose(f);
    }
    if (!existing) existing = cJSON_CreateObject();

    cJSON* item = NULL;
    cJSON_ArrayForEach(item, json) {
        cJSON* clone = cJSON_Duplicate(item, 1);
        cJSON_DeleteItemFromObject(existing, item->string);
        cJSON_AddItemToObject(existing, item->string, clone);
    }
    cJSON_Delete(json);

    char* out = cJSON_PrintUnformatted(existing);
    cJSON_Delete(existing);
    if (!out) { ui_return(arg, id, 1, "\"alloc_error\""); return; }

    f = fopen(path, "w");
    if (!f) { free(out); ui_return(arg, id, 1, "\"write_error\""); return; }
    fputs(out, f);
    fclose(f);
    free(out);

    ui_return(arg, id, 0, "\"ok\"");
}

#ifdef _WIN32
int local_bridge_port = 0;
static DWORD WINAPI bridge_thread(LPVOID arg) {
    (void)arg;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) return 1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_LOOPBACK;
    addr.sin_port = htons(0);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    int len = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &len);
    local_bridge_port = ntohs(addr.sin_port);

    log_msg("info", "Bridge TCP local en ecoute sur le port %d\n", local_bridge_port);

    while (1) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET) continue;

        char buf[256] = {0};
        int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            log_msg("debug", "Message recu du Java : %s\n", buf);
            if (strncmp(buf, "SHOW", 4) == 0) {
                show_main_window();
            }
        }

        closesocket(client_fd);
    }
    return 0;
}

#else

int local_bridge_port = 0;
static void* bridge_thread(void* arg) {
    (void)arg;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return NULL;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_LOOPBACK;
    addr.sin_port = htons(0);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    socklen_t len = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &len);
    local_bridge_port = ntohs(addr.sin_port);

    log_msg("info", "Bridge TCP local en écoute sur le port %d\n", local_bridge_port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char buf[256] = {0};
        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            log_msg("debug", "Message reçu du Java : %s\n", buf);
            if (strncmp(buf, "SHOW", 4) == 0) {
                show_main_window();
            }
        }

        close(client_fd);
    }
    return NULL;
}
#endif

static void on_launch(const char* id, const char* req, void* arg) {
    if (game_running) {
        ui_return(arg, id, 1, "\"already_running\"");
        return;
    }

    char version[64] = "";
    const char* p = req;
    while (*p && *p != '"') p++;
    if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof(version))
            version[i++] = *p++;
        version[i] = '\0';
    }

    if (version[0] == '\0') {
        ui_return(arg, id, 1, "\"no_version\"");
        return;
    }

    log_msg("info", "Launch requested for version: %s\n", version);

    LaunchArgs* la = malloc(sizeof(LaunchArgs));
    strncpy(la->version, version, sizeof(la->version) - 1);
    la->version[sizeof(la->version)-1] = '\0';
    la->ui = arg;
    la->game_running = (int*)&game_running;

#ifdef _WIN32
    HANDLE h = CreateThread(NULL, 0, launch_thread, la, 0, NULL);
    if (h) CloseHandle(h);
#else
    pthread_t t;
    pthread_create(&t, NULL, launch_thread, la);
    pthread_detach(t);
#endif

    ui_return(arg, id, 0, "\"ok\"");
}

static int file_exists(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

static void on_quit_app(const char* id, const char* req, void* arg) {
    (void)req;
    g_really_quit = 1;
    ui_return(arg, id, 0, "\"ok\"");
    ui_terminate(arg);
}

static void on_check_account(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    int ok = 0;
    if (ms_auth_validate(path) == 0) {
        ok = 1;
    } else if (ms_auth_refresh(path) == 0) {
        ok = 1;
    }

    if (ok) rpc_set_idle();
    else    rpc_set_login();

    ui_return(arg, id, 0, ok ? "true" : "false");
}

static void on_login_microsoft(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    int rc = ms_auth_login(path);
    if (rc == 0) {
        rpc_set_idle();
        ui_return(arg, id, 0, "\"ok\"");
    } else {
        char err[64];
        snprintf(err, sizeof(err), "\"error_%d\"", rc);
        ui_return(arg, id, 1, err);
    }
}

static void on_get_version(const char* id, const char* req, void* arg) {
    (void)req;
    char* version = config_get("lastVersion");
    if (!version) {
        log_msg("debug", "No version saved\n");
        ui_return(arg, id, 0, "null");
        return;
    }
    log_msg("debug", "Last version: %s\n", version);
    ui_return(arg, id, 0, version);
    free(version);
}

static void on_set_version(const char* id, const char* req, void* arg) {
    char version[64] = "";
    const char* p = req;
    while (*p && *p != '"') p++;
    if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof(version))
            version[i++] = *p++;
        version[i] = '\0';
    }

    if (version[0] == '\0') {
        ui_return(arg, id, 1, "\"no_version\"");
        return;
    }

    log_msg("debug", "Version set: %s\n", version);

    char json_val[80];
    snprintf(json_val, sizeof(json_val), "\"%s\"", version);
    config_set("lastVersion", json_val);

    ui_return(arg, id, 0, "\"ok\"");
}

static void on_get_account(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        ui_return(arg, id, 1, "\"not_found\"");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); ui_return(arg, id, 1, "\"alloc_error\""); return; }

    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    ui_return(arg, id, 0, buf);
    free(buf);
}

static void on_get_mc_token(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    log_msg("info", "Getting Minecraft Account Token...\n");

    if (ms_auth_validate(path) != 0) {
        if (ms_auth_refresh(path) != 0) {
            ui_return(arg, id, 1, "\"refresh_failed\"");
            log_msg("error", "Refresh Failed...\n");
            return;
        }
    }

    FILE* f = fopen(path, "r");
    if (!f) { ui_return(arg, id, 1, "\"not_found\""); log_msg("error", "Not found\n"); return; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    if (size <= 0 || size > 1024 * 1024) { fclose(f); ui_return(arg, id, 1, "\"invalid_size\""); log_msg("error", "Invalid Size\n"); return; }

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); ui_return(arg, id, 1, "\"alloc_error\""); log_msg("error", "Alloc Error\n"); return; }
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);

    const char* key = "\"mc_token\"";
    char* p = strstr(buf, key);
    if (!p) { free(buf); ui_return(arg, id, 1, "\"no_token\""); return; }
    p += strlen(key);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p != '"') { free(buf); ui_return(arg, id, 1, "\"parse_error\""); return; }
    p++;
    char* end = strchr(p, '"');
    if (!end) { free(buf); ui_return(arg, id, 1, "\"parse_error\""); return; }

    size_t tlen = (size_t)(end - p);
    char* out = malloc(tlen + 3);
    if (!out) { free(buf); ui_return(arg, id, 1, "\"alloc_error\""); return; }
    out[0] = '"';
    memcpy(out + 1, p, tlen);
    out[1 + tlen] = '"';
    out[2 + tlen] = '\0';

    ui_return(arg, id, 0, out);
    free(out);
    free(buf);
}

static int check_internet(void) {
#ifdef _WIN32
    DWORD flags;
    if (!InternetGetConnectedState(&flags, 0)) return 0;
    HINTERNET hNet = InternetOpenA("CeroClient", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return 0;
    HINTERNET hUrl = InternetOpenUrlA(hNet, "https://www.google.com/generate_204",
                                      NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    int ok = (hUrl != NULL);
    if (hUrl) InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);
    return ok;
#else
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("www.google.com", "80", &hints, &res) != 0 || !res) return 0;
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) { freeaddrinfo(res); return 0; }
    struct timeval tv = { .tv_sec = 3, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int ok = (connect(sock, res->ai_addr, res->ai_addrlen) == 0);
    close(sock);
    freeaddrinfo(res);
    return ok;
#endif
}

static int url_is_http_or_https(const char* url) {
    return (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0);
}

#ifndef _WIN32
static int try_exec_opener(const char* opener, const char* url) {
    char* const argv[] = { (char*)opener, (char*)url, NULL };
    execvp(opener, argv);
    return -1; 
}

static void open_url_no_shell(const char* url) {
    pid_t pid = fork();
    if (pid < 0) return;

    if (pid == 0) {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            try_exec_opener("xdg-open", url);
            try_exec_opener("sensible-browser", url);
            try_exec_opener("x-www-browser", url);
            _exit(127);
        }
        _exit(0);
    }

    waitpid(pid, NULL, 0);
}
#endif

static void on_logout_account(const char* id, const char* req, void* arg) {
    (void)req;

    char path[MAX_PATH_SIZE + 16];
    if (!build_account_path(path, sizeof(path))) {
        ui_return(arg, id, 1, "\"path_error\"");
        return;
    }

    if (file_exists(path)) {
        if (remove(path) != 0) {
            log_msg("error", "logout_account: impossible de supprimer %s\n", path);
            ui_return(arg, id, 1, "\"remove_failed\"");
            return;
        }
    }

    log_msg("info", "Compte deconnecte (account.json supprime)\n");
    rpc_set_login();
    ui_return(arg, id, 0, "\"ok\"");
}

static void on_shell_open(const char* id, const char* req, void* arg) {
    char url[2048] = "";
    const char* p = req;
    while (*p && *p != '"') p++;
    if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof(url))
            url[i++] = *p++;
        url[i] = '\0';
    }
    if (url[0] && url_is_http_or_https(url)) {
#ifdef _WIN32
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
        open_url_no_shell(url);
#endif
    } else if (url[0]) {
        log_msg("warn", "shell_open: URL refusee (schema non autorise): %s\n", url);
    }
    ui_return(arg, id, 0, "\"ok\"");
}

static void on_check_internet(const char* id, const char* req, void* arg) {
    (void)req;
    ui_return(arg, id, 0, check_internet() ? "true" : "false");
}

static void get_exe_path(char *out, size_t sz) {
#ifdef _WIN32
    GetModuleFileNameA(NULL, out, (DWORD)sz);

#elif defined(__FreeBSD__) || defined(__DragonFly__)
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t cb = sz;
    out[0] = '\0';
    if (sysctl(mib, 4, out, &cb, NULL, 0) != 0 || out[0] == '\0')
        snprintf(out, sz, "./CeroClient");

#elif defined(__OpenBSD__)
    ssize_t n = readlink("/proc/curproc/file", out, sz - 1);
    if (n > 0) out[n] = '\0';
    else snprintf(out, sz, "./CeroClient");

#else
    ssize_t n = readlink("/proc/self/exe", out, sz - 1);
    if (n > 0) out[n] = '\0';
    else snprintf(out, sz, "./CeroClient");
#endif
}



static void quit_app(void) {
    g_really_quit = 1;
    if (g_ui) ui_terminate(g_ui);
}

#ifdef _WIN32
static LRESULT CALLBACK subclass_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    LRESULT tr;
    if (tray_handle_message(hwnd, msg, wp, lp, &tr)) return tr;

    if (msg == CERO_MSG_SHOW) {
        show_main_window();
        return 0;
    }

    if (msg == WM_CLOSE && !g_really_quit) {
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }

    return CallWindowProcA(g_orig_wndproc, hwnd, msg, wp, lp);
}
#endif

#ifdef __ANDROID__
int cero_main(int argc, char** argv) {
    
}
#else
int main(int argc, char** argv){
    setlocale(LC_NUMERIC, "C");

    // Gestion des arguments de la ligne de commande
    const char* assets_path = "assets.dat";
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--customAssetsPath=", 19) == 0) {
            assets_path = argv[i] + 19;
        }
    }

    if (single_instance_check()) {
        return 0;
    }

#ifdef _WIN32
    CreateThread(NULL, 0, bridge_thread, NULL, 0, NULL);
    Sleep(100);
#else
    pthread_t bridge_tid;
    pthread_create(&bridge_tid, NULL, bridge_thread, NULL);
    pthread_detach(bridge_tid);
    usleep(100000);
#endif

    
    single_instance_write_port(local_bridge_port);

    init_config();

#ifndef _WIN32
    if (client_path[0] == '\0' || strcmp(client_path, "") == 0) {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(client_path, MAX_PATH_SIZE, "%s/.ceroclient", home);
        } else {
            snprintf(client_path, MAX_PATH_SIZE, "./.ceroclient");
        }
#ifdef __BSD__
        mkdir(client_path, 0755);
#else
        mkdir(client_path, 0755);
#endif
    }
#endif

    install_cleanup_handlers();

    log_msg("info", "Loading Assets into RAM from %s...\n", assets_path);

    char exe_path[1024];
    get_exe_path(exe_path, sizeof(exe_path));

    if (!assets_load(assets_path, exe_path)) {
        log_msg("error", "Failed to load %s\n", assets_path);
        return 1;
    }

    log_msg("info", "Starting CeroClient ...\n");

    g_start_timestamp = (int64_t)time(NULL);
    if (discord_rpc_init("1376179097113333881") == 0) {
        log_msg("info", "Discord RPC connected\n");
        rpc_set_launching();
    } else {
        log_msg("warn", "Discord not running or RPC failed\n");
    }

    char url[1024];
    snprintf(url, sizeof(url), "cero:///app/index.html");

    void* w = ui_create("CeroClient");

#if defined(__linux__) || defined(__BSD__)
    ui_watch_system_theme(w);
#endif

    ui_set_frameless(w);

    const char* icon_path = NULL;
    ui_set_icon(w, icon_path);

    ui_bind(w, "launch_mc",     on_launch,        w);
    ui_bind(w, "drag_start",    on_drag_start,    w);
    ui_bind(w, "checkAccount",  on_check_account, w);
    ui_bind(w, "checkInternet", on_check_internet,w);
    ui_bind(w, "loginMicrosoft", on_login_microsoft, w);
    ui_bind(w, "logout_account", on_logout_account, w);
    ui_bind(w, "getAccount", on_get_account, w);
    ui_bind(w, "getMcToken", on_get_mc_token, w);
    ui_bind(w, "getVersion", on_get_version, w);
    ui_bind(w, "setVersion", on_set_version, w);
    ui_bind(w, "shell_open", on_shell_open, w);
    ui_bind(w, "get_settings",  on_get_settings,  w);
    ui_bind(w, "save_settings", on_save_settings, w);
    ui_bind(w, "minimize_window", on_minimize,      w);
    ui_bind(w, "show_window", on_show_window, w);
    ui_bind(w, "close_to_tray",   on_close_to_tray, w);
    ui_bind(w, "kill_game", on_kill_game, w);
    ui_bind(w, "quit_app", on_quit_app, w);
    ui_enable_js_console(w);
    ui_navigate(w, url);
    ui_lockdown(w);

    g_ui = w;

#ifdef _WIN32
    HWND hwnd = (HWND)ui_get_window(w);
    if (hwnd) {
        g_orig_wndproc = (WNDPROC)SetWindowLongPtrA(
            hwnd, GWLP_WNDPROC, (LONG_PTR)subclass_wndproc);

        if (tray_init(hwnd, icon_path, show_main_window, quit_app) != 0) {
            log_msg("warn", "Tray icon init failed\n");
        }
    }
#else
    if (tray_init(ui_get_window(w), icon_path, show_main_window, quit_app) != 0) {
        log_msg("warn", "Tray icon init failed\n");
    }
#endif

    ui_run(w);

    tray_shutdown();
    single_instance_release();

    discord_rpc_shutdown();
    return 0;
}
#endif