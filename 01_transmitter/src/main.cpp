#include <Arduino.h>
#include <U8g2lib.h>
#include "tx_config.h"
#include "frame_builder.h"
#include "../../shared/sample_images.h"
#include "../../shared/crc16.h"

/* Khởi tạo màn hình OLED SSD1306 qua Software I2C (Ổn định tuyệt đối trên ESP32-C3) */
static U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0, 
    /* clock=*/ PIN_OLED_SCL, 
    /* data=*/ PIN_OLED_SDA, 
    /* reset=*/ U8X8_PIN_NONE
);

/* UART1 kết nối 2 chiều với bo Kênh truyền */
static HardwareSerial SerialChannel(1);

static tx_settings_t g_tx_settings = {
    .src_alg = SRC_ALG_NONE,    /* Truyền trực tiếp ảnh Bitmap 128 bytes/chunk (không qua bảng ASCII Huffman) */
    .chn_alg = CHN_ALG_NONE,    /* Bắt đầu ở chế độ Direct (Kênh tốt) */
    .seq = 0,
    .img_idx = 0
};

static uint8_t g_current_image_idx = 0;
static uint8_t g_current_chunk_idx = 0;
static uint8_t g_custom_image_buf[SAMPLE_IMAGE_SIZE_BYTES] = {0};
static bool g_use_custom_image = false;
static uint8_t g_custom_img_version = 100;
static unsigned long g_last_send_time = 0;
static unsigned long g_last_btn_time = 0;
static uint8_t g_last_rec_mode = CHN_ALG_NONE;
static float g_last_rx_ber = 0.0f;
static bool g_auto_adaptive = true; /* Bật / Tắt chế độ tự thích nghi */

static const uint8_t* get_active_image_ptr() {
    if (g_use_custom_image) {
        return g_custom_image_buf;
    }
    return g_sample_images[g_current_image_idx];
}

static int parse_hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Giám sát công tắc gạt 3 nấc chuyển Baudrate tức thì */
static uint32_t g_current_baudrate = TX_UART_BAUDRATE;
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
        SerialChannel.flush();
        SerialChannel.begin(g_current_baudrate, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
        Serial.printf("{\"node\":\"TX\",\"event\":\"BAUD_CHANGE\",\"baud\":%u}\n", g_current_baudrate);
    }
}

/* Cập nhật giao diện màn hình OLED 1: CHỈ HIỂN THỊ TÍN HIỆU ẢNH THUẦN TÚY (CĂN GIỮA) */
static void update_oled_display() {
    u8g2.clearBuffer();

    /* Khung viền bao quanh tín hiệu ảnh */
    u8g2.drawFrame(31, 0, 66, 64);

    /* Căn giữa màn hình 128x64: x = 32, y = 0, rộng 64 (8 bytes), cao 64 */
    const uint8_t *img_ptr = get_active_image_ptr();
    u8g2.drawBitmap(32, 0, 8, 64, img_ptr);

    u8g2.sendBuffer();
}

