#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tính giá trị CRC16-CCITT (Poly: 0x1021, Init: 0xFFFF)
 * @param data Con trỏ vùng đệm dữ liệu
 * @param length Độ dài chuỗi byte
 * @return uint16_t Giá trị CRC16 đã tính
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t length);

/**
 * @brief Cập nhật CRC16-CCITT theo từng byte
 */
uint16_t crc16_ccitt_update(uint16_t crc, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H */
