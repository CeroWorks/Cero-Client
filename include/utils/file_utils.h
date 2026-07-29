#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/logger.h"

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0755)
#endif

static inline int ensure_directory_exists(const char* path) {
    struct stat st = {0};

    log_msg("info", "Checking if %s exist ...\n", path);

    if (stat(path, &st) == -1) {
        if (MKDIR(path) == 0) {
            log_msg("succes", "%s succefully created\n", path);
            return 1;
        } else {
            log_msg("error", "Error while creating the folder : %s\n", path);
            return 0;
        }
    }

    log_msg("info", "%s already exist\n", path);
    return 1;
}

static inline void ensure_parent_dirs(const char* filepath) {
    char tmp[1024];
    strncpy(tmp, filepath, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char c = *p;
            *p = '\0';

            struct stat st = {0};
            if (stat(tmp, &st) == -1) {
                MKDIR(tmp);
            }

            *p = c;
        }
    }
}

#endif
