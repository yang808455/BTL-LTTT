#include "src_decoder_huffman.h"
#include "bit_unpacker.h"
#include <string.h>

#define HUFFMAN_EOF_SYMBOL 128

/* Bảng độ dài bit đồng bộ chuẩn với Máy phát */
static const uint8_t HUFFMAN_BIT_LEN[129] = {
    10, 10, 10, 10, 10, 10, 10, 10, 10,  7,  6, 10, 10,  6, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
     4,  8,  8,  8,  8,  8,  8,  8,  7,  7,  8,  7,  6,  6,  5,  7,
     6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
     6,  7,  8,  6,  8,  8,  8,
     7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
     8,  8,  8,  8,  7,  8,
     5,  7,  6,  5,  4,  6,  6,  5,  4,  9,  8,  5,  6,  5,  4,  6,
     9,  5,  5,  4,  5,  7,  6,  8,  6,  9,
     8,  8,  8,  8, 10,
     5
};

typedef struct {
    int16_t left;
    int16_t right;
    int16_t symbol; /* -1 nếu là nút trung gian, >= 0 nếu là nút lá */
} tree_node_t;

static tree_node_t s_tree[512];
static int16_t s_tree_node_count = 1;
static bool s_tree_initialized = false;

static void build_decode_tree(void) {
    if (s_tree_initialized) return;

    memset(s_tree, -1, sizeof(s_tree));
    s_tree_node_count = 1; /* Root là node 0 */
    s_tree[0].symbol = -1;

    /* Tính canonical codes */
    uint16_t bl_count[17] = {0};
    for (int i = 0; i < 129; i++) {
        uint8_t len = HUFFMAN_BIT_LEN[i];
        if (len <= 16) bl_count[len]++;
    }

    uint16_t next_code[17] = {0};
    uint16_t code = 0;
    for (int bits = 1; bits <= 16; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    /* Chèn từng ký tự vào cây */
    for (int i = 0; i < 129; i++) {
        uint8_t len = HUFFMAN_BIT_LEN[i];
        if (len == 0) continue;
        uint16_t c = next_code[len]++;

        int16_t curr = 0;
        for (int b = len - 1; b >= 0; b--) {
            uint8_t bit = (c >> b) & 1;
            if (bit == 0) {
                if (s_tree[curr].left == -1) {
                    s_tree[curr].left = s_tree_node_count++;
                    s_tree[s_tree[curr].left].symbol = -1;
                }
                curr = s_tree[curr].left;
            } else {
                if (s_tree[curr].right == -1) {
                    s_tree[curr].right = s_tree_node_count++;
                    s_tree[s_tree[curr].right].symbol = -1;
                }
                curr = s_tree[curr].right;
            }
        }
        s_tree[curr].symbol = i;
    }

    s_tree_initialized = true;
}

size_t huffman_decode(const uint8_t *comp_data, size_t comp_len,
                      uint8_t *out_text, size_t max_text_len) {
    if (!comp_data || comp_len == 0 || !out_text || max_text_len == 0) return 0;
    build_decode_tree();

    bit_unpacker_t bu;
    bit_unpacker_init(&bu, comp_data, comp_len);

    size_t out_idx = 0;
    int16_t curr = 0;

    while (bit_unpacker_has_more(&bu)) {
        uint8_t bit = 0;
        if (!bit_unpacker_read_bit(&bu, &bit)) break;

        curr = (bit == 0) ? s_tree[curr].left : s_tree[curr].right;
        if (curr == -1) {
            /* Lỗi do kênh truyền làm đảo bit phá vỡ cây Huffman -> Reset về root */
            curr = 0;
            continue;
        }

        if (s_tree[curr].symbol != -1) {
            int16_t sym = s_tree[curr].symbol;
            if (sym == HUFFMAN_EOF_SYMBOL) {
                break; /* Gặp ký hiệu kết thúc */
            }
            if (out_idx < max_text_len - 1) {
                out_text[out_idx++] = (uint8_t)sym;
            }
            curr = 0; /* Trở về gốc cây cho ký tự tiếp theo */
        }
    }

    out_text[out_idx] = '\0';
    return out_idx;
}
