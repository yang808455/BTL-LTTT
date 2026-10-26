#ifndef TX_CONFIG_H
#define TX_CONFIG_H

#include <stdint.h>
#include "../../shared/protocol_defs.h"

/* Cấu hình chân phần cứng trên ESP32-C3 SuperMini Máy phát */
#define PIN_OLED_SDA            8   /* I2C SDA màn hình OLED 1 */
#define PIN_OLED_SCL            9   /* I2C SCL màn hình OLED 1 */
#define PIN_UART_TX             21  /* Phát khung dữ liệu sang Kênh (GPIO 20) */
#define PIN_UART_RX             20  /* Nhận phản hồi Feedback từ Kênh (GPIO 21) */
#define PIN_BTN_IMAGE           10  /* Nút nhấn chuyển đổi ảnh mẫu (kéo xuống GND) */

#define TX_UART_BAUDRATE        115200
#define DEFAULT_TX_INTERVAL_MS  150 /* Thời gian giãn cách giữa các mảnh ảnh (150ms x 8 = 1.2s/ảnh) */

#endif /* TX_CONFIG_H */
