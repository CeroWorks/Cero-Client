#include "../../include/platform/platform_defines.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #ifdef __BSD__
    #include <sys/sysctl.h>
  #endif
  #ifdef __APPLE__
    #include <mach-o/dyld.h>
  #endif
#endif

#include "../../include/platform/paths.h"
#include "../../include/config/config.h"
#include <stdio.h>

int build_account_path(char *out, size_t sz) {
    int n = snprintf(out, sz, "%s%saccount.json", client_path, PATH_SEP);
    return (n > 0 && (size_t)n < sz);
}

int build_settings_path(char *out, size_t sz) {
    int n = snprintf(out, sz, "%s%ssettings.json", client_path, PATH_SEP);
    return (n > 0 && (size_t)n < sz);
}

int file_exists(const char* path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

void get_exe_path(char *out, size_t sz) {
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

#elif defined(__APPLE__)
    uint32_t size = (uint32_t)sz;
    if (_NSGetExecutablePath(out, &size) != 0)
        snprintf(out, sz, "./CeroClient");

#else
    ssize_t n = readlink("/proc/self/exe", out, sz - 1);
    if (n > 0) out[n] = '\0';
    else snprintf(out, sz, "./CeroClient");
#endif
}
