#include "src_encoder_lzw.h"
#include "bit_packer.h"
#include <string.h>

#define HASH_SIZE 1024

typedef struct {
    int16_t prefix;
    uint8_t character;
    uint16_t code;
    bool used;
} lzw_dict_entry_t;

static uint32_t lzw_hash(int16_t prefix, uint8_t c) {
    return ((uint32_t)(prefix << 5) ^ c) & (HASH_SIZE - 1);
}

size_t lzw_encode(const uint8_t *input, size_t input_len,
                  uint8_t *out_bytes, size_t max_out_bytes,
                  size_t *out_total_bits) {
    if (!input || input_len == 0 || !out_bytes || max_out_bytes == 0) return 0;

    bit_packer_t packer;
    bit_packer_init(&packer, out_bytes, max_out_bytes);

    /* Bảng băm từ điển */
    lzw_dict_entry_t dict[HASH_SIZE];
    memset(dict, 0, sizeof(dict));

    uint16_t next_code = 129; /* 0..127 là ASCII, 128 là EOF */
    int16_t prefix = input[0];

    for (size_t i = 1; i < input_len; i++) {
        uint8_t c = input[i];

        /* Tìm (prefix, c) trong từ điển */
        uint32_t h = lzw_hash(prefix, c);
        int found_code = -1;
        while (dict[h].used) {
            if (dict[h].prefix == prefix && dict[h].character == c) {
                found_code = dict[h].code;
                break;
            }
            h = (h + 1) & (HASH_SIZE - 1);
        }

        if (found_code != -1) {
            prefix = found_code;
        } else {
            /* Xuất mã prefix ra luồng bit (9 bit mỗi token) */
            bit_packer_write_bits(&packer, (uint32_t)prefix, 9);

            /* Thêm vào từ điển nếu chưa đầy 512 mã */
            if (next_code < LZW_DICT_MAX_CODES) {
                dict[h].prefix = prefix;
                dict[h].character = c;
                dict[h].code = next_code++;
                dict[h].used = true;
            }
            prefix = c;
        }
    }

    /* Xuất mã ký tự cuối cùng */
    bit_packer_write_bits(&packer, (uint32_t)prefix, 9);
    /* Xuất mã EOF */
    bit_packer_write_bits(&packer, LZW_EOF_CODE, 9);

    size_t total_bits = bit_packer_get_total_bits(&packer);
    if (out_total_bits) {
        *out_total_bits = total_bits;
    }

    return bit_packer_flush(&packer);
}
