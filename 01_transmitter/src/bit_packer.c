#include "bit_packer.h"
#include <string.h>

void bit_packer_init(bit_packer_t *bp, uint8_t *buffer, size_t capacity_bytes) {
    if (!bp || !buffer) return;
    bp->buffer = buffer;
    bp->capacity_bytes = capacity_bytes;
    bp->bit_count = 0;
    bp->current_byte = 0;
    bp->bits_in_byte = 0;
    memset(buffer, 0, capacity_bytes);
}

bool bit_packer_write_bit(bit_packer_t *bp, uint8_t bit) {
    if (!bp || !bp->buffer) return false;
    
    bp->current_byte = (bp->current_byte << 1) | (bit & 1);
    bp->bits_in_byte++;
    bp->bit_count++;
    
    if (bp->bits_in_byte == 8) {
        size_t byte_idx = (bp->bit_count - 1) / 8;
        if (byte_idx >= bp->capacity_bytes) return false;
        bp->buffer[byte_idx] = bp->current_byte;
        bp->current_byte = 0;
        bp->bits_in_byte = 0;
    }
    return true;
}

bool bit_packer_write_bits(bit_packer_t *bp, uint32_t value, uint8_t num_bits) {
    if (!bp || num_bits > 32) return false;
    for (int i = num_bits - 1; i >= 0; i--) {
        uint8_t b = (value >> i) & 1;
        if (!bit_packer_write_bit(bp, b)) return false;
    }
    return true;
}

size_t bit_packer_flush(bit_packer_t *bp) {
    if (!bp || !bp->buffer) return 0;
    if (bp->bits_in_byte > 0) {
        /* Dịch trái các bit còn thiếu để căn lề MSB */
        bp->current_byte <<= (8 - bp->bits_in_byte);
        size_t byte_idx = bp->bit_count / 8;
        if (byte_idx < bp->capacity_bytes) {
            bp->buffer[byte_idx] = bp->current_byte;
        }
        bp->current_byte = 0;
        bp->bits_in_byte = 0;
    }
    return (bp->bit_count + 7) / 8;
}

size_t bit_packer_get_total_bits(const bit_packer_t *bp) {
    return bp ? bp->bit_count : 0;
}
