#ifndef SRC_ENCODER_HUFFMAN_H
#define SRC_ENCODER_HUFFMAN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUFFMAN_EOF_SYMBOL 128 /* Ký hiệu kết thúc luồng Huffman */

typedef struct {
    uint16_t code;   /* Từ mã nhị phân */
    uint8_t bit_len; /* Độ dài bit */
} huffman_code_t;

/**
 * @brief Mã hóa mảng byte đầu vào bằng giải thuật Huffman tĩnh chuẩn
 * @param input Chuỗi byte đầu vào (ASCII)
 * @param input_len Độ dài chuỗi
 * @param out_bits Vùng đệm chứa luồng bit nén
 * @param max_out_bytes Dung lượng vùng đệm đầu ra
 * @param out_total_bits Con trỏ nhận tổng số bit sau nén
 * @return size_t Số byte thực tế được sử dụng trong out_bits
 */
size_t huffman_encode(const uint8_t *input, size_t input_len,
                      uint8_t *out_bits, size_t max_out_bytes,
                      size_t *out_total_bits);

/**
 * @brief Tra cứu từ mã Huffman của một ký tự
 */
bool huffman_get_code(uint16_t symbol, huffman_code_t *entry);

#ifdef __cplusplus
}
#endif

#endif /* SRC_ENCODER_HUFFMAN_H */
