#include "crc16.h"

uint16_t crc16_ccitt_update(uint16_t crc, uint8_t byte) {
    crc ^= ((uint16_t)byte << 8);
    for (int i = 0; i < 8; i++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    if (!data) return 0;
    for (size_t i = 0; i < length; i++) {
        crc = crc16_ccitt_update(crc, data[i]);
    }
    return crc;
}
