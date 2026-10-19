#include "src_decoder_lzw.h"
#include "bit_unpacker.h"
#include <string.h>

#define LZW_MAX_CODES 512
#define LZW_EOF_CODE  128

typedef struct {
    int16_t prefix;
    uint8_t character;
} lzw_entry_t;

static size_t reconstruct_string(const lzw_entry_t *dict, uint16_t code, uint8_t *buffer, size_t max_buf) {
    uint8_t temp[256];
    size_t count = 0;

    int16_t curr = code;
    while (curr >= 129 && count < sizeof(temp)) {
        temp[count++] = dict[curr].character;
        curr = dict[curr].prefix;
    }
    if (curr >= 0 && count < sizeof(temp)) {
        temp[count++] = (uint8_t)curr;
    }

    if (count > max_buf) count = max_buf;
    /* Đảo ngược chuỗi để có thứ tự đúng */
    for (size_t i = 0; i < count; i++) {
        buffer[i] = temp[count - 1 - i];
    }
    return count;
}

size_t lzw_decode(const uint8_t *comp_data, size_t comp_len,
                  uint8_t *out_text, size_t max_text_len) {
    if (!comp_data || comp_len == 0 || !out_text || max_text_len == 0) return 0;

    bit_unpacker_t bu;
    bit_unpacker_init(&bu, comp_data, comp_len);

    lzw_entry_t dict[LZW_MAX_CODES];
    memset(dict, 0, sizeof(dict));
    uint16_t next_code = 129;

    uint32_t first_code = 0;
    if (!bit_unpacker_read_bits(&bu, 9, &first_code) || first_code == LZW_EOF_CODE) {
        out_text[0] = '\0';
        return 0;
    }

    size_t out_idx = 0;
    out_text[out_idx++] = (uint8_t)first_code;
    uint16_t old_code = (uint16_t)first_code;

    while (bit_unpacker_has_more(&bu)) {
        uint32_t code = 0;
        if (!bit_unpacker_read_bits(&bu, 9, &code) || code == LZW_EOF_CODE) {
            break;
        }

        uint8_t str_buf[256];
        size_t str_len = 0;

        if (code < next_code) {
            str_len = reconstruct_string(dict, (uint16_t)code, str_buf, sizeof(str_buf));
        } else if (code == next_code) {
            /* Trường hợp đặc biệt cScSc */
            str_len = reconstruct_string(dict, old_code, str_buf, sizeof(str_buf));
            if (str_len < sizeof(str_buf)) {
                str_buf[str_len] = str_buf[0];
                str_len++;
            }
        } else {
            break; /* Mã không hợp lệ do nhiễu */
        }

        /* Ghi vào out_text */
        for (size_t i = 0; i < str_len; i++) {
            if (out_idx < max_text_len - 1) {
                out_text[out_idx++] = str_buf[i];
            }
        }

        /* Thêm vào từ điển */
        if (next_code < LZW_MAX_CODES) {
            dict[next_code].prefix = old_code;
            dict[next_code].character = str_buf[0];
            next_code++;
        }
        old_code = (uint16_t)code;
    }

    out_text[out_idx] = '\0';
    return out_idx;
}
