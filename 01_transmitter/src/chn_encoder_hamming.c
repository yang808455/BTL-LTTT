#include "chn_encoder_hamming.h"

uint8_t hamming84_encode_nibble(uint8_t nibble) {
    /* Trích xuất 4 bit dữ liệu: d3, d2, d1, d0 */
    uint8_t d3 = (nibble >> 3) & 1;
    uint8_t d2 = (nibble >> 2) & 1;
    uint8_t d1 = (nibble >> 1) & 1;
    uint8_t d0 = (nibble >> 0) & 1;

    /* Tính các bit kiểm tra theo ma trận hệ thống H = [P^T | I] */
    uint8_t p1 = d0 ^ d1 ^ d3;
    uint8_t p2 = d0 ^ d2 ^ d3;
    uint8_t p3 = d1 ^ d2 ^ d3;

    /* Parity tổng thể (Overall parity for SEC-DED) */
    uint8_t p4 = d3 ^ d2 ^ d1 ^ d0 ^ p1 ^ p2 ^ p3;

    /* Ghép thành từ mã hệ thống [d3 d2 d1 d0 p1 p2 p3 p4] */
    uint8_t codeword = ((nibble & 0x0F) << 4) |
                       (p1 << 3) |
                       (p2 << 2) |
                       (p3 << 1) |
                       (p4 << 0);
    return codeword;
}

size_t hamming84_encode_buffer(const uint8_t *input, size_t input_len,
                              uint8_t *out_codewords, size_t max_out_bytes) {
    if (!input || !out_codewords || max_out_bytes < input_len * 2) return 0;

    size_t out_idx = 0;
    for (size_t i = 0; i < input_len; i++) {
        uint8_t b = input[i];
        uint8_t high_nibble = (b >> 4) & 0x0F;
        uint8_t low_nibble  = b & 0x0F;

        out_codewords[out_idx++] = hamming84_encode_nibble(high_nibble);
        out_codewords[out_idx++] = hamming84_encode_nibble(low_nibble);
    }
    return out_idx;
}
