#ifndef SRC_ENCODER_LZW_H
#define SRC_ENCODER_LZW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LZW_DICT_MAX_CODES 512 /* 9-bit LZW dictionary (0..511) */
#define LZW_EOF_CODE       128 /* Mã kết thúc luồng LZW */

/**
 * @brief Mã hóa dữ liệu bằng thuật toán từ điển LZW (Lempel-Ziv-Welch 9-bit)
 * @param input Dữ liệu đầu vào
 * @param input_len Độ dài dữ liệu
 * @param out_bytes Vùng đệm chứa luồng bit nén LZW
 * @param max_out_bytes Kích thước vùng đệm đầu ra
 * @param out_total_bits Con trỏ nhận tổng số bit sau nén
 * @return size_t Số byte thực tế sau khi nén
 */
size_t lzw_encode(const uint8_t *input, size_t input_len,
                  uint8_t *out_bytes, size_t max_out_bytes,
                  size_t *out_total_bits);

#ifdef __cplusplus
}
#endif

#endif /* SRC_ENCODER_LZW_H */