/* Xử lý lệnh JSON từ PC Dashboard qua cổng USB CDC */
static void process_pc_command(const String &cmd_str) {
    if (cmd_str.indexOf("PING") >= 0) {
        Serial.printf("{\"node\":\"TX\",\"status\":\"OK\",\"img_idx\":%d,\"is_custom\":%s,\"chn_alg\":%d,\"seq\":%d}\n",
                      g_use_custom_image ? g_custom_img_version : g_current_image_idx,
                      g_use_custom_image ? "true" : "false",
                      (int)g_tx_settings.chn_alg, g_tx_settings.seq);
    } else if (cmd_str.indexOf("LOAD_IMAGE_HEX") >= 0) {
        int hex_key = cmd_str.indexOf("\"hex\"");
        if (hex_key >= 0) {
            int val_quote_start = cmd_str.indexOf("\"", hex_key + 5);
            if (val_quote_start >= 0) {
                int val_start = val_quote_start + 1;
                int val_end = cmd_str.indexOf("\"", val_start);
                if (val_end > val_start) {
                    String hex_data = cmd_str.substring(val_start, val_end);
                    size_t parsed_bytes = 0;
                    for (size_t i = 0; i + 1 < hex_data.length() && parsed_bytes < SAMPLE_IMAGE_SIZE_BYTES; i += 2) {
                        int hi = parse_hex_nibble(hex_data.charAt(i));
                        int lo = parse_hex_nibble(hex_data.charAt(i + 1));
                        if (hi >= 0 && lo >= 0) {
                            g_custom_image_buf[parsed_bytes++] = (uint8_t)((hi << 4) | lo);
                        } else {
                            break;
                        }
                    }
                    if (parsed_bytes == SAMPLE_IMAGE_SIZE_BYTES) {
                        g_use_custom_image = true;
                        g_custom_img_version++;
                        if (g_custom_img_version < 100) g_custom_img_version = 100;
                        g_tx_settings.img_idx = g_custom_img_version;
                        g_current_chunk_idx = 0;
                        g_last_send_time = 0;
                        update_oled_display();
                        Serial.printf("{\"node\":\"TX\",\"event\":\"CUSTOM_IMAGE_LOADED\",\"size\":%u,\"version\":%u}\n",
                                      (unsigned)parsed_bytes, (unsigned)g_custom_img_version);
                    } else {
                        Serial.printf("{\"node\":\"TX\",\"error\":\"INVALID_HEX_LENGTH\",\"received\":%u,\"expected\":%u}\n",
                                      (unsigned)parsed_bytes, (unsigned)SAMPLE_IMAGE_SIZE_BYTES);
                    }
                }
            }
        }
    } else if (cmd_str.indexOf("SET_IMAGE") >= 0) {
        int idx_pos = cmd_str.indexOf("\"idx\"");
        if (idx_pos >= 0) {
            int col_pos = cmd_str.indexOf(":", idx_pos);
            if (col_pos >= 0) {
                int new_idx = cmd_str.substring(col_pos + 1).toInt();
                if (new_idx >= 0 && new_idx < SAMPLE_IMAGE_COUNT) {
                    g_use_custom_image = false;
                    g_current_image_idx = new_idx;
                    g_current_chunk_idx = 0;
                    g_last_send_time = 0; /* Phát ngay lập tức */
                    update_oled_display();
                    Serial.printf("{\"node\":\"TX\",\"event\":\"IMAGE_CHANGED\",\"img_idx\":%d}\n", g_current_image_idx);
                }
            }
        }
    } else if (cmd_str.indexOf("SET_ADAPTIVE") >= 0) {
        int pos = cmd_str.indexOf("\"val\"");
        if (pos >= 0) {
            int col_pos = cmd_str.indexOf(":", pos);
            if (col_pos >= 0) {
                int val = cmd_str.substring(col_pos + 1).toInt();
                g_auto_adaptive = (val != 0);
                Serial.printf("{\"node\":\"TX\",\"event\":\"ADAPTIVE_TOGGLE\",\"auto\":%s}\n", g_auto_adaptive ? "true" : "false");
            }
        }
    } else if (cmd_str.indexOf("SET_MODE") >= 0) {
        int mode_pos = cmd_str.indexOf("\"mode\"");
        if (mode_pos >= 0) {
            int col_pos = cmd_str.indexOf(":", mode_pos);
            if (col_pos >= 0) {
                int new_mode = cmd_str.substring(col_pos + 1).toInt();
                if (new_mode >= 0 && new_mode <= 2) {
                    g_tx_settings.chn_alg = (channel_alg_t)new_mode;
                    g_auto_adaptive = false; /* Chuyển thủ công thì tạm tắt auto để không bị feedback đè */
                    update_oled_display();
                    Serial.printf("{\"node\":\"TX\",\"event\":\"MODE_OVERRIDE\",\"chn_alg\":%d,\"auto\":false}\n", (int)g_tx_settings.chn_alg);
                }
            }
        }
    }
}

/* Gửi 1 mảnh của ảnh 64x64 qua Kênh */
static void transmit_image_chunk() {
    const uint8_t *full_img = get_active_image_ptr();
    size_t offset = g_current_chunk_idx * SAMPLE_IMAGE_CHUNK_SIZE;
    const uint8_t *chunk_data = full_img + offset;

    static uint8_t s_frame_super_seq = 0;
    /* Khóa chặt 3 bit thấp của seq luôn luôn là chunk_idx (0..7) để máy thu không bao giờ lệch vị trí */
    g_tx_settings.seq = (uint8_t)((s_frame_super_seq << 3) | (g_current_chunk_idx & 0x07));
    g_tx_settings.img_idx = g_use_custom_image ? g_custom_img_version : g_current_image_idx;
    uint8_t frame_buf[FRAME_MAX_TOTAL_LEN];
    tx_metrics_t metrics;

    size_t frame_len = build_tx_frame(
        chunk_data,
        SAMPLE_IMAGE_CHUNK_SIZE,
        &g_tx_settings,
        frame_buf,
        sizeof(frame_buf),
        &metrics
    );

    if (frame_len > 0) {
        /* Bắn khung qua UART1 sang bo Kênh */
        SerialChannel.write(frame_buf, frame_len);
        SerialChannel.flush();

        /* Xuất JSON Telemetry chuẩn hóa cho Python App */
        Serial.printf("{\"node\":\"TX\",\"seq\":%d,\"img_idx\":%d,\"is_custom\":%s,\"chunk\":%d,\"src_alg\":%d,\"chn_alg\":%d,\"raw_len\":%u,\"comp_len\":%u,\"frame_len\":%u,\"cr\":%.2f}\n",
                      g_tx_settings.seq,
                      g_tx_settings.img_idx,
                      g_use_custom_image ? "true" : "false",
                      g_current_chunk_idx + 1,
                      (int)g_tx_settings.src_alg, (int)g_tx_settings.chn_alg,
                      metrics.raw_len, metrics.compressed_len, frame_len, metrics.compression_ratio);

        if (g_current_chunk_idx == (SAMPLE_IMAGE_NUM_CHUNKS - 1)) {
            s_frame_super_seq++;
        }
        g_current_chunk_idx = (g_current_chunk_idx + 1) % SAMPLE_IMAGE_NUM_CHUNKS;
    }
}

