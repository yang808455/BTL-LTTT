#include "frame_parser.h"
#include "../../shared/crc16.h"
#include <string.h>

typedef enum {
    PARSE_WAIT_SOF,
    PARSE_HEADER,
    PARSE_PAYLOAD,
    PARSE_CRC_MSB,
    PARSE_CRC_LSB,
    PARSE_EOF
} parse_state_t;

static parse_state_t s_state = PARSE_WAIT_SOF;
static uint8_t s_buf[FRAME_MAX_TOTAL_LEN];
static size_t s_buf_idx = 0;
static uint8_t s_payload_len = 0;
static size_t s_payload_read = 0;

void frame_parser_init(void) {
    s_state = PARSE_WAIT_SOF;
    s_buf_idx = 0;
    s_payload_len = 0;
    s_payload_read = 0;
}

bool frame_parser_process_byte(uint8_t byte_in, parsed_frame_t *out_frame) {
    switch (s_state) {
        case PARSE_WAIT_SOF:
            if (byte_in == FRAME_SOF) {
                s_buf_idx = 0;
                s_buf[s_buf_idx++] = byte_in;
                s_state = PARSE_HEADER;
            }
            break;

        case PARSE_HEADER:
            s_buf[s_buf_idx++] = byte_in;
            if (s_buf_idx == FRAME_HEADER_LEN) {
                s_payload_len = s_buf[4];
                s_payload_read = 0;
                if (s_payload_len > FRAME_MAX_PAYLOAD_LEN) {
                    s_state = PARSE_WAIT_SOF;
                } else if (s_payload_len == 0) {
                    s_state = PARSE_CRC_MSB;
                } else {
                    s_state = PARSE_PAYLOAD;
                }
            }
            break;

        case PARSE_PAYLOAD:
            s_buf[s_buf_idx++] = byte_in;
            s_payload_read++;
            if (s_payload_read >= s_payload_len) {
                s_state = PARSE_CRC_MSB;
            }
            break;

        case PARSE_CRC_MSB:
            s_buf[s_buf_idx++] = byte_in;
            s_state = PARSE_CRC_LSB;
            break;

        case PARSE_CRC_LSB:
            s_buf[s_buf_idx++] = byte_in;
            s_state = PARSE_EOF;
            break;

        case PARSE_EOF:
            s_buf[s_buf_idx++] = byte_in;
            s_state = PARSE_WAIT_SOF;

            if (byte_in == FRAME_EOF && out_frame) {
                out_frame->total_frame_len = s_buf_idx;
                out_frame->seq = s_buf[1];
                out_frame->src_alg = (source_alg_t)s_buf[2];
                out_frame->chn_alg = (channel_alg_t)s_buf[3];
                out_frame->payload_len = s_payload_len;

                memcpy(out_frame->payload, &s_buf[FRAME_HEADER_LEN], s_payload_len);

                size_t crc_offset = FRAME_HEADER_LEN + s_payload_len;
                out_frame->received_crc = ((uint16_t)s_buf[crc_offset] << 8) | s_buf[crc_offset + 1];
                out_frame->calculated_crc = crc16_ccitt(s_buf, crc_offset);
                out_frame->crc_valid = (out_frame->received_crc == out_frame->calculated_crc);

                return true;
            }
            break;
    }
    return false;
}
