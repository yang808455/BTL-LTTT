#include <Arduino.h>
#include "include/tx_config.h"
#include "include/frame_builder.h"

static HardwareSerial SerialChannel(1); /* UART1 kết nối sang Bo mạch Kênh */

static tx_settings_t g_tx_settings = {
    .src_alg = SRC_ALG_HUFFMAN,
    .chn_alg = CHN_ALG_HAMMING_84,
    .seq = 0
};

static String g_current_payload = DEFAULT_PAYLOAD_TEXT;
static bool g_auto_send = true;
static unsigned long g_send_interval_ms = DEFAULT_TX_INTERVAL_MS;
static unsigned long g_last_send_time = 0;

/* Hàm chuyển mảng byte sang chuỗi Hex để gửi lên Web */
static String bytes_to_hex(const uint8_t *data, size_t len) {
    String hex = "";
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) hex += "0";
        hex += String(data[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

/* Thực hiện đóng gói và phát 1 gói tin */
static void transmit_packet(const String &text_data) {
    uint8_t frame_buf[FRAME_MAX_TOTAL_LEN];
    tx_metrics_t metrics;

    size_t frame_len = build_tx_frame(
        (const uint8_t*)text_data.c_str(),
        text_data.length(),
        &g_tx_settings,
        frame_buf,
        sizeof(frame_buf),
        &metrics
    );

    if (frame_len > 0) {
        /* Bắn khung qua UART1 sang bo Kênh */
        SerialChannel.write(frame_buf, frame_len);
        SerialChannel.flush();

        /* Gửi thông điệp JSON lên Web Dashboard qua cổng USB Serial */
        Serial.print("{\"type\":\"TX_EVENT\",");
        Serial.print("\"seq\":"); Serial.print(g_tx_settings.seq); Serial.print(",");
        Serial.print("\"src_alg\":"); Serial.print((int)g_tx_settings.src_alg); Serial.print(",");
        Serial.print("\"chn_alg\":"); Serial.print((int)g_tx_settings.chn_alg); Serial.print(",");
        Serial.print("\"raw_len\":"); Serial.print(metrics.raw_len); Serial.print(",");
        Serial.print("\"comp_len\":"); Serial.print(metrics.compressed_len); Serial.print(",");
        Serial.print("\"comp_bits\":"); Serial.print(metrics.compressed_bits); Serial.print(",");
        Serial.print("\"encoded_len\":"); Serial.print(metrics.encoded_len); Serial.print(",");
        Serial.print("\"total_frame_len\":"); Serial.print(frame_len); Serial.print(",");
        Serial.print("\"cr\":"); Serial.print(metrics.compression_ratio, 2); Serial.print(",");
        Serial.print("\"crc\":\"0x"); Serial.print(metrics.crc16, HEX); Serial.print("\",");
        Serial.print("\"raw_text\":\""); Serial.print(text_data); Serial.print("\",");
        Serial.print("\"hex_frame\":\""); Serial.print(bytes_to_hex(frame_buf, frame_len)); Serial.print("\"");
        Serial.println("}");

        g_tx_settings.seq++;
    }
}

/* Xử lý lệnh JSON gửi từ Web Dashboard xuống bo mạch qua USB CDC */
static void process_pc_command(const String &cmd_str) {
    if (cmd_str.indexOf("\"cmd\":\"SEND\"") >= 0) {
        int data_start = cmd_str.indexOf("\"data\":\"");
        if (data_start >= 0) {
            data_start += 8;
            int data_end = cmd_str.indexOf("\"", data_start);
            if (data_end > data_start) {
                String custom_text = cmd_str.substring(data_start, data_end);
                transmit_packet(custom_text);
            }
        } else {
            transmit_packet(g_current_payload);
        }
    } else if (cmd_str.indexOf("\"cmd\":\"SET_CONFIG\"") >= 0) {
        int src_idx = cmd_str.indexOf("\"src_alg\":");
        if (src_idx >= 0) {
            g_tx_settings.src_alg = (source_alg_t)cmd_str.substring(src_idx + 10).toInt();
        }
        int chn_idx = cmd_str.indexOf("\"chn_alg\":");
        if (chn_idx >= 0) {
            g_tx_settings.chn_alg = (channel_alg_t)cmd_str.substring(chn_idx + 10).toInt();
        }
        int auto_idx = cmd_str.indexOf("\"auto_send\":");
        if (auto_idx >= 0) {
            g_auto_send = (cmd_str.indexOf("\"auto_send\":true") >= 0);
        }
        int interval_idx = cmd_str.indexOf("\"interval_ms\":");
        if (interval_idx >= 0) {
            g_send_interval_ms = cmd_str.substring(interval_idx + 14).toInt();
            if (g_send_interval_ms < 100) g_send_interval_ms = 100;
        }
        int payload_idx = cmd_str.indexOf("\"payload\":\"");
        if (payload_idx >= 0) {
            payload_idx += 11;
            int p_end = cmd_str.indexOf("\"", payload_idx);
            if (p_end > payload_idx) {
                g_current_payload = cmd_str.substring(payload_idx, p_end);
            }
        }
        /* Phản hồi trạng thái */
        Serial.print("{\"type\":\"CONFIG_ACK\",\"src_alg\":");
        Serial.print((int)g_tx_settings.src_alg);
        Serial.print(",\"chn_alg\":");
        Serial.print((int)g_tx_settings.chn_alg);
        Serial.print(",\"auto_send\":");
        Serial.print(g_auto_send ? "true" : "false");
        Serial.print(",\"interval_ms\":");
        Serial.print(g_send_interval_ms);
        Serial.println("}");
    }
}

void setup() {
    /* Khởi tạo USB CDC debug & giao tiếp với Web Dashboard */
    Serial.begin(115200);

    /* Khởi tạo Hardware UART1 phát dữ liệu sang ESP32 Kênh */
    SerialChannel.begin(UART_BAUDRATE, SERIAL_8N1, PIN_UART1_RX, PIN_UART1_TX);

    delay(1000);
    Serial.println("{\"type\":\"NODE_READY\",\"node\":\"TRANSMITTER\",\"version\":\"1.0\"}");
}

void loop() {
    /* Đọc lệnh điều khiển từ Web Dashboard qua USB */
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            process_pc_command(line);
        }
    }

    /* Phát tự động theo chu kỳ nếu bật auto_send */
    if (g_auto_send && (millis() - g_last_send_time >= g_send_interval_ms)) {
        g_last_send_time = millis();
        transmit_packet(g_current_payload);
    }
}
