#ifndef MS_AUTH_H
#define MS_AUTH_H

int ms_auth_login(const char* save_path);

int ms_auth_validate(const char* save_path);
int ms_auth_refresh(const char* path);

#endif
