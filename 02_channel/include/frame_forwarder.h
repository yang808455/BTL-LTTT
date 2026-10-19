#ifndef FRAME_FORWARDER_H
#define FRAME_FORWARDER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "channel_config.h"
#include "../../shared/protocol_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t raw_frame[FRAME_MAX_TOTAL_LEN];
    uint8_t corrupted_frame[FRAME_MAX_TOTAL_LEN];
    size_t frame_len;
    uint8_t seq;
    size_t bits_in_payload;
    size_t bits_flipped;
    float current_ber;
} channel_event_t;

/**
 * @brief Khởi tạo bộ gom khung và chuyển tiếp
 */
void forwarder_init(void);

/**
 * @brief Nạp từng byte nhận được từ UART của Máy phát vào FSM gom khung
 * @param byte_in Byte nhận được
 * @param settings Cấu hình nhiễu
 * @param out_event Con trỏ nhận dữ liệu sự kiện khi gom trọn vẹn 1 khung
 * @return true nếu đã bắt trọn vẹn và xử lý xong 1 khung
 */
bool forwarder_process_byte(uint8_t byte_in, const channel_settings_t *settings, channel_event_t *out_event);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_FORWARDER_H */
