#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PATH_SIZE 4096

extern char client_path[MAX_PATH_SIZE];

void init_config(void);

int config_add(const char* key, const char* value);

int config_set(const char* key, const char* value);

int config_delete(const char* key);

char* config_get(const char* key);

#endif
