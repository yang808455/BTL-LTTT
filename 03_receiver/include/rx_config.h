#ifndef RX_CONFIG_H
#define RX_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* Chân kết nối phần cứng ESP32-C3 SuperMini Máy thu */
#define PIN_RX_FROM_CHANNEL     20  /* Nhận luồng byte từ Node 2 (Kênh truyền) */
#define PIN_I2C_SDA             8   /* SDA màn hình OLED SSD1306 (Tùy chọn) */
#define PIN_I2C_SCL             9   /* SCL màn hình OLED SSD1306 (Tùy chọn) */
#define RX_BAUDRATE             115200

#endif /* RX_CONFIG_H */
