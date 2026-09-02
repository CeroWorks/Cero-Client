#ifndef CERO_MC_JSON_H
#define CERO_MC_JSON_H

#include <stddef.h>

/* Extracts the string value of `key` from a flat JSON blob without a full
 * parser. Returns `out` on success, NULL if not found. */
const char* json_str(const char* json, const char* key, char* out, size_t outsz);

/* Reads an entire file into a malloc'd, NUL-terminated buffer.
 * Caller must free() the result. Returns NULL on failure. */
char* read_file(const char* path);

#endif /* CERO_MC_JSON_H */
