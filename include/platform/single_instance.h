#ifndef SINGLE_INSTANCE_H
#define SINGLE_INSTANCE_H

#ifdef _WIN32
#include <windows.h>

#define CERO_MSG_SHOW (WM_APP + 100)
#endif

int single_instance_check(void);
void single_instance_release(void);
void single_instance_write_port(int port);   // nouveau

#endif
