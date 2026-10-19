#include "frame_builder.h"
#include "src_encoder_huffman.h"
#include "src_encoder_arithmetic.h"
#include "src_encoder_lzw.h"
#include "chn_encoder_hamming.h"
#include "chn_encoder_cyclic.h"
#include "../../shared/crc16.h"
#include <string.h>

size_t build_tx_frame(const uint8_t *input, size_t input_len,
                      const tx_settings_t *settings,
                      uint8_t *out_frame, size_t max_frame_len,
                      tx_metrics_t *metrics) {
    if (!input || input_len == 0 || !settings || !out_frame || max_frame_len < FRAME_OVERHEAD_LEN) {
        return 0;
    }

    /* Vùng đệm trung gian */
    uint8_t comp_buf[FRAME_MAX_PAYLOAD_LEN];
    size_t comp_len = 0;
    size_t comp_bits = 0;

    /* 1. NÉN NGUỒN (SOURCE ENCODING) */
    switch (settings->src_alg) {
        case SRC_ALG_HUFFMAN:
            comp_len = huffman_encode(input, input_len, comp_buf, sizeof(comp_buf), &comp_bits);
            break;
        case SRC_ALG_ARITHMETIC:
            comp_len = arithmetic_encode(input, input_len, comp_buf, sizeof(comp_buf), &comp_bits);
            break;
        case SRC_ALG_LZW:
            comp_len = lzw_encode(input, input_len, comp_buf, sizeof(comp_buf), &comp_bits);
            break;
        case SRC_ALG_NONE:
        default:
            comp_len = (input_len < sizeof(comp_buf)) ? input_len : sizeof(comp_buf);
            memcpy(comp_buf, input, comp_len);
            comp_bits = comp_len * 8;
            break;
    }

    if (comp_len == 0) return 0;

    /* 2. MÃ HÓA KÊNH HỆ THỐNG (SYSTEMATIC CHANNEL CODING) */
    uint8_t payload_buf[FRAME_MAX_PAYLOAD_LEN];
    size_t payload_len = 0;

    switch (settings->chn_alg) {
        case CHN_ALG_HAMMING_84:
            payload_len = hamming84_encode_buffer(comp_buf, comp_len, payload_buf, sizeof(payload_buf));
            break;
        case CHN_ALG_CYCLIC_74:
            payload_len = cyclic74_encode_buffer(comp_buf, comp_len, payload_buf, sizeof(payload_buf));
            break;
        case CHN_ALG_NONE:
        default:
            payload_len = comp_len;
            memcpy(payload_buf, comp_buf, payload_len);
            break;
    }

    if (payload_len == 0 || payload_len > 255) return 0;

    /* Kiểm tra tổng chiều dài khung */
    size_t total_frame_len = FRAME_HEADER_LEN + payload_len + 3; /* + CRC16(2) + EOF(1) */
    if (total_frame_len > max_frame_len) return 0;

    /* 3. ĐÓNG GÓI KHUNG (FRAMING) */
    out_frame[0] = FRAME_SOF;
    out_frame[1] = settings->seq;
    out_frame[2] = (uint8_t)settings->src_alg;
    out_frame[3] = (uint8_t)settings->chn_alg;
    out_frame[4] = (uint8_t)payload_len;

    memcpy(&out_frame[FRAME_HEADER_LEN], payload_buf, payload_len);

    /* 4. TÍNH CRC16 TRÊN HEADER + PAYLOAD */
    uint16_t crc = crc16_ccitt(out_frame, FRAME_HEADER_LEN + payload_len);
    out_frame[FRAME_HEADER_LEN + payload_len]     = (uint8_t)(crc >> 8);   /* CRC MSB */
    out_frame[FRAME_HEADER_LEN + payload_len + 1] = (uint8_t)(crc & 0xFF); /* CRC LSB */
    out_frame[FRAME_HEADER_LEN + payload_len + 2] = FRAME_EOF;

    /* 5. GHI NHẬN METRICS */
    if (metrics) {
        metrics->raw_len = input_len;
        metrics->compressed_len = comp_len;
        metrics->compressed_bits = comp_bits;
        metrics->encoded_len = payload_len;
        metrics->compression_ratio = (float)input_len / (float)comp_len;
        metrics->crc16 = crc;
    }

    return total_frame_len;
}
