#ifndef CHN_ENCODER_HAMMING_H
#define CHN_ENCODER_HAMMING_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mã hóa 1 nibble (4 bit) thành 1 từ mã Systematic Hamming (8,4) SEC-DED (8 bit)
 * Định dạng từ mã: [d3 d2 d1 d0 | p1 p2 p3 p4]
 * 4 bit cao (MSB) là 4 bit dữ liệu gốc M.
 * 4 bit thấp (LSB) là 4 bit kiểm tra dư thừa P.
 */
uint8_t hamming84_encode_nibble(uint8_t nibble);

/**
 * @brief Mã hóa một mảng byte dữ liệu thành mảng các từ mã Systematic Hamming (8,4)
 * Mỗi byte dữ liệu đầu vào (8 bit) được tách thành 2 nibbles -> sinh ra 2 byte từ mã (16 bit)
 * @param input Mảng byte dữ liệu đầu vào
 * @param input_len Số byte đầu vào
 * @param out_codewords Mảng nhận các byte từ mã đã được bảo vệ
 * @param max_out_bytes Kích thước mảng đầu ra (tối thiểu 2 * input_len)
 * @return size_t Số byte từ mã được sinh ra
 */
size_t hamming84_encode_buffer(const uint8_t *input, size_t input_len,
                              uint8_t *out_codewords, size_t max_out_bytes);

#ifdef __cplusplus
}
#endif

#endif /* CHN_ENCODER_HAMMING_H */
