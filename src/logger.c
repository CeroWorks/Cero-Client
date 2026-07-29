#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <time.h>
  #include <stdint.h>
#endif

#define RESET   "\033[0m"
#define ROUGE   "\033[1;31m"
#define VERT    "\033[1;32m"
#define JAUNE   "\033[1;33m"
#define BLEU    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define BLANC   "\033[1;37m"
#define GRIS    "\033[1;90m"

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

void log_msg(const char* type, const char* format, ...) {
    if (start_time == 0) init_logger();
    if (strcmp(type, "debug") == 0) return;

    double elapsed = (double)(now_ms() - start_time) / 1000.0;
    printf("[%6.3fs] ", elapsed);
    if      (strcmp(type, "error")  == 0) printf(ROUGE "[ERROR] " RESET);
    else if (strcmp(type, "warn")   == 0) printf(JAUNE "[WARN]  " RESET);
    else if (strcmp(type, "succes") == 0) printf(VERT  "[SUCCES]" RESET " ");
    else if (strcmp(type, "info")   == 0) printf(BLEU  "[INFO]  " RESET);
    else if (strcmp(type, "js")     == 0) printf(JAUNE "[JS]    " RESET);
    else                                   printf("[LOG]   ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}