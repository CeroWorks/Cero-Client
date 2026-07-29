#include "../include/sha1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ROL(x,n) (((x)<<(n))|((x)>>(32-(n))))

void sha1(const uint8_t *data, size_t len, uint8_t out[SHA1_DIGEST_LENGTH]) {
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE,
             h3 = 0x10325476, h4 = 0xC3D2E1F0;

    size_t total = len + 1;
    while (total % 64 != 56) total++;
    total += 8;

    uint8_t *msg = (uint8_t*)calloc(1, total);
    memcpy(msg, data, len);
    msg[len] = 0x80;
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) msg[total-1-i] = (uint8_t)((bits >> (i*8)) & 0xff);

    for (size_t off = 0; off < total; off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)msg[off+i*4]<<24) | ((uint32_t)msg[off+i*4+1]<<16) |
                   ((uint32_t)msg[off+i*4+2]<<8) | (uint32_t)msg[off+i*4+3];
        }
        for (int i = 16; i < 80; i++) {
            w[i] = ROL(w[i-3]^w[i-8]^w[i-14]^w[i-16], 1);
        }

        uint32_t a=h0,b=h1,c=h2,d=h3,e=h4;
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | ((~b) & d);      k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;                 k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;                 k = 0xCA62C1D6; }

            uint32_t temp = ROL(a,5) + f + e + k + w[i];
            e = d; d = c; c = ROL(b,30); b = a; a = temp;
        }

        h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
    }
    free(msg);

    uint32_t h[5] = {h0,h1,h2,h3,h4};
    for (int i = 0; i < 5; i++) {
        out[i*4]   = (uint8_t)((h[i]>>24)&0xff);
        out[i*4+1] = (uint8_t)((h[i]>>16)&0xff);
        out[i*4+2] = (uint8_t)((h[i]>>8)&0xff);
        out[i*4+3] = (uint8_t)(h[i]&0xff);
    }
}

bool sha1_file_matches(const char *path, const char *expected_hex) {
    if (!expected_hex || expected_hex[0] == '\0') {
        
        return true;
    }
    if (strlen(expected_hex) != SHA1_DIGEST_LENGTH * 2) return false;

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return false; }
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = (uint8_t*)malloc((size_t)size);
    if (!buf) { fclose(f); return false; }
    size_t read_n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (read_n != (size_t)size) { free(buf); return false; }

    uint8_t digest[SHA1_DIGEST_LENGTH];
    sha1(buf, (size_t)size, digest);
    free(buf);

    char hex[SHA1_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
        snprintf(hex + i*2, 3, "%02x", digest[i]);
    }

    for (int i = 0; i < SHA1_DIGEST_LENGTH * 2; i++) {
        if (tolower((unsigned char)hex[i]) != tolower((unsigned char)expected_hex[i])) {
            return false;
        }
    }
    return true;
}
