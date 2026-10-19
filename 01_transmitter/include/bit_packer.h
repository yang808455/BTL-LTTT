#ifndef BIT_PACKER_H
#define BIT_PACKER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buffer;
    size_t capacity_bytes;
    size_t bit_count;       /* Tổng số bit đã ghi */
    uint8_t current_byte;   /* Byte đang tích lũy */
    uint8_t bits_in_byte;   /* Số bit đang có trong byte hiện tại (0-7) */
} bit_packer_t;

void bit_packer_init(bit_packer_t *bp, uint8_t *buffer, size_t capacity_bytes);
bool bit_packer_write_bit(bit_packer_t *bp, uint8_t bit);
bool bit_packer_write_bits(bit_packer_t *bp, uint32_t value, uint8_t num_bits);
size_t bit_packer_flush(bit_packer_t *bp); /* Đẩy nốt các bit còn dư ra buffer, trả về tổng số byte */
size_t bit_packer_get_total_bits(const bit_packer_t *bp);

#ifdef __cplusplus
}
#endif

#endif /* BIT_PACKER_H */
