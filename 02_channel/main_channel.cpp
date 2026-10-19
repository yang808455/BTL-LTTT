#include <Arduino.h>
#include "include/channel_config.h"
#include "include/noise_generator.h"
#include "include/frame_forwarder.h"

static HardwareSerial SerialFromTx(0); /* UART0 nhận từ Tx trên GPIO 20 */
static HardwareSerial SerialToRx(1);   /* UART1 phát sang Rx trên GPIO 21 */

static channel_settings_t g_settings = {
    .ber = 0.01f, /* 1.0% bit errors */
    .scope = NOISE_SCOPE_PAYLOAD_ONLY,
    .enabled = true
};

static String bytes_to_hex(const uint8_t *data, size_t len) {
    String hex = "";
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) hex += "0";
        hex += String(data[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

/* Xử lý lệnh cấu hình từ Web Dashboard qua USB CDC */
static void process_pc_command(const String &cmd_str) {
    if (cmd_str.indexOf("\"cmd\":\"SET_CHANNEL\"") >= 0) {
        int ber_idx = cmd_str.indexOf("\"ber\":");
        if (ber_idx >= 0) {
            g_settings.ber = cmd_str.substring(ber_idx + 6).toFloat();
            if (g_settings.ber < 0.0f) g_settings.ber = 0.0f;
            if (g_settings.ber > 0.5f) g_settings.ber = 0.5f;
        }
        int scope_idx = cmd_str.indexOf("\"scope\":");
        if (scope_idx >= 0) {
            g_settings.scope = (noise_scope_t)cmd_str.substring(scope_idx + 8).toInt();
        }
        int en_idx = cmd_str.indexOf("\"enabled\":");
        if (en_idx >= 0) {
            g_settings.enabled = (cmd_str.indexOf("\"enabled\":true") >= 0);
        }

        Serial.print("{\"type\":\"CHANNEL_CONFIG_ACK\",");
        Serial.print("\"ber\":"); Serial.print(g_settings.ber, 4); Serial.print(",");
        Serial.print("\"scope\":"); Serial.print((int)g_settings.scope); Serial.print(",");
        Serial.print("\"enabled\":"); Serial.print(g_settings.enabled ? "true" : "false");
        Serial.println("}");
    } else if (cmd_str.indexOf("\"cmd\":\"RESET_STATS\"") >= 0) {
        noise_gen_reset_stats();
        Serial.println("{\"type\":\"RESET_STATS_ACK\"}");
    }
}

void setup() {
    Serial.begin(115200); /* USB CDC giao tiếp với PC Web Dashboard */

    /* UART0: Nhận từ Node 1 (Tx) trên GPIO 20 */
    SerialFromTx.begin(CHANNEL_BAUDRATE, SERIAL_8N1, PIN_RX_FROM_TX, -1);

    /* UART1: Phát sang Node 3 (Rx) trên GPIO 21 */
    SerialToRx.begin(CHANNEL_BAUDRATE, SERIAL_8N1, -1, PIN_TX_TO_RX);

    noise_gen_init();
    forwarder_init();

    delay(1000);
    Serial.println("{\"type\":\"NODE_READY\",\"node\":\"CHANNEL_EMULATOR\",\"version\":\"1.0\"}");
}

void loop() {
    /* Đọc lệnh điều khiển từ PC qua USB */
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            process_pc_command(line);
        }
    }

    /* Đọc byte từ Node 1 (Tx) */
    while (SerialFromTx.available()) {
        uint8_t b = SerialFromTx.read();
        channel_event_t evt;
        if (forwarder_process_byte(b, &g_settings, &evt)) {
            /* 1. Chuyển tiếp khung đã gây nhiễu sang Node 3 (Rx) */
            SerialToRx.write(evt.corrupted_frame, evt.frame_len);
            SerialToRx.flush();

            /* 2. Thống kê lũy kế */
            noise_stats_t stats;
            noise_gen_get_stats(&stats);

            /* 3. Báo cáo JSON lên Web Dashboard Kênh truyền */
            Serial.print("{\"type\":\"CHANNEL_EVENT\",");
            Serial.print("\"seq\":"); Serial.print(evt.seq); Serial.print(",");
            Serial.print("\"frame_len\":"); Serial.print(evt.frame_len); Serial.print(",");
            Serial.print("\"bits_payload\":"); Serial.print(evt.bits_in_payload); Serial.print(",");
            Serial.print("\"bits_flipped\":"); Serial.print(evt.bits_flipped); Serial.print(",");
            Serial.print("\"target_ber\":"); Serial.print(g_settings.ber, 4); Serial.print(",");
            Serial.print("\"frame_ber\":"); Serial.print(evt.current_ber, 4); Serial.print(",");
            Serial.print("\"total_bits\":"); Serial.print(stats.total_bits_processed); Serial.print(",");
            Serial.print("\"total_flipped\":"); Serial.print(stats.total_bits_flipped); Serial.print(",");
            Serial.print("\"cumulative_ber\":"); Serial.print(stats.cumulative_ber, 4); Serial.print(",");
            Serial.print("\"clean_hex\":\""); Serial.print(bytes_to_hex(evt.raw_frame, evt.frame_len)); Serial.print("\",");
            Serial.print("\"corrupted_hex\":\""); Serial.print(bytes_to_hex(evt.corrupted_frame, evt.frame_len)); Serial.print("\"");
            Serial.println("}");
        }
    }
}
