#ifndef SHA256_H
#define SHA256_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define SHA256_DIGEST_LENGTH 32

void sha256(const uint8_t *data, size_t len, uint8_t out[32]);

#endif
