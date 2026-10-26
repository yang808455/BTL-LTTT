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

/* Tắt hoàn toàn 3 đèn LED chỉ thị ở máy thu theo yêu cầu người dùng */
static void update_led_indicators(channel_alg_t mode) {
    (void)mode;
    pinMode(PIN_LED_GREEN, INPUT);
    pinMode(PIN_LED_YELLOW, INPUT);
    pinMode(PIN_LED_RED, INPUT);
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
        Serial.printf("{\"node\":\"RX\",\"status\":\"OK\",\"frames\":%u,\"fixed\":%u,\"ber\":%.2f,\"mode\":%d}\n",
                      g_total_frames_received, g_total_bits_corrected, g_measured_ber * 100.0f, (int)g_current_mode);
    } else if (cmd_str.indexOf("\"cmd\":\"RESET_STATS\"") >= 0) {
        g_total_bits_received = 0;
        g_total_bits_corrected = 0;
        g_total_frames_received = 0;
        g_measured_ber = 0.0f;
        memset(g_rx_image_buf, 0, sizeof(g_rx_image_buf));
        update_oled_display();
        Serial.println("{\"node\":\"RX\",\"status\":\"RESET\"}");
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
            /* Ở chế độ Direct không có parity: nếu CRC hỏng, đánh giá có lỗi kênh */
            if (!frame->crc_valid) {
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

    /* Tắt hoàn toàn 3 chân LED máy thu theo yêu cầu người dùng */
    pinMode(PIN_LED_GREEN, INPUT);
    pinMode(PIN_LED_YELLOW, INPUT);
    pinMode(PIN_LED_RED, INPUT);

    /* Khởi tạo bộ bóc tách khung UART */
    frame_parser_init();

    /* Khởi tạo màn hình OLED 3 */
    u8g2.begin();
    u8g2.setContrast(255);

    /* UART0: Kết nối 2 chiều với Kênh truyền (RX: 20, TX: 21) */
    SerialFromChannel.begin(RX_UART_BAUDRATE, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

    update_oled_display();
    Serial.println("{\"node\":\"RX\",\"status\":\"READY\"}");
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
