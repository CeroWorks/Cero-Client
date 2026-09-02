#ifndef CERO_AUTH_HANDLERS_H
#define CERO_AUTH_HANDLERS_H

void on_check_account(const char* id, const char* req, void* arg);
void on_login_microsoft(const char* id, const char* req, void* arg);
void on_get_account(const char* id, const char* req, void* arg);
void on_get_mc_token(const char* id, const char* req, void* arg);
void on_logout_account(const char* id, const char* req, void* arg);

#endif /* CERO_AUTH_HANDLERS_H */
