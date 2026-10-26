#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "channel_config.h"
#include "noise_generator.h"
#include "../../shared/sample_images.h"

/* Khởi tạo màn hình OLED SSD1306 qua Software I2C (Cực kỳ ổn định trên ESP32-C3) */
static U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0, 
    /* clock=*/ PIN_OLED_SCL, 
    /* data=*/ PIN_OLED_SDA, 
    /* reset=*/ U8X8_PIN_NONE
);

/* UART0: Kết nối với Máy phát (Tx) */
static HardwareSerial SerialFromTx(0);

/* UART1: Kết nối với Máy thu (Rx) */
static HardwareSerial SerialToRx(1);

static channel_settings_t g_settings = {
    .ber = 0.0f,
    .scope = NOISE_SCOPE_PAYLOAD_ONLY,
    .enabled = true
};

static float g_manual_ber_override = -1.0f; /* < 0: Dùng biến trở ADC; >= 0: Lệnh từ GUI PC */
static uint32_t g_total_frames = 0;
static uint32_t g_last_flipped_bits = 0;
static unsigned long g_last_oled_time = 0;
static unsigned long g_last_adc_time = 0;
static unsigned long g_last_diag_time = 0;

/* Framebuffer ảnh 64x64 sau khi chịu tác động của nhiễu trên Kênh truyền */
static uint8_t g_ch_noisy_image_buf[SAMPLE_IMAGE_SIZE_BYTES] = {0};

/* Bộ lọc trung bình 32 mẫu + Vùng trễ chống rung số cho biến trở ADC */
static uint16_t g_stable_pot_raw = 0;

static float read_filtered_potentiometer() {
    uint32_t sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(PIN_POTENTIOMETER);
    }
    uint16_t current_avg = (uint16_t)(sum / 32);

    /* Vùng trễ (Deadband): Chỉ cập nhật khi người dùng thực sự xoay núm (> 20 nấc) */
    if (abs((int)current_avg - (int)g_stable_pot_raw) > 20 || current_avg < 35 || current_avg > 4060) {
        g_stable_pot_raw = current_avg;
    }

    /* Chốt cận 2 đầu (Deadzone) */
    if (g_stable_pot_raw < 35) {
        g_stable_pot_raw = 0;
    } else if (g_stable_pot_raw > 4060) {
        g_stable_pot_raw = 4095;
    }

    /* Ánh xạ 0..4095 -> BER 0.0% đến 12.0% (0.00 đến 0.12) */
    float ber = ((float)g_stable_pot_raw / 4095.0f) * 0.12f;
    return ber;
}

/* Cập nhật giao diện màn hình OLED 2: CHỈ HIỂN THỊ TÍN HIỆU ẢNH NHIỄU (CĂN GIỮA) */
static void update_oled_display() {
    u8g2.clearBuffer();

    /* Khung viền bao quanh tín hiệu ảnh */
    u8g2.drawFrame(31, 0, 66, 64);

    /* Căn giữa màn hình 128x64: x = 32, y = 0, rộng 64 (8 bytes), cao 64 */
    u8g2.drawBitmap(32, 0, 8, 64, g_ch_noisy_image_buf);

    u8g2.sendBuffer();
}

/* Xử lý lệnh JSON điều khiển từ PC qua cổng USB CDC */
static void process_pc_command(const String &cmd_str) {
    if (cmd_str.indexOf("\"cmd\":\"PING\"") >= 0 || cmd_str.indexOf("\"PING\"") >= 0) {
        noise_stats_t stats;
        noise_gen_get_stats(&stats);
        Serial.printf("{\"node\":\"CH\",\"status\":\"OK\",\"ber\":%.4f,\"frames\":%u,\"flipped\":%u}\n",
                      g_settings.ber, g_total_frames, stats.total_bits_flipped);
    } else if (cmd_str.indexOf("\"cmd\":\"SET_BER\"") >= 0) {
        int pos = cmd_str.indexOf("\"val\":");
        if (pos >= 0) {
            float val = cmd_str.substring(pos + 6).toFloat();
            if (val < 0.0f) {
                g_manual_ber_override = -1.0f; /* Trả quyền lại cho biến trở ADC */
                Serial.println("{\"node\":\"CH\",\"event\":\"BER_CONTROL_ADC\"}");
            } else {
                if (val > 0.15f) val = 0.15f;
                g_manual_ber_override = val;
                g_settings.ber = val;
                update_oled_display();
                Serial.printf("{\"node\":\"CH\",\"event\":\"BER_OVERRIDE\",\"ber\":%.4f}\n", val);
            }
        }
    } else if (cmd_str.indexOf("\"cmd\":\"RESET_STATS\"") >= 0) {
        g_total_frames = 0;
        g_last_flipped_bits = 0;
        memset(g_ch_noisy_image_buf, 0, sizeof(g_ch_noisy_image_buf));
        noise_gen_init();
        update_oled_display();
        Serial.println("{\"node\":\"CH\",\"status\":\"RESET\"}");
    }
}

