#include "../../include/platform/platform_defines.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <pthread.h>
#endif

#include "../../include/ipc/bridge.h"
#include "../../include/launch/launch_minecraft.h"
#include "../../include/ipc/window_handlers.h"
#include "../../include/core/logger.h"
#include <string.h>

int local_bridge_port = 0;

#ifdef _WIN32
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

    log_msg("info", "Local TCP bridge listening on the port %d\n", local_bridge_port);

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

void bridge_start(void) {
    CreateThread(NULL, 0, bridge_thread, NULL, 0, NULL);
    Sleep(100);
}

#else

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

    log_msg("info", "Local TCP bridge listening on the port %d\n", local_bridge_port);

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

void bridge_start(void) {
    pthread_t bridge_tid;
    pthread_create(&bridge_tid, NULL, bridge_thread, NULL);
    pthread_detach(bridge_tid);
    usleep(100000);
}

#endif
