#ifndef CHN_DECODER_CYCLIC_H
#define CHN_DECODER_CYCLIC_H

#include <stdint.h>
#include <stddef.h>
#include "../../shared/protocol_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t decoded_nibble;         /* 4 bit thông tin m(x) */
    chn_decode_status_t status;     /* OK hoặc CORRECTED */
    uint8_t syndrome;               /* Đa thức phần dư S(x) (0-7) */
    int8_t error_bit_pos;           /* Vị trí bit sai (0-6), -1 nếu không có lỗi */
} cyclic_decode_result_t;

/**
 * @brief Giải mã 1 từ mã Systematic Cyclic (7,4)
 */
cyclic_decode_result_t cyclic74_decode_codeword(uint8_t codeword);

/**
 * @brief Giải mã toàn bộ vùng đệm từ mã Systematic Cyclic (7,4)
 */
size_t cyclic74_decode_buffer(const uint8_t *codewords, size_t codewords_len,
                             uint8_t *out_data, size_t max_out_bytes,
                             size_t *out_corrected_errors);

#ifdef __cplusplus
}
#endif

#endif /* CHN_DECODER_CYCLIC_H */