/* 1. Luồng dữ liệu thuận: Nhận từ Tx -> Trộn nhiễu BSC -> Bắn sang Rx */
static void process_forward_data() {
    static uint8_t rx_buf[FRAME_MAX_TOTAL_LEN];
    static size_t rx_idx = 0;
    static uint8_t exp_len = 0;

    while (SerialFromTx.available() > 0) {
        uint8_t b = SerialFromTx.read();

        if (rx_idx == 0) {
            if (b == FRAME_SOF_DATA) {
                rx_buf[rx_idx++] = b;
            }
        } else {
            rx_buf[rx_idx++] = b;

            /* Lấy độ dài payload từ header byte 4 */
            if (rx_idx == FRAME_HEADER_LEN) {
                exp_len = rx_buf[4];
            }

            /* Khi đã nhận đủ khung: HEADER(5) + PAYLOAD(exp_len) + CRC(2) + EOF(1) */
            if (rx_idx >= FRAME_HEADER_LEN && rx_idx == (size_t)(FRAME_HEADER_LEN + exp_len + 3)) {
                if (rx_buf[rx_idx - 1] == FRAME_EOF) {
                    /* Gây nhiễu BSC trên Payload & Parity (bảo vệ 5 byte Header) */
                    size_t flipped = 0;
                    if (g_settings.enabled && g_settings.ber > 0.0001f && exp_len > 0) {
                        noise_gen_apply_bsc(rx_buf, FRAME_HEADER_LEN, exp_len, g_settings.ber, &flipped);
                    }
                    g_last_flipped_bits = flipped;
                    g_total_frames++;

                    /* Chuyển tiếp khung đã gây nhiễu sang Máy thu (Rx) */
                    SerialToRx.write(rx_buf, rx_idx);
                    SerialToRx.flush();

                    /* Tái tạo mảnh ảnh 128 byte tương ứng chịu đúng mức nhiễu BSC này */
                    uint8_t seq = rx_buf[1];
                    uint8_t img_idx = (rx_buf[2] >> 4) & 0x01;
                    uint8_t chunk_idx = seq % SAMPLE_IMAGE_NUM_CHUNKS;

                    static uint8_t s_channel_last_img_idx = 0xFF;
                    if (img_idx != s_channel_last_img_idx) {
                        s_channel_last_img_idx = img_idx;
                        const uint8_t *full_src = g_sample_images[img_idx];
                        memcpy(g_ch_noisy_image_buf, full_src, SAMPLE_IMAGE_SIZE_BYTES);
                        if (g_settings.enabled && g_settings.ber > 0.0001f) {
                            size_t f_all = 0;
                            noise_gen_apply_bsc(g_ch_noisy_image_buf, 0, SAMPLE_IMAGE_SIZE_BYTES, g_settings.ber, &f_all);
                        }
                        update_oled_display(); /* Hiển thị ngay ảnh mới kèm nhiễu */
                    }

                    const uint8_t *src_chunk = g_sample_images[img_idx] + chunk_idx * SAMPLE_IMAGE_CHUNK_SIZE;
                    uint8_t noisy_chunk[SAMPLE_IMAGE_CHUNK_SIZE];
                    memcpy(noisy_chunk, src_chunk, SAMPLE_IMAGE_CHUNK_SIZE);

                    if (g_settings.enabled && g_settings.ber > 0.0001f) {
                        size_t img_flipped = 0;
                        noise_gen_apply_bsc(noisy_chunk, 0, SAMPLE_IMAGE_CHUNK_SIZE, g_settings.ber, &img_flipped);
                    }

                    memcpy(&g_ch_noisy_image_buf[chunk_idx * SAMPLE_IMAGE_CHUNK_SIZE], noisy_chunk, SAMPLE_IMAGE_CHUNK_SIZE);

                    /* Chuyển mảnh ảnh nhiễu thành Hex để GUI PC vẽ thời gian thực */
                    char hex_noisy[SAMPLE_IMAGE_CHUNK_SIZE * 2 + 1];
                    for (size_t i = 0; i < SAMPLE_IMAGE_CHUNK_SIZE; i++) {
                        sprintf(&hex_noisy[i * 2], "%02X", noisy_chunk[i]);
                    }
                    hex_noisy[sizeof(hex_noisy) - 1] = '\0';

                    /* Xuất JSON Telemetry chuẩn hóa kèm giá trị biến trở và chuỗi Hex ảnh nhiễu */
                    Serial.printf("{\"node\":\"CH\",\"event\":\"FORWARD\",\"seq\":%u,\"frames\":%u,\"ber\":%.4f,\"flipped\":%u,\"chunk\":%d,\"pot\":%u,\"hex\":\"%s\"}\n",
                                  rx_buf[1], g_total_frames, g_settings.ber, (unsigned)flipped, chunk_idx + 1, g_stable_pot_raw, hex_noisy);
                }
                rx_idx = 0; /* Reset cho khung tiếp theo */
            } else if (rx_idx >= FRAME_MAX_TOTAL_LEN) {
                rx_idx = 0;
            }
        }
    }
}

