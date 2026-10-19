#ifndef SRC_ENCODER_ARITHMETIC_H
#define SRC_ENCODER_ARITHMETIC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mã hóa nguồn bằng thuật toán Mã số học (Arithmetic Coding)
 * @param input Chuỗi dữ liệu đầu vào
 * @param input_len Chiều dài chuỗi
 * @param out_bytes Vùng đệm chứa dòng byte sau nén số học
 * @param max_out_bytes Kích thước vùng đệm đầu ra
 * @param out_total_bits Con trỏ nhận tổng số bit đã tạo
 * @return size_t Số byte thực tế sau khi đóng gói
 */
size_t arithmetic_encode(const uint8_t *input, size_t input_len,
                         uint8_t *out_bytes, size_t max_out_bytes,
                         size_t *out_total_bits);

#ifdef __cplusplus
}
#endif

#endif /* SRC_ENCODER_ARITHMETIC_H */