/* Đọc và xử lý gói tin Feedback ngược từ Kênh (Rx -> Ch -> Tx) */
static void process_incoming_feedback() {
    static uint8_t fb_buf[FEEDBACK_FRAME_LEN];
    static size_t fb_idx = 0;

    while (SerialChannel.available() > 0) {
        uint8_t b = SerialChannel.read();

        if (fb_idx == 0) {
            if (b == FRAME_SOF_FEEDBACK) {
                fb_buf[fb_idx++] = b;
            }
        } else {
            fb_buf[fb_idx++] = b;
            if (fb_idx == FEEDBACK_FRAME_LEN) {
                if (fb_buf[FEEDBACK_FRAME_LEN - 1] == FRAME_EOF) {
                    feedback_frame_t *fb = (feedback_frame_t*)fb_buf;
                    uint16_t calc_crc = crc16_ccitt(&fb_buf[1], 5);
                    uint16_t pkt_crc = ((uint16_t)fb_buf[6] << 8) | fb_buf[7];

                    if (calc_crc == pkt_crc) {
                        /* TỰ THÍCH NGHI: Đổi Mode theo phản hồi từ Máy thu (chỉ khi bật Auto) */
                        channel_alg_t new_mode = (channel_alg_t)fb->recommended_mode;
                        if (g_auto_adaptive && (new_mode != g_tx_settings.chn_alg)) {
                            g_tx_settings.chn_alg = new_mode;
                            g_last_rec_mode = new_mode;
                            Serial.printf("{\"node\":\"TX\",\"event\":\"ADAPTIVE_SWITCH\",\"chn_alg\":%d,\"ber\":%.1f}\n",
                                          (int)new_mode, (float)fb->measured_ber_x10 / 10.0f);
                        }
                        g_last_rx_ber = (float)fb->measured_ber_x10 / 10.0f;
                    }
                }
                fb_idx = 0;
            }
        }
    }
}

void setup() {
    Serial.setRxBufferSize(2048);
    Serial.begin(115200);

    /* Khởi tạo nút nhấn đổi ảnh (Pull-up nội) */
    pinMode(PIN_BTN_IMAGE, INPUT_PULLUP);

    /* Khởi tạo màn hình OLED qua Software I2C */
    u8g2.begin();
    u8g2.setContrast(255);

    /* Khởi tạo 2 chân công tắc 3 nấc chuyển Baudrate */
    pinMode(PIN_SW_BAUD_1, INPUT_PULLUP);
    pinMode(PIN_SW_BAUD_2, INPUT_PULLUP);
    g_current_baudrate = read_switch_baudrate();

    /* Khởi tạo UART1 hai chiều nối với Kênh (RX: 20, TX: 21) theo Baudrate ban đầu */
    SerialChannel.begin(g_current_baudrate, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

    update_oled_display();
    Serial.println("{\"node\":\"TX\",\"status\":\"READY\"}");
}

void loop() {
    /* 0. Quét công tắc 3 nấc để đồng bộ Baudrate tức thời */
    check_and_update_baudrate();

    /* 1. Tiếp nhận lệnh điều khiển từ PC qua cổng USB (Non-blocking) */
    static String s_cmd_buf = "";
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            s_cmd_buf.trim();
            if (s_cmd_buf.length() > 0) {
                process_pc_command(s_cmd_buf);
            }
            s_cmd_buf = "";
        } else {
            if (s_cmd_buf.length() < 2048) {
                s_cmd_buf += c;
            }
        }
    }

    /* 2. Xử lý nút nhấn đổi ảnh mẫu trên mạch */
    if (digitalRead(PIN_BTN_IMAGE) == LOW) {
        if (millis() - g_last_btn_time > 300) {
            g_use_custom_image = false; /* Khi ấn nút vật lý, quay lại quay vòng các ảnh mẫu */
            g_current_image_idx = (g_current_image_idx + 1) % SAMPLE_IMAGE_COUNT;
            g_current_chunk_idx = 0;
            g_last_btn_time = millis();
            g_last_send_time = 0; /* Phát ngay lập tức */
            update_oled_display();
            Serial.printf("{\"node\":\"TX\",\"event\":\"BUTTON_IMAGE_CHANGED\",\"img_idx\":%d}\n", g_current_image_idx);
        }
    }

    /* 3. Tiếp nhận luồng Feedback thích nghi từ Máy thu */
    process_incoming_feedback();

    /* 4. Phát chu kỳ từng mảnh ảnh */
    if (millis() - g_last_send_time >= DEFAULT_TX_INTERVAL_MS) {
        g_last_send_time = millis();
        transmit_image_chunk();
        update_oled_display();
    }
}
