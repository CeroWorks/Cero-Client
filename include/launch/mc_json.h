#ifndef CERO_MC_JSON_H
#define CERO_MC_JSON_H

#include <stddef.h>

const char* json_str(const char* json, const char* key, char* out, size_t outsz);

char* read_file(const char* path);

#endif 
