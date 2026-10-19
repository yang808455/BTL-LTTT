#include <Arduino.h>
#include "rx_config.h"
#include "frame_parser.h"
#include "chn_decoder_hamming.h"
#include "chn_decoder_cyclic.h"
#include "src_decoder_huffman.h"
#include "src_decoder_arithmetic.h"
#include "src_decoder_lzw.h"
#include "metrics_evaluator.h"

static HardwareSerial SerialFromChannel(0); /* UART0 nhận từ ESP32 Kênh trên GPIO 20 */

static String bytes_to_hex(const uint8_t *data, size_t len) {
    String hex = "";
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) hex += "0";
        hex += String(data[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

static void process_received_frame(const parsed_frame_t *frame) {
    uint8_t comp_buf[FRAME_MAX_PAYLOAD_LEN];
    size_t comp_len = 0;
    size_t single_errors = 0;
    size_t double_errors = 0;

    /* 1. GIẢI MÃ KÊNH HỆ THỐNG & SỬA LỖI BIT */
    switch (frame->chn_alg) {
        case CHN_ALG_HAMMING_84:
            comp_len = hamming84_decode_buffer(frame->payload, frame->payload_len,
                                               comp_buf, sizeof(comp_buf),
                                               &single_errors, &double_errors);
            break;
        case CHN_ALG_CYCLIC_74:
            comp_len = cyclic74_decode_buffer(frame->payload, frame->payload_len,
                                              comp_buf, sizeof(comp_buf),
                                              &single_errors);
            break;
        case CHN_ALG_NONE:
        default:
            comp_len = frame->payload_len;
            memcpy(comp_buf, frame->payload, comp_len);
            break;
    }

    /* 2. GIẢI NÉN NGUỒN VỀ KÝ TỰ GỐC */
    uint8_t recovered_text[512];
    size_t raw_len = 0;

    switch (frame->src_alg) {
        case SRC_ALG_HUFFMAN:
            raw_len = huffman_decode(comp_buf, comp_len, recovered_text, sizeof(recovered_text));
            break;
        case SRC_ALG_ARITHMETIC:
            raw_len = arithmetic_decode(comp_buf, comp_len, recovered_text, sizeof(recovered_text));
            break;
        case SRC_ALG_LZW:
            raw_len = lzw_decode(comp_buf, comp_len, recovered_text, sizeof(recovered_text));
            break;
        case SRC_ALG_NONE:
        default:
            raw_len = (comp_len < sizeof(recovered_text) - 1) ? comp_len : (sizeof(recovered_text) - 1);
            memcpy(recovered_text, comp_buf, raw_len);
            recovered_text[raw_len] = '\0';
            break;
    }

    /* 3. CẬP NHẬT CHỈ SỐ METRICS */
    metrics_update_frame(frame->crc_valid, single_errors, double_errors, raw_len, comp_len);

    global_metrics_t m;
    metrics_get(&m);

    /* 4. GỬI BÁO CÁO JSON LÊN WEB DASHBOARD MÁY THU (PORT 8003) */
    Serial.print("{\"type\":\"RX_EVENT\",");
    Serial.print("\"seq\":"); Serial.print(frame->seq); Serial.print(",");
    Serial.print("\"src_alg\":"); Serial.print((int)frame->src_alg); Serial.print(",");
    Serial.print("\"chn_alg\":"); Serial.print((int)frame->chn_alg); Serial.print(",");
    Serial.print("\"crc_valid\":"); Serial.print(frame->crc_valid ? "true" : "false"); Serial.print(",");
    Serial.print("\"crc_rx\":\"0x"); Serial.print(frame->received_crc, HEX); Serial.print("\",");
    Serial.print("\"crc_calc\":\"0x"); Serial.print(frame->calculated_crc, HEX); Serial.print("\",");
    Serial.print("\"single_err\":"); Serial.print(single_errors); Serial.print(",");
    Serial.print("\"double_err\":"); Serial.print(double_errors); Serial.print(",");
    Serial.print("\"payload_len\":"); Serial.print(frame->payload_len); Serial.print(",");
    Serial.print("\"comp_len\":"); Serial.print(comp_len); Serial.print(",");
    Serial.print("\"raw_len\":"); Serial.print(raw_len); Serial.print(",");
    Serial.print("\"cr\":"); Serial.print(comp_len > 0 ? ((float)raw_len / (float)comp_len) : 0.0f, 2); Serial.print(",");
    Serial.print("\"total_frames\":"); Serial.print(m.total_frames_received); Serial.print(",");
    Serial.print("\"total_valid\":"); Serial.print(m.total_crc_valid_frames); Serial.print(",");
    Serial.print("\"fer\":"); Serial.print(m.fer, 2); Serial.print(",");
    Serial.print("\"correction_rate\":"); Serial.print(m.correction_success_rate, 2); Serial.print(",");
    Serial.print("\"text\":\""); Serial.print((char*)recovered_text); Serial.print("\",");
    Serial.print("\"hex_payload\":\""); Serial.print(bytes_to_hex(frame->payload, frame->payload_len)); Serial.print("\"");
    Serial.println("}");
}

void setup() {
    Serial.begin(115200); /* USB CDC giao tiếp với PC Web Dashboard */

    /* UART0: Nhận dữ liệu từ Node 2 (Kênh truyền) trên chân GPIO 20 */
    SerialFromChannel.begin(RX_BAUDRATE, SERIAL_8N1, PIN_RX_FROM_CHANNEL, -1);

    frame_parser_init();
    metrics_init();

    delay(1000);
    Serial.println("{\"type\":\"NODE_READY\",\"node\":\"RECEIVER\",\"version\":\"1.0\"}");
}

void loop() {
    /* Đọc lệnh từ PC qua USB */
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.indexOf("\"cmd\":\"RESET_METRICS\"") >= 0) {
            metrics_reset();
            Serial.println("{\"type\":\"RESET_METRICS_ACK\"}");
        }
    }

    /* Đọc luồng byte từ Kênh truyền */
    while (SerialFromChannel.available()) {
        uint8_t b = SerialFromChannel.read();
        parsed_frame_t frame;
        if (frame_parser_process_byte(b, &frame)) {
            process_received_frame(&frame);
        }
    }
}
