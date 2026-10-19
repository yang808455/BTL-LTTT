#ifndef CHANNEL_CONFIG_H
#define CHANNEL_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* Chân kết nối phần cứng ESP32-C3 SuperMini Kênh truyền */
#define PIN_RX_FROM_TX      20  /* Nhận luồng byte từ Node 1 (Tx) */
#define PIN_TX_TO_RX        21  /* Phát luồng byte đã gây nhiễu sang Node 3 (Rx) */
#define CHANNEL_BAUDRATE    115200

typedef enum {
    NOISE_SCOPE_PAYLOAD_ONLY = 0, /* Chỉ gây lỗi vào trường dữ liệu (giữ nguyên Header để Rx không mất đồng bộ khung) */
    NOISE_SCOPE_FULL_FRAME   = 1  /* Gây lỗi trên toàn bộ khung (bao gồm cả SOF, Length, CRC) */
} noise_scope_t;

typedef struct {
    float ber;                  /* Tỷ lệ lỗi bit mong muốn (0.000 -> 0.200) */
    noise_scope_t scope;        /* Phạm vi gây nhiễu */
    bool enabled;               /* Bật / Tắt bộ gây nhiễu */
} channel_settings_t;

#endif /* CHANNEL_CONFIG_H */
