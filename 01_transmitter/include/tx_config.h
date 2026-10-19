#ifndef TX_CONFIG_H
#define TX_CONFIG_H

/* Cấu hình chân phần cứng ESP32-C3 SuperMini */
#define PIN_UART1_TX        21  /* Chân TX1 gửi sang ESP32 Kênh truyền */
#define PIN_UART1_RX        20  /* Chân RX1 (dự phòng) */
#define UART_BAUDRATE       115200

/* Cấu hình mặc định */
#define DEFAULT_TX_INTERVAL_MS 1500
#define DEFAULT_PAYLOAD_TEXT   "ANTIGRAVITY: Temp=28.4C Hum=65% Press=1013hPa"

#endif /* TX_CONFIG_H */
