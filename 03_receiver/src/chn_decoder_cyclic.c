#include "chn_decoder_cyclic.h"

#define CYCLIC_POLY 0x0B /* g(x) = x^3 + x + 1 */

/* Bảng ánh xạ đa thức dư Syndrome S(x) sang vị trí bit cần sửa (0..6) */
static const int8_t CYCLIC_SYNDROME_TO_BIT[8] = {
    -1, /* S = 0: Không có lỗi */
     0, /* S = 1: Lỗi ở bit 0 */
     1, /* S = 2: Lỗi ở bit 1 */
     3, /* S = 3: Lỗi ở bit 3 */
     2, /* S = 4: Lỗi ở bit 2 */
     6, /* S = 5: Lỗi ở bit 6 */
     4, /* S = 6: Lỗi ở bit 4 */
     5  /* S = 7: Lỗi ở bit 5 */
};

cyclic_decode_result_t cyclic74_decode_codeword(uint8_t codeword) {
    cyclic_decode_result_t res;
    res.decoded_nibble = 0;
    res.status = CHN_STATUS_OK;
    res.error_bit_pos = -1;

    uint8_t v = codeword & 0x7F; /* 7 bit từ mã */

    /* Chia c(x) cho g(x) để tính hội chứng lỗi S(x) */
    for (int i = 6; i >= 3; i--) {
        if ((v >> i) & 1) {
            v ^= (CYCLIC_POLY << (i - 3));
        }
    }
    uint8_t syndrome = v & 0x07;
    res.syndrome = syndrome;

    if (syndrome == 0) {
        /* Không có lỗi */
        res.status = CHN_STATUS_OK;
        res.decoded_nibble = (codeword >> 3) & 0x0F;
    } else {
        /* Phát hiện lỗi bit -> Sửa lỗi */
        int8_t err_pos = CYCLIC_SYNDROME_TO_BIT[syndrome];
        res.error_bit_pos = err_pos;
        res.status = CHN_STATUS_CORRECTED;

        uint8_t corrected_codeword = codeword ^ (1 << err_pos);
        res.decoded_nibble = (corrected_codeword >> 3) & 0x0F;
    }

    return res;
}

size_t cyclic74_decode_buffer(const uint8_t *codewords, size_t codewords_len,
                             uint8_t *out_data, size_t max_out_bytes,
                             size_t *out_corrected_errors) {
    if (!codewords || codewords_len < 2 || !out_data) return 0;

    size_t corrected = 0;
    size_t out_idx = 0;

    for (size_t i = 0; i + 1 < codewords_len; i += 2) {
        if (out_idx >= max_out_bytes) break;

        cyclic_decode_result_t res_high = cyclic74_decode_codeword(codewords[i]);
        cyclic_decode_result_t res_low  = cyclic74_decode_codeword(codewords[i + 1]);

        if (res_high.status == CHN_STATUS_CORRECTED) corrected++;
        if (res_low.status == CHN_STATUS_CORRECTED) corrected++;

        out_data[out_idx++] = (res_high.decoded_nibble << 4) | (res_low.decoded_nibble & 0x0F);
    }

    if (out_corrected_errors) *out_corrected_errors = corrected;
    return out_idx;
}
