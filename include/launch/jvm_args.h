#ifndef CERO_LAUNCH_JVM_ARGS_H
#define CERO_LAUNCH_JVM_ARGS_H

#include <stddef.h>
#include "classpath.h"

#if defined(_WIN32)
  #define CP_SEP ";"
#else
  #define CP_SEP ":"
#endif

/* Builds the ':'/';' separated classpath string: optional cero agent jar,
 * then every resolved library, then the client jar. */
void build_classpath(char* out, size_t outsz,
                     int has_cero, const char* cero_jar_path,
                     const CpLib* libs, int libs_count,
                     const char* client_jar);

typedef struct {
    const char* java_exe;
    const char* classpath;
    const char* game_main_class;
    int         has_cero;
    const char* username;
    const char* version_id_for_args;
    const char* client_dir;
    const char* asset_index;
    const char* uuid;
    const char* token;
    const char* vanilla_version;
    int         bridge_port;
} LaunchParams;

/* Fills `argv` (NULL-terminated, at most max_argv entries incl. the NULL)
 * with the full java invocation for `p`. `bridge_port_buf` must outlive
 * the argv array (it's referenced by pointer, not copied). Returns the
 * number of non-NULL entries written. */
int build_launch_argv(const LaunchParams* p, const char** argv, int max_argv,
                      char* bridge_port_buf, size_t bridge_port_buf_sz);

#endif /* CERO_LAUNCH_JVM_ARGS_H */
