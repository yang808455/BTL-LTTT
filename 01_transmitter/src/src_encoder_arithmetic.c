#include "src_encoder_arithmetic.h"
#include "bit_packer.h"
#include <string.h>

#define ARITH_NUM_SYMBOLS 129
#define ARITH_EOF_SYMBOL  128

/* Bảng trọng số tần suất tích lũy cho 129 symbols (0..128) */
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

static void emit_bit_plus_follow(bit_packer_t *bp, uint8_t bit, uint32_t *follow_bits) {
    bit_packer_write_bit(bp, bit);
    while (*follow_bits > 0) {
        bit_packer_write_bit(bp, !bit);
        (*follow_bits)--;
    }
}

size_t arithmetic_encode(const uint8_t *input, size_t input_len,
                         uint8_t *out_bytes, size_t max_out_bytes,
                         size_t *out_total_bits) {
    if (!input || !out_bytes || max_out_bytes == 0) return 0;
    init_arith_freq();

    bit_packer_t packer;
    bit_packer_init(&packer, out_bytes, max_out_bytes);

    uint32_t low = 0x00000000;
    uint32_t high = 0xFFFFFFFF;
    uint32_t follow_bits = 0;
    uint32_t total_freq = s_cum_freq[ARITH_NUM_SYMBOLS];

    /* Mã hóa từng ký tự + symbol EOF */
    for (size_t idx = 0; idx <= input_len; idx++) {
        uint16_t sym = (idx < input_len) ? input[idx] : ARITH_EOF_SYMBOL;
        if (sym > 127 && sym != ARITH_EOF_SYMBOL) sym = '?';

        uint64_t range = (uint64_t)(high - low) + 1;
        high = low + (uint32_t)((range * s_cum_freq[sym + 1]) / total_freq) - 1;
        low  = low + (uint32_t)((range * s_cum_freq[sym]) / total_freq);

        /* Chuẩn hóa dải giá trị (Renormalization) */
        for (;;) {
            if ((high & 0x80000000) == (low & 0x80000000)) {
                emit_bit_plus_follow(&packer, (high >> 31) & 1, &follow_bits);
                low = (low << 1);
                high = (high << 1) | 1;
            } else if ((low & 0x40000000) && !(high & 0x40000000)) {
                follow_bits++;
                low = (low & 0x3FFFFFFF) << 1;
                high = ((high & 0x3FFFFFFF) << 1) | 0x80000001;
            } else {
                break;
            }
        }
    }

    /* Đẩy các bit còn lại kết thúc dải */
    follow_bits++;
    if (low < 0x40000000) {
        emit_bit_plus_follow(&packer, 0, &follow_bits);
    } else {
        emit_bit_plus_follow(&packer, 1, &follow_bits);
    }

    size_t total_bits = bit_packer_get_total_bits(&packer);
    if (out_total_bits) {
        *out_total_bits = total_bits;
    }

    return bit_packer_flush(&packer);
}
