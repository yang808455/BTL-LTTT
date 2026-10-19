#include "src_decoder_arithmetic.h"
#include "bit_unpacker.h"
#include <string.h>

#define ARITH_NUM_SYMBOLS 129
#define ARITH_EOF_SYMBOL  128

static uint32_t s_cum_freq[ARITH_NUM_SYMBOLS + 1];
static bool s_freq_initialized = false;

static void init_arith_freq(void) {
    if (s_freq_initialized) return;
    s_cum_freq[0] = 0;
    for (int i = 0; i < ARITH_NUM_SYMBOLS; i++) {
        uint32_t weight = 1;
        if (i == ' ') weight = 10;
        else if (i == 'e' || i == 't' || i == 'a' || i == 'o' || i == 'i') weight = 8;
        else if (i >= '0' && i <= '9') weight = 5;
        else if (i >= 'a' && i <= 'z') weight = 4;
        else if (i >= 'A' && i <= 'Z') weight = 3;
        else if (i == ARITH_EOF_SYMBOL) weight = 2;
        s_cum_freq[i + 1] = s_cum_freq[i] + weight;
    }
    s_freq_initialized = true;
}

static uint8_t read_next_bit(bit_unpacker_t *bu) {
    uint8_t bit = 0;
    if (!bit_unpacker_read_bit(bu, &bit)) return 0;
    return bit;
}

size_t arithmetic_decode(const uint8_t *comp_data, size_t comp_len,
                         uint8_t *out_text, size_t max_text_len) {
    if (!comp_data || comp_len == 0 || !out_text || max_text_len == 0) return 0;
    init_arith_freq();

    bit_unpacker_t bu;
    bit_unpacker_init(&bu, comp_data, comp_len);

    uint32_t low = 0x00000000;
    uint32_t high = 0xFFFFFFFF;
    uint32_t value = 0;
    uint32_t total_freq = s_cum_freq[ARITH_NUM_SYMBOLS];

    /* Nạp 32 bit đầu tiên vào value */
    for (int i = 0; i < 32; i++) {
        value = (value << 1) | read_next_bit(&bu);
    }

    size_t out_idx = 0;

    for (;;) {
        uint64_t range = (uint64_t)(high - low) + 1;
        uint32_t cum = (uint32_t)((((uint64_t)(value - low) + 1) * total_freq - 1) / range);

        /* Tìm symbol */
        int sym = 0;
        while (s_cum_freq[sym + 1] <= cum && sym < ARITH_NUM_SYMBOLS) {
            sym++;
        }

        if (sym == ARITH_EOF_SYMBOL || out_idx >= max_text_len - 1) {
            break;
        }

        out_text[out_idx++] = (uint8_t)sym;

        high = low + (uint32_t)((range * s_cum_freq[sym + 1]) / total_freq) - 1;
        low  = low + (uint32_t)((range * s_cum_freq[sym]) / total_freq);

        for (;;) {
            if ((high & 0x80000000) == (low & 0x80000000)) {
                low = low << 1;
                high = (high << 1) | 1;
                value = (value << 1) | read_next_bit(&bu);
            } else if ((low & 0x40000000) && !(high & 0x40000000)) {
                low = (low & 0x3FFFFFFF) << 1;
                high = ((high & 0x3FFFFFFF) << 1) | 0x80000001;
                value = ((value & 0x3FFFFFFF) << 1) | read_next_bit(&bu);
            } else {
                break;
            }
        }
    }

    out_text[out_idx] = '\0';
    return out_idx;
}
