#include "../include/aes_gcm.h"
#include <string.h>
#include <stdint.h>



static const uint8_t SBOX[256] = {
0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t RCON[15] = {
    0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36,0x6c,0xd8,0xab,0x4d
};

#define AES_BLOCK 16
#define AES_ROUNDS_256 14
#define AES_KS_SIZE (AES_BLOCK * (AES_ROUNDS_256 + 1)) 

static uint8_t xtime(uint8_t x) {
    return (uint8_t)((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

static void key_expansion_256(const uint8_t key[32], uint8_t out[AES_KS_SIZE]) {
    memcpy(out, key, 32);
    uint8_t temp[4];
    int i = 32;
    int rcon_i = 1;
    while (i < AES_KS_SIZE) {
        memcpy(temp, out + i - 4, 4);
        if (i % 32 == 0) {
            uint8_t t = temp[0];
            temp[0] = SBOX[temp[1]] ^ RCON[rcon_i++];
            temp[1] = SBOX[temp[2]];
            temp[2] = SBOX[temp[3]];
            temp[3] = SBOX[t];
        } else if (i % 32 == 16) {
            temp[0] = SBOX[temp[0]];
            temp[1] = SBOX[temp[1]];
            temp[2] = SBOX[temp[2]];
            temp[3] = SBOX[temp[3]];
        }
        for (int j = 0; j < 4; j++)
            out[i + j] = out[i - 32 + j] ^ temp[j];
        i += 4;
    }
}

static void aes_encrypt_block(const uint8_t in[16], uint8_t out[16],
                              const uint8_t ks[AES_KS_SIZE])
{
    uint8_t s[16];
    memcpy(s, in, 16);
    for (int i = 0; i < 16; i++) s[i] ^= ks[i];

    for (int r = 1; r < AES_ROUNDS_256; r++) {
        
        for (int i = 0; i < 16; i++) s[i] = SBOX[s[i]];
        
        uint8_t t;
        t = s[1];  s[1]  = s[5];  s[5]  = s[9];  s[9]  = s[13]; s[13] = t;
        t = s[2];  s[2]  = s[10]; s[10] = t;
        t = s[6];  s[6]  = s[14]; s[14] = t;
        t = s[15]; s[15] = s[11]; s[11] = s[7];  s[7]  = s[3];  s[3]  = t;
        
        for (int c = 0; c < 4; c++) {
            uint8_t *col = s + 4 * c;
            uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
            uint8_t h = a0 ^ a1 ^ a2 ^ a3;
            col[0] ^= h ^ xtime((uint8_t)(a0 ^ a1));
            col[1] ^= h ^ xtime((uint8_t)(a1 ^ a2));
            col[2] ^= h ^ xtime((uint8_t)(a2 ^ a3));
            col[3] ^= h ^ xtime((uint8_t)(a3 ^ a0));
        }
        
        for (int i = 0; i < 16; i++) s[i] ^= ks[16 * r + i];
    }
    
    for (int i = 0; i < 16; i++) s[i] = SBOX[s[i]];
    uint8_t t;
    t = s[1];  s[1]  = s[5];  s[5]  = s[9];  s[9]  = s[13]; s[13] = t;
    t = s[2];  s[2]  = s[10]; s[10] = t;
    t = s[6];  s[6]  = s[14]; s[14] = t;
    t = s[15]; s[15] = s[11]; s[11] = s[7];  s[7]  = s[3];  s[3]  = t;
    for (int i = 0; i < 16; i++) s[i] ^= ks[16 * AES_ROUNDS_256 + i];

    memcpy(out, s, 16);
}



static void gf_mul(uint8_t Z[16], const uint8_t H[16]) {
    uint8_t V[16];
    uint8_t R = 0xe1;
    uint8_t Zout[16] = {0};
    memcpy(V, H, 16);

    for (int i = 0; i < 128; i++) {
        
        uint8_t zbit = (uint8_t)((Z[i >> 3] >> (7 - (i & 7))) & 1);
        uint8_t mask = (uint8_t)(-(int8_t)zbit);
        for (int k = 0; k < 16; k++) Zout[k] ^= (uint8_t)(V[k] & mask);

        
        uint8_t lsb = (uint8_t)(V[15] & 1);
        for (int k = 15; k > 0; k--)
            V[k] = (uint8_t)((V[k] >> 1) | ((V[k - 1] & 1) << 7));
        V[0] >>= 1;
        uint8_t lmask = (uint8_t)(-(int8_t)lsb);
        V[0] ^= (uint8_t)(R & lmask);
    }
    memcpy(Z, Zout, 16);
}

static void ghash_update(uint8_t Y[16], const uint8_t H[16],
                         const uint8_t *data, size_t len)
{
    while (len >= 16) {
        for (int i = 0; i < 16; i++) Y[i] ^= data[i];
        gf_mul(Y, H);
        data += 16; len -= 16;
    }
    if (len) {
        uint8_t buf[16] = {0};
        memcpy(buf, data, len);
        for (int i = 0; i < 16; i++) Y[i] ^= buf[i];
        gf_mul(Y, H);
    }
}



static void inc32(uint8_t ctr[16]) {
    for (int i = 15; i >= 12; i--) {
        if (++ctr[i] != 0) break;
    }
}

static int ct_memcmp(const void *a, const void *b, size_t n) {
    const volatile uint8_t *x = (const volatile uint8_t *)a;
    const volatile uint8_t *y = (const volatile uint8_t *)b;
    uint8_t r = 0;
    for (size_t i = 0; i < n; i++) r |= (uint8_t)(x[i] ^ y[i]);
    return r;
}

static void secure_wipe(void *p, size_t n) {
    if (!p) return;
    volatile uint8_t *v = (volatile uint8_t *)p;
    while (n--) *v++ = 0;
}

int aes_gcm_decrypt(const uint8_t *key,   size_t key_len,
                    const uint8_t *nonce, size_t nonce_len,
                    const uint8_t *aad,   size_t aad_len,
                    const uint8_t *ct,    size_t ct_len,
                    const uint8_t *tag,   size_t tag_len,
                    uint8_t *out_plain)
{
    if (key_len != 32 || nonce_len != 12 || tag_len != 16) return 0;
    if (ct_len && !ct) return 0;
    if (aad_len && !aad) return 0;

    uint8_t ks[AES_KS_SIZE];
    key_expansion_256(key, ks);

    
    uint8_t H[16] = {0};
    aes_encrypt_block(H, H, ks);

    
    uint8_t J0[16] = {0};
    memcpy(J0, nonce, 12);
    J0[15] = 1;

    
    uint8_t Y[16] = {0};
    ghash_update(Y, H, aad, aad_len);
    ghash_update(Y, H, ct,  ct_len);

    uint8_t lens[16];
    uint64_t a_bits = (uint64_t)aad_len * 8;
    uint64_t c_bits = (uint64_t)ct_len  * 8;
    for (int i = 0; i < 8; i++) lens[i]     = (uint8_t)(a_bits >> (56 - i * 8));
    for (int i = 0; i < 8; i++) lens[8 + i] = (uint8_t)(c_bits >> (56 - i * 8));
    for (int i = 0; i < 16; i++) Y[i] ^= lens[i];
    gf_mul(Y, H);

    
    uint8_t S[16];
    aes_encrypt_block(J0, S, ks);
    for (int i = 0; i < 16; i++) S[i] ^= Y[i];

    
    int auth_fail = ct_memcmp(S, tag, 16);

    
    uint8_t ctr[16];
    memcpy(ctr, J0, 16);
    inc32(ctr);

    uint8_t stream[16];
    size_t off = 0;
    while (off < ct_len) {
        aes_encrypt_block(ctr, stream, ks);
        inc32(ctr);
        size_t n = ct_len - off;
        if (n > 16) n = 16;
        for (size_t i = 0; i < n; i++)
            out_plain[off + i] = (uint8_t)(ct[off + i] ^ stream[i]);
        off += n;
    }

    
    if (auth_fail) {
        secure_wipe(out_plain, ct_len);
    }

    
    secure_wipe(ks,     sizeof(ks));
    secure_wipe(H,      sizeof(H));
    secure_wipe(J0,     sizeof(J0));
    secure_wipe(Y,      sizeof(Y));
    secure_wipe(S,      sizeof(S));
    secure_wipe(stream, sizeof(stream));
    secure_wipe(ctr,    sizeof(ctr));
    secure_wipe(lens,   sizeof(lens));

    return auth_fail == 0;
}
