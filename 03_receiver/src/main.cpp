#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "rx_config.h"
#include "frame_parser.h"
#include "chn_decoder_hamming.h"
#include "chn_decoder_cyclic.h"
#include "src_decoder_huffman.h"
#include "../../shared/sample_images.h"
#include "../../shared/crc16.h"

/* Khởi tạo màn hình OLED SSD1306 qua Software I2C (Cực kỳ ổn định trên ESP32-C3) */
static U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0, 
    /* clock=*/ PIN_OLED_SCL, 
    /* data=*/ PIN_OLED_SDA, 
    /* reset=*/ U8X8_PIN_NONE
);

/* UART1 kết nối 2 chiều với bo Kênh truyền (RX: 20, TX: 21) */
static HardwareSerial SerialFromChannel(1);

/* Framebuffer ảnh tái tạo 64x64 (512 bytes) - Dùng double buffer chống xé hình và giật */
static uint8_t g_rx_image_buf[SAMPLE_IMAGE_SIZE_BYTES] = {0};
static uint8_t s_assemble_buf[SAMPLE_IMAGE_SIZE_BYTES] = {0};
static uint8_t s_chunks_received_mask = 0;
static unsigned long s_last_chunk_time = 0;

/* Thống kê đánh giá hệ thống (Metrics) */
static uint32_t g_total_bits_received = 0;
static uint32_t g_total_bits_corrected = 0;
static uint32_t g_total_frames_received = 0;
static uint32_t g_raw_bytes_in = 0;
static uint8_t g_last_raw_byte = 0;
static unsigned long g_last_diag_time = 0;
static channel_alg_t g_current_mode = CHN_ALG_NONE;
static channel_alg_t g_recommended_mode = CHN_ALG_NONE;
static float g_measured_ber = 0.0f;
static unsigned long g_last_oled_time = 0;
static unsigned long g_last_feedback_time = 0;
static uint8_t g_fb_seq = 0;
static bool s_oled_needs_update = true;
static bool g_crc_check_enabled = true; /* Cờ Bật / Tắt kiểm tra CRC16 */

/* Giám sát công tắc gạt 3 nấc chuyển Baudrate tức thì */
static uint32_t g_current_baudrate = RX_UART_BAUDRATE;
static unsigned long g_last_baud_sw_time = 0;

static uint32_t read_switch_baudrate() {
    int sw1 = digitalRead(PIN_SW_BAUD_1); // GPIO 2 (kéo lên 1)
    int sw2 = digitalRead(PIN_SW_BAUD_2); // GPIO 3 (kéo lên 1)

    if (sw1 == 0 && sw2 == 1) {
        return 9600;    /* Nấc 1: Gạt sang TRÁI (GPIO 2 chạm GND) */
    } else if (sw1 == 1 && sw2 == 1) {
        return 38400;   /* Nấc 2: Ở GIỮA (hở mạch cả 2 bên) */
    } else if (sw1 == 1 && sw2 == 0) {
        return 115200;  /* Nấc 3: Gạt sang PHẢI (GPIO 3 chạm GND) */
    }
    return 115200;      /* Mặc định an toàn */
}

