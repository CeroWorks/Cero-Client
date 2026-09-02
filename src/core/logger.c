#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <time.h>
  #include <stdint.h>
#endif
#include <ctype.h>

#define RESET "\033[0m"

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

#define GRAY "\033[90m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

static unsigned long long start_time = 0;

static unsigned long long now_ms(void) {
#ifdef _WIN32
    return (unsigned long long)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000ULL
         + (unsigned long long)(ts.tv_nsec / 1000000LL);
#endif
}

void init_logger() {
    start_time = now_ms();
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode))
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

static int type_equals(const char* a, const char* b) {
    if (!a || !b) return 0;

    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }

    return *a == '\0' && *b == '\0';
}

void log_msg(const char* type, const char* format, ...) {
    if (start_time == 0) init_logger();
    if (type_equals(type, "debug")) return;

    double elapsed = (double)(now_ms() - start_time) / 1000.0;

    printf("[%6.3fs] ", elapsed);

    if      (type_equals(type, "error")) printf(RED "[ERROR] " RESET);
    else if (type_equals(type, "warn")) printf(YELLOW "[WARN] " RESET);
    else if (type_equals(type, "succes")) printf(GREEN "[SUCCES] " RESET);
    else if (type_equals(type, "info")) printf(BLUE "[INFO] " RESET);
    else if (type_equals(type, "uncaught")) printf(BRIGHT_YELLOW "[UNCAUGHT] " RESET);
    else if (type_equals(type, "promise")) printf(BRIGHT_MAGENTA "[PROMISE] " RESET);
    else printf(BRIGHT_BLUE "[LOG] " RESET);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    fflush(stdout);
}
