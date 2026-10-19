#ifndef BIT_UNPACKER_H
#define BIT_UNPACKER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t *buffer;
    size_t total_bytes;
    size_t bit_pos;
} bit_unpacker_t;

void bit_unpacker_init(bit_unpacker_t *bu, const uint8_t *buffer, size_t total_bytes);
bool bit_unpacker_read_bit(bit_unpacker_t *bu, uint8_t *bit);
bool bit_unpacker_read_bits(bit_unpacker_t *bu, uint8_t num_bits, uint32_t *value);
bool bit_unpacker_has_more(const bit_unpacker_t *bu);

#ifdef __cplusplus
}
#endif

#endif /* BIT_UNPACKER_H */
