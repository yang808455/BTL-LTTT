#ifndef CHN_ENCODER_CYCLIC_H
#define CHN_ENCODER_CYCLIC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CYCLIC_POLY 0x0B /* g(x) = x^3 + x + 1 -> 1011 nhị phân */

/**
 * @brief Mã hóa 1 nibble (4 bit) thành 1 từ mã Systematic Cyclic (7,4) (7 bit trong 1 byte)
 * Dạng từ mã nhị phân: [0 m3 m2 m1 m0 r2 r1 r0]
 * Bits 6..3: 4 bit thông tin gốc m(x)
 * Bits 2..0: 3 bit dư thừa r(x) thu được từ phép chia x^3*m(x) cho g(x)
 */
uint8_t cyclic74_encode_nibble(uint8_t nibble);

/**
 * @brief Mã hóa mảng byte dữ liệu thành mảng các từ mã Systematic Cyclic (7,4)
 * Mỗi byte dữ liệu đầu vào sinh ra 2 byte từ mã (mỗi byte chứa 1 từ mã 7-bit)
 */
size_t cyclic74_encode_buffer(const uint8_t *input, size_t input_len,
                             uint8_t *out_codewords, size_t max_out_bytes);

#ifdef __cplusplus
}
#endif

#endif /* CHN_ENCODER_CYCLIC_H */
