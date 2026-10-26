#ifndef RX_CONFIG_H
#define RX_CONFIG_H

#include <stdint.h>
#include "../../shared/protocol_defs.h"

/* Cấu hình chân phần cứng trên ESP32-C3 SuperMini Máy thu */
#define PIN_OLED_SDA            8   /* I2C SDA màn hình OLED 3 */
#define PIN_OLED_SCL            9   /* I2C SCL màn hình OLED 3 */

#define PIN_UART_RX             21  /* Nhận dữ liệu nhiễu từ Kênh (Chân 5 của Kênh nối vào 21 của Thu) */
#define PIN_UART_TX             20  /* Gửi Feedback sang Kênh (Chân 20 của Thu nối vào 4 của Kênh) */

/* 3 Đèn LED chỉ thị Mode mã hóa tự thích nghi */
#define PIN_LED_GREEN           6   /* Mode Direct (Kênh tốt, BER < 1%) */
#define PIN_LED_YELLOW          7   /* Mode Hamming (Nhiễu vừa, 1% <= BER <= 4%) */
#define PIN_LED_RED             10  /* Mode Cyclic (Nhiễu nặng, BER > 4%) */

#define RX_UART_BAUDRATE        115200

#endif /* RX_CONFIG_H */