static void check_and_update_baudrate() {
    if (millis() - g_last_baud_sw_time < 50) return; // Debounce 50ms
    g_last_baud_sw_time = millis();

    uint32_t target_baud = read_switch_baudrate();
    if (target_baud != g_current_baudrate) {
        g_current_baudrate = target_baud;
        SerialFromChannel.flush();
        SerialFromChannel.begin(g_current_baudrate, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
        Serial.printf("{\"node\":\"RX\",\"event\":\"BAUD_CHANGE\",\"baud\":%u}\n", g_current_baudrate);
    }
}

/* 3 Chân đèn LED RGB trên bo Máy thu: Đỏ (GPIO 5), Xanh lá (GPIO 6), Xanh dương (GPIO 7) */
/* Cực âm nối chung GND bo Máy thu, Cực dương qua trở 220Ω vào chân GPIO */
#define LED_BRIGHTNESS_DEFAULT  0.01f  /* Cố định độ sáng 1% (duty tối đa 3 / 255) */

static bool g_led_test_override = false; /* Cờ ưu tiên kiểm tra LED từ PC (ngăn hàm nhận ảnh tự đè LED) */

static void set_rgb_leds_pwm(uint8_t r, uint8_t g, uint8_t b, float brightness = LED_BRIGHTNESS_DEFAULT) {
    uint32_t duty_r = (uint32_t)(r * brightness + 0.5f);
    uint32_t duty_g = (uint32_t)(g * brightness + 0.5f);
    uint32_t duty_b = (uint32_t)(b * brightness + 0.5f);

    /* Cực âm nối chung GND: Kích mức HIGH (xung dương) để sáng */
    analogWrite(PIN_RGB_R, duty_r);
    analogWrite(PIN_RGB_G, duty_g);
    analogWrite(PIN_RGB_B, duty_b);
}

static void set_rgb_leds_digital(int r, int g, int b) {
    pinMode(PIN_RGB_R, OUTPUT);
    pinMode(PIN_RGB_G, OUTPUT);
    pinMode(PIN_RGB_B, OUTPUT);
    digitalWrite(PIN_RGB_R, r ? HIGH : LOW);
    digitalWrite(PIN_RGB_G, g ? HIGH : LOW);
    digitalWrite(PIN_RGB_B, b ? HIGH : LOW);
}

static void leds_init() {
    pinMode(PIN_RGB_R, OUTPUT);
    pinMode(PIN_RGB_G, OUTPUT);
    pinMode(PIN_RGB_B, OUTPUT);
    set_rgb_leds_pwm(0, 0, 0);
}

/* Tự động chỉ thị Mode thích nghi theo mức BER đo được (độ sáng 10%) */
static void update_led_indicators(channel_alg_t mode) {
    if (g_led_test_override) {
        return; /* Đang trong chế độ kiểm tra từ PC, giữ nguyên trạng thái test */
    }
    switch (mode) {
        case CHN_ALG_NONE:
            set_rgb_leds_pwm(0, 255, 0);   /* Mode Direct: LED Xanh lá (chân 6) sáng 10% */
            break;
        case CHN_ALG_HAMMING_84:
            set_rgb_leds_pwm(0, 0, 255);   /* Mode Hamming: LED Xanh dương (chân 7) sáng 10% */
            break;
        case CHN_ALG_CYCLIC_74:
            set_rgb_leds_pwm(255, 0, 0);   /* Mode Cyclic: LED Đỏ (chân 5) sáng 10% */
            break;
        default:
            set_rgb_leds_pwm(0, 0, 0);
            break;
    }
}

/* Cập nhật giao diện màn hình OLED 3: CHỈ HIỂN THỊ TÍN HIỆU ẢNH KHÔI PHỤC (CĂN GIỮA) */
static void update_oled_display() {
    u8g2.setPowerSave(0);
    u8g2.clearBuffer();

    /* Khung viền bao quanh tín hiệu ảnh */
    u8g2.drawFrame(31, 0, 66, 64);

    /* Căn giữa màn hình 128x64: x = 32, y = 0, rộng 64 (8 bytes), cao 64 */
    u8g2.drawBitmap(32, 0, 8, 64, g_rx_image_buf);

    u8g2.sendBuffer();
}

/* Gửi gói tin Feedback ngược về Máy phát qua Kênh */
static void send_feedback_packet() {
    feedback_frame_t fb;
    fb.sof = FRAME_SOF_FEEDBACK;
    fb.seq = g_fb_seq++;
    fb.measured_ber_x10 = (uint8_t)(g_measured_ber * 1000.0f);
    fb.corrected_bits = (uint16_t)g_total_bits_corrected;
    fb.recommended_mode = (uint8_t)g_recommended_mode;
    
    /* Tính CRC16 trên các byte từ seq đến recommended_mode (5 bytes) */
    fb.crc16 = crc16_ccitt((const uint8_t*)&fb.seq, 5);
    fb.eof = FRAME_EOF;

    SerialFromChannel.write((const uint8_t*)&fb, sizeof(fb));
    SerialFromChannel.flush();

    Serial.printf("{\"node\":\"RX\",\"event\":\"FB_SENT\",\"seq\":%u,\"rec_mode\":%d,\"ber\":%.2f,\"raw_bytes\":%u,\"frames\":%u}\n",
                  fb.seq, (int)g_recommended_mode, g_measured_ber * 100.0f, g_raw_bytes_in, g_total_frames_received);
}

/* Xử lý lệnh JSON điều khiển từ PC qua cổng USB CDC */
static void process_pc_command(const String &cmd_str) {
    if (cmd_str.indexOf("\"cmd\":\"PING\"") >= 0 || cmd_str.indexOf("\"PING\"") >= 0) {
        Serial.printf("{\"node\":\"RX\",\"status\":\"OK\",\"frames\":%u,\"fixed\":%u,\"ber\":%.2f,\"mode\":%d,\"crc_en\":%s}\n",
                      g_total_frames_received, g_total_bits_corrected, g_measured_ber * 100.0f, (int)g_current_mode,
                      g_crc_check_enabled ? "true" : "false");
    } else if (cmd_str.indexOf("\"cmd\":\"SET_CRC_CHECK\"") >= 0) {
        if (cmd_str.indexOf("\"enabled\":false") >= 0 || cmd_str.indexOf("\"val\":0") >= 0) {
            g_crc_check_enabled = false;
        } else {
            g_crc_check_enabled = true;
        }
        Serial.printf("{\"node\":\"RX\",\"event\":\"CRC_CHECK_CHANGED\",\"enabled\":%s}\n",
                      g_crc_check_enabled ? "true" : "false");
    } else if (cmd_str.indexOf("\"cmd\":\"RESET_STATS\"") >= 0) {
        g_total_bits_received = 0;
        g_total_bits_corrected = 0;
        g_total_frames_received = 0;
        g_measured_ber = 0.0f;
        memset(g_rx_image_buf, 0, sizeof(g_rx_image_buf));
        update_oled_display();
        Serial.println("{\"node\":\"RX\",\"status\":\"RESET\"}");
    } else if (cmd_str.indexOf("\"cmd\":\"SET_LED\"") >= 0 || cmd_str.indexOf("\"cmd\":\"SET_RGB\"") >= 0 ||
               cmd_str.startsWith("SET_LED") || cmd_str.startsWith("SET_RGB")) {
        g_led_test_override = true; /* Khóa ưu tiên kiểm tra từ PC, ngăn hàm nhận ảnh tự đè LED */
        int r = 0, g = 0, b = 0;
        int bright_pct = 1; /* Mặc định 1% */

        int b_pct_idx = cmd_str.indexOf("\"pct\":");
        if (b_pct_idx >= 0) bright_pct = cmd_str.substring(b_pct_idx + 6).toInt();

        /* Đọc các tham số r (Đỏ - GPIO 5), g (Xanh lá - GPIO 6), b (Xanh dương - GPIO 7) */
        int r_idx = cmd_str.indexOf("\"r\":");
        if (r_idx >= 0) r = cmd_str.substring(r_idx + 4).toInt();
        int g_idx = cmd_str.indexOf("\"g\":");
        if (g_idx >= 0) g = cmd_str.substring(g_idx + 4).toInt();
        int b_idx = cmd_str.indexOf("\"b\":");
        if (b_idx >= 0) b = cmd_str.substring(b_idx + 4).toInt();

        /* Hỗ trợ thêm trường hợp gửi y (Vàng) thay cho b */
        int y_idx = cmd_str.indexOf("\"y\":");
        if (y_idx >= 0 && b_idx < 0) b = cmd_str.substring(y_idx + 4).toInt();

        /* Hỗ trợ text thô: SET_RGB r g b [pct] */
        if (cmd_str.startsWith("SET_LED") || cmd_str.startsWith("SET_RGB")) {
            int pr = 0, pg = 0, pb = 0, ppct = 1;
            int n = sscanf(cmd_str.c_str(), "%*s %d %d %d %d", &pr, &pg, &pb, &ppct);
            if (n >= 3) {
                r = pr; g = pg; b = pb;
                if (n >= 4) bright_pct = ppct;
            }
        }

        r = constrain(r, 0, 255);
        g = constrain(g, 0, 255);
        b = constrain(b, 0, 255);
        bright_pct = constrain(bright_pct, 1, 100);

        float bright_factor = (float)bright_pct / 100.0f;
        set_rgb_leds_pwm((uint8_t)r, (uint8_t)g, (uint8_t)b, bright_factor);

        uint32_t duty_r = (uint32_t)(r * bright_factor + 0.5f);
        uint32_t duty_g = (uint32_t)(g * bright_factor + 0.5f);
        uint32_t duty_b = (uint32_t)(b * bright_factor + 0.5f);

        Serial.printf("{\"node\":\"RX\",\"event\":\"RGB_SET\",\"r\":%d,\"g\":%d,\"b\":%d,\"duty_r\":%u,\"duty_g\":%u,\"duty_b\":%u,\"pct\":%d}\n",
                      r, g, b, duty_r, duty_g, duty_b, bright_pct);
    } else if (cmd_str.indexOf("\"cmd\":\"RELEASE_LED\"") >= 0 || cmd_str.startsWith("RELEASE_LED")) {
        g_led_test_override = false; /* Trả quyền điều khiển LED lại cho luồng ảnh hệ thống */
        channel_alg_t active_led_mode = (g_current_mode != CHN_ALG_NONE) ? g_current_mode : g_recommended_mode;
        update_led_indicators(active_led_mode);
        Serial.println("{\"node\":\"RX\",\"event\":\"LED_RELEASED\"}");
    } else if (cmd_str.indexOf("\"cmd\":\"SET_RAW\"") >= 0 || cmd_str.startsWith("SET_RAW")) {
        g_led_test_override = true;
        int r = 0, g = 0, b = 0;
        int idxr = cmd_str.indexOf("\"r\":");
        if (idxr >= 0) r = cmd_str.substring(idxr + 4).toInt();
        int idxg = cmd_str.indexOf("\"g\":");
        if (idxg >= 0) g = cmd_str.substring(idxg + 4).toInt();
        int idxb = cmd_str.indexOf("\"b\":");
        if (idxb >= 0) b = cmd_str.substring(idxb + 4).toInt();

        /* Nếu dùng p5, p6, p7 */
        int idx5 = cmd_str.indexOf("\"p5\":");
        if (idx5 >= 0) r = cmd_str.substring(idx5 + 5).toInt();
        int idx6 = cmd_str.indexOf("\"p6\":");
        if (idx6 >= 0) g = cmd_str.substring(idx6 + 5).toInt();
        int idx7 = cmd_str.indexOf("\"p7\":");
        if (idx7 >= 0) b = cmd_str.substring(idx7 + 5).toInt();

        if (cmd_str.startsWith("SET_RAW")) {
            sscanf(cmd_str.c_str(), "SET_RAW %d %d %d", &r, &g, &b);
        }

        set_rgb_leds_digital(r, g, b);
        Serial.printf("{\"node\":\"RX\",\"event\":\"RAW_SET\",\"r\":%d,\"g\":%d,\"b\":%d}\n", r, g, b);
    }
}

/* Xử lý khung dữ liệu nhận được từ Kênh */
static void process_received_frame(const parsed_frame_t *frame) {
    uint8_t comp_buf[FRAME_MAX_PAYLOAD_LEN];
    size_t comp_len = 0;
    size_t single_errors = 0;
    size_t double_errors = 0;

    g_current_mode = (channel_alg_t)frame->chn_alg;
    g_total_frames_received++;
    g_total_bits_received += frame->payload_len * 8;

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

    g_total_bits_corrected += single_errors;

    /* 2. GIẢI NÉN NGUỒN (Xử lý trực tiếp mảnh Bitmap 128 bytes) */
    uint8_t recovered_chunk[SAMPLE_IMAGE_CHUNK_SIZE];
    size_t raw_len = 0;

    if (comp_len == SAMPLE_IMAGE_CHUNK_SIZE) {
        memcpy(recovered_chunk, comp_buf, SAMPLE_IMAGE_CHUNK_SIZE);
        raw_len = SAMPLE_IMAGE_CHUNK_SIZE;
    } else if (frame->src_alg == SRC_ALG_HUFFMAN) {
        raw_len = huffman_decode(comp_buf, comp_len, recovered_chunk, sizeof(recovered_chunk));
    } else {
        raw_len = (comp_len < sizeof(recovered_chunk)) ? comp_len : sizeof(recovered_chunk);
        memcpy(recovered_chunk, comp_buf, raw_len);
    }

    /* 3. TÁI TẠO VÀO BUFFER HỘI TỤ ẢNH 64x64 */
    static uint8_t s_last_img_idx = 0xFF;
    if (frame->img_idx != s_last_img_idx) {
        s_last_img_idx = frame->img_idx;
        memset(s_assemble_buf, 0, sizeof(s_assemble_buf));
        memset(g_rx_image_buf, 0, sizeof(g_rx_image_buf));
        s_chunks_received_mask = 0;
    }

    uint8_t chunk_idx = frame->seq % SAMPLE_IMAGE_NUM_CHUNKS;
    if (raw_len == SAMPLE_IMAGE_CHUNK_SIZE) {
        size_t offset = chunk_idx * SAMPLE_IMAGE_CHUNK_SIZE;
        memcpy(&s_assemble_buf[offset], recovered_chunk, SAMPLE_IMAGE_CHUNK_SIZE);
        memcpy(&g_rx_image_buf[offset], recovered_chunk, SAMPLE_IMAGE_CHUNK_SIZE);
        s_chunks_received_mask |= (1 << chunk_idx);
        s_last_chunk_time = millis();
        s_oled_needs_update = true;
    }

    /* 4. THUẬT TOÁN THÍCH NGHI VỚI HYSTERESIS */
    if (g_total_bits_received > 0) {
        float instant_ber = 0.0f;
        if (frame->chn_alg == CHN_ALG_NONE) {
            /* Ở chế độ Direct: nếu bật kiểm tra CRC và CRC hỏng thì đánh giá có lỗi kênh */
            if (g_crc_check_enabled && !frame->crc_valid) {
                instant_ber = 0.025f;
            }
        } else {
            instant_ber = (float)(single_errors + double_errors * 2) / (float)(frame->payload_len * 8);
        }
        g_measured_ber = 0.8f * g_measured_ber + 0.2f * instant_ber;

        if (g_measured_ber < 0.008f && double_errors == 0) {
            g_recommended_mode = CHN_ALG_NONE;
        } else if (g_measured_ber <= 0.035f && double_errors == 0) {
            g_recommended_mode = CHN_ALG_HAMMING_84;
        } else {
            g_recommended_mode = CHN_ALG_CYCLIC_74;
        }
    }

    /* Bật đèn LED tương ứng với Mode thích nghi theo mức BER đo được */
    channel_alg_t active_led_mode = (g_current_mode != CHN_ALG_NONE) ? g_current_mode : g_recommended_mode;
    update_led_indicators(active_led_mode);

    /* Chuyển 128 byte chunk thành chuỗi Hex để GUI PC hiển thị ảnh tức thì */
    char hex_str[SAMPLE_IMAGE_CHUNK_SIZE * 2 + 1];
    for (size_t i = 0; i < SAMPLE_IMAGE_CHUNK_SIZE; i++) {
        sprintf(&hex_str[i * 2], "%02X", recovered_chunk[i]);
    }
    hex_str[sizeof(hex_str) - 1] = '\0';

    /* Xuất JSON Telemetry chuẩn hóa */
    Serial.printf("{\"node\":\"RX\",\"seq\":%d,\"mode\":%d,\"fixed\":%u,\"total_fixed\":%u,\"ber\":%.2f,\"chunk\":%d,\"hex\":\"%s\"}\n",
                  frame->seq, (int)frame->chn_alg, (unsigned)single_errors,
                  g_total_bits_corrected, g_measured_ber * 100.0f, chunk_idx + 1, hex_str);

    /* Khi đã nhận đủ chu kỳ 8 mảnh, reset mask cho chu kỳ tiếp theo */
    if (chunk_idx == (SAMPLE_IMAGE_NUM_CHUNKS - 1) || s_chunks_received_mask == 0xFF) {
        s_chunks_received_mask = 0;
    }
}

void setup() {
    Serial.begin(115200);

    /* Khởi tạo PWM điều khiển 3 LED Xanh (6), Vàng (7), Đỏ (10) - Độ sáng 30% */
    leds_init();

    /* Khởi tạo bộ bóc tách khung UART */
    frame_parser_init();

    /* Khởi tạo màn hình OLED 3 */
    u8g2.begin();
    u8g2.setContrast(255);

    /* Khởi tạo 2 chân công tắc 3 nấc chuyển Baudrate */
    pinMode(PIN_SW_BAUD_1, INPUT_PULLUP);
    pinMode(PIN_SW_BAUD_2, INPUT_PULLUP);
    g_current_baudrate = read_switch_baudrate();

    /* UART0: Kết nối 2 chiều với Kênh truyền (RX: 20, TX: 21) theo Baudrate ban đầu */
    SerialFromChannel.begin(g_current_baudrate, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

    update_oled_display();
    Serial.println("{\"node\":\"RX\",\"status\":\"READY\"}");
}

void loop() {
    /* 0. Quét công tắc 3 nấc để đồng bộ Baudrate tức thời */
    check_and_update_baudrate();

    /* 1. Tiếp nhận lệnh điều khiển từ PC qua cổng USB */
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() > 0) {
            process_pc_command(cmd);
        }
    }

    /* 2. Nhận và phân giải từng byte dữ liệu từ Kênh */
    while (SerialFromChannel.available() > 0) {
        uint8_t b = SerialFromChannel.read();
        g_raw_bytes_in++;
        g_last_raw_byte = b;
        parsed_frame_t frame;
        if (frame_parser_process_byte(b, &frame)) {
            process_received_frame(&frame);
        }
    }

    /* 3. Định kỳ gửi gói tin Feedback thích nghi ngược về Kênh */
    if (millis() - g_last_feedback_time >= 350) {
        g_last_feedback_time = millis();
        send_feedback_packet();
    }

    /* 4. Định kỳ in chẩn đoán đường UART kết nối */
    if (millis() - g_last_diag_time >= 1000) {
        g_last_diag_time = millis();
        Serial.printf("{\"node\":\"RX\",\"diag\":\"UART_STATUS\",\"raw_bytes\":%u,\"frames\":%u,\"last_byte\":\"0x%02X\"}\n",
                      g_raw_bytes_in, g_total_frames_received, g_last_raw_byte);
    }

    /* 5. Cập nhật giao diện màn hình OLED: Tần số cao (30ms ~ 33 FPS) hoặc khi có chunk mới */
    if (s_oled_needs_update || (millis() - g_last_oled_time >= 30)) {
        g_last_oled_time = millis();
        s_oled_needs_update = false;
        update_oled_display();
    }
}
