#include "chn_encoder_cyclic.h"

uint8_t cyclic74_encode_nibble(uint8_t nibble) {
    nibble &= 0x0F;
    uint8_t v = nibble << 3; /* Dịch trái 3 bit: x^3 * m(x) */

    /* Chia đa thức trên GF(2) cho g(x) = x^3 + x + 1 (0b1011) */
    for (int i = 6; i >= 3; i--) {
        if ((v >> i) & 1) {
            v ^= (CYCLIC_POLY << (i - 3));
        }
    }
    uint8_t remainder = v & 0x07; /* 3 bit dư r(x) */

    /* Từ mã hệ thống: [0 m3 m2 m1 m0 r2 r1 r0] */
    uint8_t codeword = (nibble << 3) | remainder;
    return codeword;
}

size_t cyclic74_encode_buffer(const uint8_t *input, size_t input_len,
                             uint8_t *out_codewords, size_t max_out_bytes) {
    if (!input || !out_codewords || max_out_bytes < input_len * 2) return 0;

    size_t out_idx = 0;
    for (size_t i = 0; i < input_len; i++) {
        uint8_t b = input[i];
        uint8_t high_nibble = (b >> 4) & 0x0F;
        uint8_t low_nibble  = b & 0x0F;

        out_codewords[out_idx++] = cyclic74_encode_nibble(high_nibble);
        out_codewords[out_idx++] = cyclic74_encode_nibble(low_nibble);
    }
    return out_idx;
}
