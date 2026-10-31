#ifndef RX_CONFIG_H
#define RX_CONFIG_H

#include <stdint.h>
#include "../../shared/protocol_defs.h"

/* Cấu hình chân phần cứng trên ESP32-C3 SuperMini Máy thu */
#define PIN_OLED_SDA            8   /* I2C SDA màn hình OLED 3 */
#define PIN_OLED_SCL            9   /* I2C SCL màn hình OLED 3 */

#define PIN_UART_RX             21  /* Nhận dữ liệu nhiễu từ Kênh (Chân 5 của Kênh nối vào 21 của Thu) */
#define PIN_UART_TX             20  /* Gửi Feedback sang Kênh (Chân 20 của Thu nối vào 4 của Kênh) */

/* 3 Chân đèn LED RGB trên bo Máy thu (Cực âm chung GND, Cực dương qua trở 220Ω vào GPIO) */
#define PIN_RGB_R               5   /* Kênh Đỏ (Red - GPIO 5) */
#define PIN_RGB_G               6   /* Kênh Xanh lá (Green - GPIO 6) */
#define PIN_RGB_B               7   /* Kênh Xanh dương (Blue - GPIO 7) */

#define PIN_LED_GREEN           PIN_RGB_G  /* Mode Direct: LED Xanh lá */
#define PIN_LED_YELLOW          PIN_RGB_B  /* Mode Hamming: LED Xanh dương */
#define PIN_LED_RED             PIN_RGB_R  /* Mode Cyclic: LED Đỏ */

#define RX_UART_BAUDRATE        115200

/* Cấu hình công tắc gạt 3 nấc chuyển Baudrate tức thì */
#define PIN_SW_BAUD_1           2   /* Bìa trái (GND -> 9600 bps) */
#define PIN_SW_BAUD_2           3   /* Bìa phải (GND -> 115200 bps) | Ở giữa -> 38400 bps */

#endif /* RX_CONFIG_H */