/* 2. Luồng phản hồi ngược: Nhận Feedback từ Rx -> Forward nguyên vẹn về Tx */
static void process_backward_feedback() {
    static uint8_t fb_buf[FEEDBACK_FRAME_LEN];
    static size_t fb_idx = 0;

    while (SerialToRx.available() > 0) {
        uint8_t b = SerialToRx.read();

        if (fb_idx == 0) {
            if (b == FRAME_SOF_FEEDBACK) {
                fb_buf[fb_idx++] = b;
            }
        } else {
            fb_buf[fb_idx++] = b;
            if (fb_idx == FEEDBACK_FRAME_LEN) {
                if (fb_buf[FEEDBACK_FRAME_LEN - 1] == FRAME_EOF) {
                    /* Forward nguyên vẹn về Máy phát qua UART0 */
                    SerialFromTx.write(fb_buf, FEEDBACK_FRAME_LEN);
                    SerialFromTx.flush();

                    /* Xuất JSON Telemetry phản hồi */
                    Serial.printf("{\"node\":\"CH\",\"event\":\"FB_FORWARD\",\"seq\":%u,\"rec_mode\":%u,\"rx_ber\":%.3f}\n",
                                  fb_buf[1], fb_buf[4], (float)fb_buf[2] / 10.0f);
                }
                fb_idx = 0;
            }
        }
    }
}

void setup() {
    Serial.begin(115200); /* USB CDC debug */

    /* Khởi tạo ADC đọc biến trở */
    analogReadResolution(12); /* 12-bit ADC (0..4095) */
    pinMode(PIN_POTENTIOMETER, INPUT);

    /* Khởi tạo bộ sinh số ngẫu nhiên cho nhiễu */
    noise_gen_init();

    /* Khởi tạo màn hình OLED 2 */
    u8g2.begin();
    u8g2.setContrast(255);

    /* UART0: Kết nối với Máy phát Tx (RX: 20, TX: 21) */
    SerialFromTx.begin(CHANNEL_BAUDRATE, SERIAL_8N1, PIN_RX_FROM_TX, PIN_TX_TO_TX);

    /* UART1: Kết nối với Máy thu Rx (RX: 4, TX: 5) */
    SerialToRx.begin(CHANNEL_BAUDRATE, SERIAL_8N1, PIN_RX_FROM_RX, PIN_TX_TO_RX);

    update_oled_display();
    Serial.println("{\"node\":\"CH\",\"status\":\"READY\"}");
}

void loop() {
    /* 1. Tiếp nhận lệnh điều khiển từ PC qua cổng USB */
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() > 0) {
            process_pc_command(cmd);
        }
    }

    /* 2. Định kỳ đọc biến trở ADC nếu không bị ghi đè thủ công từ GUI */
    if (g_manual_ber_override < 0.0f) {
        if (millis() - g_last_adc_time >= 50) {
            g_last_adc_time = millis();
            float new_ber = read_filtered_potentiometer();
            if (fabs(new_ber - g_settings.ber) > 0.002f) {
                g_settings.ber = new_ber;
            }
        }
    }

    /* 3. Xử lý luồng thuận: Tx -> Channel -> Rx */
    process_forward_data();

    /* 4. Xử lý luồng ngược: Rx -> Channel -> Tx */
    process_backward_feedback();

    /* 5. Cập nhật OLED hiển thị tần số cao (30ms ~ 33 FPS) */
    if (millis() - g_last_oled_time >= 30) {
        g_last_oled_time = millis();
        update_oled_display();
    }

    /* 6. In chẩn đoán giá trị ADC chân biến trở */
    if (millis() - g_last_diag_time >= 1000) {
        g_last_diag_time = millis();
        uint16_t a0 = analogRead(0);
        uint16_t a1 = analogRead(1);
        uint16_t a2 = analogRead(2);
        uint16_t a3 = analogRead(3);
        Serial.printf("{\"node\":\"CH\",\"diag\":\"ADC\",\"A0\":%u,\"A1\":%u,\"A2\":%u,\"A3\":%u,\"ber\":%.4f,\"raw\":%u}\n",
                      a0, a1, a2, a3, g_settings.ber, g_stable_pot_raw);
    }
}
