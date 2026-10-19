#include "noise_generator.h"
#include <stdlib.h>

static noise_stats_t s_stats = {0, 0, 0.0f};
static uint32_t s_prng_state = 0x12345678;

/* PRNG xorshift32 siêu nhanh, độc lập phần cứng */
static uint32_t xorshift32(void) {
    uint32_t x = s_prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_prng_state = x;
    return x;
}

static float random_float_01(void) {
    return (float)(xorshift32() & 0xFFFFFF) / 16777216.0f;
}

void noise_gen_init(void) {
    s_prng_state = 0x9ABCDEF1;
    noise_gen_reset_stats();
}

void noise_gen_apply_bsc(uint8_t *buffer, size_t start_idx, size_t count,
                         float ber, size_t *out_flipped_count) {
    if (!buffer || ber <= 0.0f) {
        if (out_flipped_count) *out_flipped_count = 0;
        return;
    }

    size_t flipped = 0;
    size_t total_bits = count * 8;

    for (size_t i = 0; i < count; i++) {
        size_t byte_idx = start_idx + i;
        for (int b = 0; b < 8; b++) {
            float r = random_float_01();
            if (r < ber) {
                buffer[byte_idx] ^= (1 << b); /* Đảo bit */
                flipped++;
            }
        }
    }

    s_stats.total_bits_processed += total_bits;
    s_stats.total_bits_flipped += flipped;
    if (s_stats.total_bits_processed > 0) {
        s_stats.cumulative_ber = (float)s_stats.total_bits_flipped / (float)s_stats.total_bits_processed;
    }

    if (out_flipped_count) {
        *out_flipped_count = flipped;
    }
}

bool noise_gen_manual_flip(uint8_t *buffer, size_t buffer_len, size_t byte_idx, uint8_t bit_idx) {
    if (!buffer || byte_idx >= buffer_len || bit_idx > 7) return false;
    buffer[byte_idx] ^= (1 << bit_idx);
    s_stats.total_bits_flipped++;
    return true;
}

void noise_gen_get_stats(noise_stats_t *stats) {
    if (stats) *stats = s_stats;
}

void noise_gen_reset_stats(void) {
    s_stats.total_bits_processed = 0;
    s_stats.total_bits_flipped = 0;
    s_stats.cumulative_ber = 0.0f;
}
