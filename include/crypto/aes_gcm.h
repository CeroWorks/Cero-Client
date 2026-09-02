#ifndef AES_GCM_H
#define AES_GCM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int aes_gcm_decrypt(const uint8_t *key,   size_t key_len,
                    const uint8_t *nonce, size_t nonce_len,
                    const uint8_t *aad,   size_t aad_len,
                    const uint8_t *ct,    size_t ct_len,
                    const uint8_t *tag,   size_t tag_len,
                    uint8_t *out_plain);

#ifdef __cplusplus
}
#endif

#endif
