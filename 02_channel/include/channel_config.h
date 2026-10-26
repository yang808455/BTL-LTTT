#ifndef CHANNEL_CONFIG_H
#define CHANNEL_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "../../shared/protocol_defs.h"

/* Cấu hình chân phần cứng trên ESP32-C3 SuperMini Kênh truyền */
#define PIN_OLED_SDA            8   /* I2C SDA màn hình OLED 2 */
#define PIN_OLED_SCL            9   /* I2C SCL màn hình OLED 2 */
#define PIN_POTENTIOMETER       3   /* Chân Analog ADC1 đọc điện áp biến trở 10k (GPIO 3 trên mạch thật) */

/* Cổng UART0: Giao tiếp với Máy phát (Tx) */
#define PIN_RX_FROM_TX          20  /* Nhận dữ liệu từ Tx (GPIO 21) */
#define PIN_TX_TO_TX            21  /* Chuyển tiếp Feedback về Tx (GPIO 20) */

/* Cổng UART1: Giao tiếp với Máy thu (Rx) */
#define PIN_TX_TO_RX            5   /* Phát dữ liệu đã tạo nhiễu sang Rx (GPIO 20) */
#define PIN_RX_FROM_RX          4   /* Nhận Feedback từ Rx (GPIO 21) */

#define CHANNEL_BAUDRATE        115200

typedef enum {
    NOISE_SCOPE_PAYLOAD_ONLY,
    NOISE_SCOPE_FULL_FRAME
} noise_scope_t;

typedef struct {
    float ber;              /* 0.0 -> 0.15 (0% đến 15%) */
    noise_scope_t scope;
    bool enabled;
} channel_settings_t;

#endif /* CHANNEL_CONFIG_H */
