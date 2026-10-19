#include "bit_unpacker.h"

void bit_unpacker_init(bit_unpacker_t *bu, const uint8_t *buffer, size_t total_bytes) {
    if (!bu) return;
    bu->buffer = buffer;
    bu->total_bytes = total_bytes;
    bu->bit_pos = 0;
}

bool bit_unpacker_read_bit(bit_unpacker_t *bu, uint8_t *bit) {
    if (!bu || !bu->buffer || !bit) return false;
    size_t byte_idx = bu->bit_pos / 8;
    if (byte_idx >= bu->total_bytes) return false;

    uint8_t bit_offset = 7 - (bu->bit_pos % 8);
    *bit = (bu->buffer[byte_idx] >> bit_offset) & 1;
    bu->bit_pos++;
    return true;
}

bool bit_unpacker_read_bits(bit_unpacker_t *bu, uint8_t num_bits, uint32_t *value) {
    if (!bu || !value || num_bits > 32) return false;
    uint32_t val = 0;
    for (int i = 0; i < num_bits; i++) {
        uint8_t b = 0;
        if (!bit_unpacker_read_bit(bu, &b)) return false;
        val = (val << 1) | b;
    }
    *value = val;
    return true;
}

bool bit_unpacker_has_more(const bit_unpacker_t *bu) {
    if (!bu) return false;
    return (bu->bit_pos < bu->total_bytes * 8);
}
