#ifndef CHN_DECODER_HAMMING_H
#define CHN_DECODER_HAMMING_H

#include <stdint.h>
#include <stddef.h>
#include "../../shared/protocol_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t decoded_nibble;         /* 4 bit dữ liệu gốc đã khôi phục */
    chn_decode_status_t status;     /* OK, CORRECTED, UNCORRECTABLE */
    uint8_t syndrome;               /* Giá trị syndrome S (0-7) */
    int8_t error_bit_pos;           /* Vị trí bit sai (0-7), -1 nếu không có lỗi */
} hamming_decode_result_t;

/**
 * @brief Giải mã 1 từ mã Systematic Hamming (8,4) SEC-DED
 * Kiểm tra Syndrome, sửa lỗi 1 bit và phát hiện lỗi 2 bit
 */
hamming_decode_result_t hamming84_decode_codeword(uint8_t codeword);

/**
 * @brief Giải mã toàn bộ vùng đệm từ mã Hamming (8,4) về dữ liệu nén
 * Cứ 2 byte từ mã (16 bit) gộp lại thành 1 byte dữ liệu (8 bit)
 * @param codewords Mảng các từ mã nhận được từ kênh
 * @param codewords_len Số byte từ mã
 * @param out_data Mảng nhận dữ liệu đã giải mã & sửa lỗi
 * @param max_out_bytes Kích thước vùng đệm đầu ra
 * @param out_single_errors Con trỏ nhận tổng số lỗi 1 bit đã sửa thành công
 * @param out_double_errors Con trỏ nhận tổng số lỗi kép không thể sửa
 * @return size_t Số byte dữ liệu khôi phục được
 */
size_t hamming84_decode_buffer(const uint8_t *codewords, size_t codewords_len,
                              uint8_t *out_data, size_t max_out_bytes,
                              size_t *out_single_errors, size_t *out_double_errors);

#ifdef __cplusplus
}
#endif

#endif /* CHN_DECODER_HAMMING_H */
