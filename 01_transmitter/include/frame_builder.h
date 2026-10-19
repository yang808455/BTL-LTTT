#ifndef FRAME_BUILDER_H
#define FRAME_BUILDER_H

#include <stdint.h>
#include <stddef.h>
#include "../../shared/protocol_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    source_alg_t src_alg;
    channel_alg_t chn_alg;
    uint8_t seq;
} tx_settings_t;

typedef struct {
    size_t raw_len;
    size_t compressed_len;
    size_t compressed_bits;
    size_t encoded_len;
    float compression_ratio;
    uint16_t crc16;
} tx_metrics_t;

/**
 * @brief Quy trình xử lý toàn diện phía Máy phát:
 * 1. Nén nguồn (Huffman / Arithmetic / LZW)
 * 2. Mã hóa kênh (Systematic Hamming / Systematic Cyclic)
 * 3. Đóng gói khung UART và tính CRC16
 * 
 * @param input Chuỗi dữ liệu thô (ASCII / Sensor)
 * @param input_len Độ dài dữ liệu thô
 * @param settings Cấu hình thuật toán & số thứ tự gói tin
 * @param out_frame Vùng đệm nhận khung UART hoàn chỉnh để phát ra phần cứng
 * @param max_frame_len Kích thước tối đa của vùng đệm out_frame
 * @param metrics Con trỏ nhận các thông số thống kê quá trình mã hóa
 * @return size_t Tổng số byte của khung UART (0 nếu có lỗi)
 */
size_t build_tx_frame(const uint8_t *input, size_t input_len,
                      const tx_settings_t *settings,
                      uint8_t *out_frame, size_t max_frame_len,
                      tx_metrics_t *metrics);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_BUILDER_H */
