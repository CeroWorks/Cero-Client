#pragma once
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
extern HANDLE g_mc_process;
#else
#include <sys/types.h>
extern pid_t g_mc_process;
#endif

int process_run(const char* exe, const char* const argv[]);
void process_kill(void);
