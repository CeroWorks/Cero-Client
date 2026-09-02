#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SHA1_DIGEST_LENGTH 20

void sha1(const uint8_t *data, size_t len, uint8_t out[SHA1_DIGEST_LENGTH]);

bool sha1_file_matches(const char *path, const char *expected_hex);

#endif
