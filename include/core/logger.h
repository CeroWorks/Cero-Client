#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

void log_msg(const char* type, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
