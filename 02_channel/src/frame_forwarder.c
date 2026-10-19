#include "frame_forwarder.h"
#include "noise_generator.h"
#include <string.h>

typedef enum {
    FSM_WAIT_SOF,
    FSM_READ_HEADER,
    FSM_READ_PAYLOAD,
    FSM_READ_CRC_MSB,
    FSM_READ_CRC_LSB,
    FSM_READ_EOF
} fsm_state_t;

static fsm_state_t s_state = FSM_WAIT_SOF;
static uint8_t s_frame_buf[FRAME_MAX_TOTAL_LEN];
static size_t s_buf_idx = 0;
static uint8_t s_payload_len = 0;
static size_t s_payload_read = 0;

void forwarder_init(void) {
    s_state = FSM_WAIT_SOF;
    s_buf_idx = 0;
    s_payload_len = 0;
    s_payload_read = 0;
}

bool forwarder_process_byte(uint8_t byte_in, const channel_settings_t *settings, channel_event_t *out_event) {
    switch (s_state) {
        case FSM_WAIT_SOF:
            if (byte_in == FRAME_SOF) {
                s_buf_idx = 0;
                s_frame_buf[s_buf_idx++] = byte_in;
                s_state = FSM_READ_HEADER;
            }
            break;

        case FSM_READ_HEADER:
            s_frame_buf[s_buf_idx++] = byte_in;
            if (s_buf_idx == FRAME_HEADER_LEN) {
                s_payload_len = s_frame_buf[4];
                s_payload_read = 0;
                if (s_payload_len > FRAME_MAX_PAYLOAD_LEN) {
                    s_state = FSM_WAIT_SOF; /* Payload quá lớn -> Hủy khung */
                } else if (s_payload_len == 0) {
                    s_state = FSM_READ_CRC_MSB;
                } else {
                    s_state = FSM_READ_PAYLOAD;
                }
            }
            break;

        case FSM_READ_PAYLOAD:
            s_frame_buf[s_buf_idx++] = byte_in;
            s_payload_read++;
            if (s_payload_read >= s_payload_len) {
                s_state = FSM_READ_CRC_MSB;
            }
            break;

        case FSM_READ_CRC_MSB:
            s_frame_buf[s_buf_idx++] = byte_in;
            s_state = FSM_READ_CRC_LSB;
            break;

        case FSM_READ_CRC_LSB:
            s_frame_buf[s_buf_idx++] = byte_in;
            s_state = FSM_READ_EOF;
            break;

        case FSM_READ_EOF:
            s_frame_buf[s_buf_idx++] = byte_in;
            s_state = FSM_WAIT_SOF;

            if (byte_in == FRAME_EOF && out_event) {
                /* Đã bắt trọn vẹn 1 khung */
                out_event->frame_len = s_buf_idx;
                out_event->seq = s_frame_buf[1];
                out_event->bits_in_payload = s_payload_len * 8;
                memcpy(out_event->raw_frame, s_frame_buf, s_buf_idx);
                memcpy(out_event->corrupted_frame, s_frame_buf, s_buf_idx);

                size_t flipped = 0;
                if (settings && settings->enabled && settings->ber > 0.0f) {
                    if (settings->scope == NOISE_SCOPE_PAYLOAD_ONLY && s_payload_len > 0) {
                        /* Chỉ bơm nhiễu vào trường payload (từ offset 5) */
                        noise_gen_apply_bsc(out_event->corrupted_frame, FRAME_HEADER_LEN, s_payload_len,
                                            settings->ber, &flipped);
                    } else if (settings->scope == NOISE_SCOPE_FULL_FRAME) {
                        /* Bơm nhiễu vào toàn bộ khung */
                        noise_gen_apply_bsc(out_event->corrupted_frame, 0, s_buf_idx,
                                            settings->ber, &flipped);
                    }
                }
                out_event->bits_flipped = flipped;
                out_event->current_ber = (out_event->bits_in_payload > 0) ?
                    ((float)flipped / (float)out_event->bits_in_payload) : 0.0f;

                return true;
            }
            break;
    }
    return false;
}
