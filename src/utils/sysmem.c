#include "../../include/utils/sysmem.h"
#include "../../include/platform/platform_defines.h"

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
#else
  #include <stdio.h>
  #include <string.h>
  #include <unistd.h>
#endif

long sysmem_total_mb(void) {
#if defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) return 0;
    return (long)(status.ullTotalPhys / (1024 * 1024));

#elif defined(__APPLE__)
    int64_t mem_bytes = 0;
    size_t len = sizeof(mem_bytes);
    if (sysctlbyname("hw.memsize", &mem_bytes, &len, NULL, 0) != 0) return 0;
    return (long)(mem_bytes / (1024 * 1024));

#else
    FILE* f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        long kb = 0;
        int found = 0;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "MemTotal: %ld kB", &kb) == 1) { found = 1; break; }
        }
        fclose(f);
        if (found) return kb / 1024;
    }

    #if defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        return (long)(((long long)pages * (long long)page_size) / (1024 * 1024));
    }
    #endif

    return 0;
#endif
}
