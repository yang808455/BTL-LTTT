#include "chn_decoder_hamming.h"

/* Bảng ánh xạ Syndrome S (0..7) sang vị trí bit cần lật (khi P_all == 1) */
static const int8_t SYNDROME_TO_BIT_MAP[8] = {
    0, /* S = 0: lỗi ở bit 0 (p4) */
    3, /* S = 1: lỗi ở bit 3 (p1) */
    2, /* S = 2: lỗi ở bit 2 (p2) */
    4, /* S = 3: lỗi ở bit 4 (d0) */
    1, /* S = 4: lỗi ở bit 1 (p3) */
    5, /* S = 5: lỗi ở bit 5 (d1) */
    6, /* S = 6: lỗi ở bit 6 (d2) */
    7  /* S = 7: lỗi ở bit 7 (d3) */
};

hamming_decode_result_t hamming84_decode_codeword(uint8_t codeword) {
    hamming_decode_result_t res;
    res.decoded_nibble = 0;
    res.status = CHN_STATUS_OK;
    res.error_bit_pos = -1;

    uint8_t d3 = (codeword >> 7) & 1;
    uint8_t d2 = (codeword >> 6) & 1;
    uint8_t d1 = (codeword >> 5) & 1;
    uint8_t d0 = (codeword >> 4) & 1;
    uint8_t p1 = (codeword >> 3) & 1;
    uint8_t p2 = (codeword >> 2) & 1;
    uint8_t p3 = (codeword >> 1) & 1;
    uint8_t p4 = (codeword >> 0) & 1;

    /* Tính các bit hội chứng Syndrome */
    uint8_t s1 = d0 ^ d1 ^ d3 ^ p1;
    uint8_t s2 = d0 ^ d2 ^ d3 ^ p2;
    uint8_t s3 = d1 ^ d2 ^ d3 ^ p3;
    uint8_t p_all = d3 ^ d2 ^ d1 ^ d0 ^ p1 ^ p2 ^ p3 ^ p4;

    uint8_t syndrome = (s3 << 2) | (s2 << 1) | s1;
    res.syndrome = syndrome;

    if (syndrome == 0 && p_all == 0) {
        /* Không có lỗi */
        res.status = CHN_STATUS_OK;
        res.decoded_nibble = (codeword >> 4) & 0x0F;
    } else if (p_all == 1) {
        /* Phát hiện lỗi 1-bit -> SỬA LỖI ĐƯỢC */
        int8_t flip_bit = SYNDROME_TO_BIT_MAP[syndrome];
        res.error_bit_pos = flip_bit;
        res.status = CHN_STATUS_CORRECTED;

        uint8_t corrected_codeword = codeword ^ (1 << flip_bit);
        res.decoded_nibble = (corrected_codeword >> 4) & 0x0F;
    } else {
        /* syndrome != 0 nhưng p_all == 0 -> Lỗi kép (2-bit error), không thể sửa */
        res.status = CHN_STATUS_UNCORRECTABLE;
        res.decoded_nibble = (codeword >> 4) & 0x0F; /* Trích xuất thô dù biết bị hỏng */
    }

    return res;
}

size_t hamming84_decode_buffer(const uint8_t *codewords, size_t codewords_len,
                              uint8_t *out_data, size_t max_out_bytes,
                              size_t *out_single_errors, size_t *out_double_errors) {
    if (!codewords || codewords_len < 2 || !out_data) return 0;

    size_t single_err = 0;
    size_t double_err = 0;
    size_t out_idx = 0;

    for (size_t i = 0; i + 1 < codewords_len; i += 2) {
        if (out_idx >= max_out_bytes) break;

        hamming_decode_result_t res_high = hamming84_decode_codeword(codewords[i]);
        hamming_decode_result_t res_low  = hamming84_decode_codeword(codewords[i + 1]);

        if (res_high.status == CHN_STATUS_CORRECTED) single_err++;
        else if (res_high.status == CHN_STATUS_UNCORRECTABLE) double_err++;

        if (res_low.status == CHN_STATUS_CORRECTED) single_err++;
        else if (res_low.status == CHN_STATUS_UNCORRECTABLE) double_err++;

        out_data[out_idx++] = (res_high.decoded_nibble << 4) | (res_low.decoded_nibble & 0x0F);
    }

    if (out_single_errors) *out_single_errors = single_err;
    if (out_double_errors) *out_double_errors = double_err;

    return out_idx;
}
