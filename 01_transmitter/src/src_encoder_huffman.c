#include "src_encoder_huffman.h"
#include "bit_packer.h"
#include <string.h>

/* Bảng độ dài bit Huffman chuẩn hóa (Canonical Bit Lengths) cho 129 symbols (0..127 ASCII + 128 EOF)
   Ký tự phổ biến (khoảng trắng, 'e', 't', 'a', 'o', 'i', số, v.v.) có bit ngắn hơn */
static const uint8_t HUFFMAN_BIT_LEN[129] = {
    /* 0..31: Ký tự điều khiển (ít dùng -> 10 bit) */
    10, 10, 10, 10, 10, 10, 10, 10, 10,  7 /*\t*/,  6 /*\n*/, 10, 10,  6 /*\r*/, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    /* 32..47: Space, Punctuation */
     4 /*' '*/,  8 /*!*/,  8 /*"*/,  8 /*#*/,  8 /*$*/,  8 /*%*/,  8 /*&*/,  8 /*'*/,
     7 /*(*/,  7 /*)*/,  8 /***/,  7 /*+*/,  6 /*,*/,  6 /*-*/,  5 /*.*/,  7 /*/*/,
    /* 48..57: '0'..'9' (thường dùng trong sensor -> 6 bit) */
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    /* 58..64: Punctuation */
     6 /*:*/,  7 /*;*/,  8 /*<*/,  6 /*=*/,  8 /*>*/,  8 /*?*/,  8 /*@*/,
    /* 65..90: 'A'..'Z' (7 bit) */
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    /* 91..96: Punctuation */
     8 /*[*/,  8 /*\*/,  8 /*]*/,  8 /*^*/,  7 /*_*/,  8 /*`*/,
    /* 97..122: 'a'..'z' (Ký tự thường: e,t,a,o,i,n,s,r... rất ngắn: 4-6 bit) */
     5 /*a*/,  7 /*b*/,  6 /*c*/,  5 /*d*/,  4 /*e*/,  6 /*f*/,  6 /*g*/,  5 /*h*/,
     4 /*i*/,  9 /*j*/,  8 /*k*/,  5 /*l*/,  6 /*m*/,  5 /*n*/,  4 /*o*/,  6 /*p*/,
     9 /*q*/,  5 /*r*/,  5 /*s*/,  4 /*t*/,  5 /*u*/,  7 /*v*/,  6 /*w*/,  8 /*x*/,
     6 /*y*/,  9 /*z*/,
    /* 123..127: { | } ~ DEL */
     8, 8, 8, 8, 10,
    /* 128: HUFFMAN_EOF_SYMBOL */
     5
};

static huffman_code_t s_codebook[129];
static bool s_codebook_initialized = false;

/* Hàm khởi tạo bảng mã Canonical Huffman từ mảng độ dài bit */
static void init_canonical_codebook(void) {
    if (s_codebook_initialized) return;

    /* Đếm số symbol theo từng độ dài bit (tối đa 16 bit) */
    uint16_t bl_count[17] = {0};
    for (int i = 0; i < 129; i++) {
        uint8_t len = HUFFMAN_BIT_LEN[i];
        if (len <= 16) {
            bl_count[len]++;
        }
    }

    /* Tìm mã nhị phân nhỏ nhất cho từng độ dài bit */
    uint16_t next_code[17] = {0};
    uint16_t code = 0;
    for (int bits = 1; bits <= 16; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    /* Gán mã cho từng symbol */
    for (int i = 0; i < 129; i++) {
        uint8_t len = HUFFMAN_BIT_LEN[i];
        if (len > 0) {
            s_codebook[i].code = next_code[len]++;
            s_codebook[i].bit_len = len;
        } else {
            s_codebook[i].code = 0;
            s_codebook[i].bit_len = 0;
        }
    }

    s_codebook_initialized = true;
}

bool huffman_get_code(uint16_t symbol, huffman_code_t *entry) {
    if (symbol > 128 || !entry) return false;
    init_canonical_codebook();
    *entry = s_codebook[symbol];
    return (entry->bit_len > 0);
}

size_t huffman_encode(const uint8_t *input, size_t input_len,
                      uint8_t *out_bits, size_t max_out_bytes,
                      size_t *out_total_bits) {
    if (!input || !out_bits || max_out_bytes == 0) return 0;
    init_canonical_codebook();

    bit_packer_t packer;
    bit_packer_init(&packer, out_bits, max_out_bytes);

    /* Mã hóa từng ký tự của chuỗi */
    for (size_t i = 0; i < input_len; i++) {
        uint8_t sym = input[i];
        if (sym > 127) sym = '?'; /* Ký tự ngoài chuẩn ASCII thay bằng '?' */
        huffman_code_t c = s_codebook[sym];
        if (!bit_packer_write_bits(&packer, c.code, c.bit_len)) {
            break; /* Buffer đầy */
        }
    }

    /* Ghi ký hiệu kết thúc luồng EOF */
    huffman_code_t eof_code = s_codebook[HUFFMAN_EOF_SYMBOL];
    bit_packer_write_bits(&packer, eof_code.code, eof_code.bit_len);

    size_t total_bits = bit_packer_get_total_bits(&packer);
    if (out_total_bits) {
        *out_total_bits = total_bits;
    }

    return bit_packer_flush(&packer);
}
