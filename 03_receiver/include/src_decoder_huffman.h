#ifndef SRC_DECODER_HUFFMAN_H
#define SRC_DECODER_HUFFMAN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Giải nén luồng bit Huffman thành chuỗi ký tự gốc
 * @param comp_data Luồng byte nén đã được giải mã kênh sửa lỗi
 * @param comp_len Độ dài luồng nén
 * @param out_text Vùng đệm nhận chuỗi văn bản đã giải nén
 * @param max_text_len Kích thước vùng đệm đầu ra
 * @return size_t Số ký tự đã khôi phục (không tính null terminator)
 */
size_t huffman_decode(const uint8_t *comp_data, size_t comp_len,
                      uint8_t *out_text, size_t max_text_len);

#ifdef __cplusplus
}
#endif

#endif /* SRC_DECODER_HUFFMAN_H */
