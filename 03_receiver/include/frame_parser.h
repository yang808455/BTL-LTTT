#ifndef FRAME_PARSER_H
#define FRAME_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../shared/protocol_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t seq;
    source_alg_t src_alg;
    channel_alg_t chn_alg;
    uint8_t payload[FRAME_MAX_PAYLOAD_LEN];
    uint8_t payload_len;
    uint16_t received_crc;
    uint16_t calculated_crc;
    bool crc_valid;
    size_t total_frame_len;
} parsed_frame_t;

void frame_parser_init(void);

/**
 * @brief Đưa từng byte từ UART vào máy trạng thái bóc tách khung
 * @param byte_in Byte nhận được
 * @param out_frame Con trỏ nhận khung hoàn chỉnh khi bắt trúng EOF
 * @return true nếu bắt trọn vẹn 1 khung hợp lệ
 */
bool frame_parser_process_byte(uint8_t byte_in, parsed_frame_t *out_frame);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_PARSER_H */
