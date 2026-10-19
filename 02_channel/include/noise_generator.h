#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_bits_processed;
    uint32_t total_bits_flipped;
    float cumulative_ber;
} noise_stats_t;

/**
 * @brief Khởi tạo bộ sinh nhiễu BSC (Binary Symmetric Channel)
 */
void noise_gen_init(void);

/**
 * @brief Gây nhiễu Bernoulli đảo bit trên một vùng đệm dữ liệu
 * @param buffer Vùng đệm dữ liệu
 * @param start_idx Vị trí byte bắt đầu gây lỗi
 * @param count Số byte cần quét gây lỗi
 * @param ber Xác suất lỗi bit Pe (0.0 đến 1.0)
 * @param out_flipped_count Con trỏ nhận số bit thực tế đã bị đảo trong lần gọi này
 */
void noise_gen_apply_bsc(uint8_t *buffer, size_t start_idx, size_t count,
                         float ber, size_t *out_flipped_count);

/**
 * @brief Đảo thủ công 1 bit xác định (phục vụ người dùng click can thiệp từ Web Dashboard)
 */
bool noise_gen_manual_flip(uint8_t *buffer, size_t buffer_len, size_t byte_idx, uint8_t bit_idx);

/**
 * @brief Lấy thống kê lũy kế
 */
void noise_gen_get_stats(noise_stats_t *stats);
void noise_gen_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* NOISE_GENERATOR_H */
