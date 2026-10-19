#ifndef PROTOCOL_DEFS_H
#define PROTOCOL_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Các hằng số đóng khung UART */
#define FRAME_SOF               0x7E    /* Start of Frame: '~' */
#define FRAME_EOF               0x7F    /* End of Frame: DEL */
#define FRAME_MAX_PAYLOAD_LEN   256     /* Dung lượng tối đa của một khối payload */
#define FRAME_HEADER_LEN        5       /* SOF(1) + SEQ(1) + SRC_ALG(1) + CHN_ALG(1) + LEN(1) */
#define FRAME_OVERHEAD_LEN      8       /* HEADER(5) + CRC16(2) + EOF(1) */
#define FRAME_MAX_TOTAL_LEN     (FRAME_MAX_PAYLOAD_LEN + FRAME_OVERHEAD_LEN)

/* Định danh thuật toán mã hóa nguồn (Source Coding Algorithm) */
typedef enum {
    SRC_ALG_NONE        = 0x00, /* Không nén (Raw data) */
    SRC_ALG_HUFFMAN     = 0x01, /* Nén Huffman (Cây tiền tố tĩnh tối ưu) */
    SRC_ALG_ARITHMETIC  = 0x02, /* Nén Mã số học (Arithmetic Coding) */
    SRC_ALG_LZW         = 0x03  /* Nén Từ điển LZW */
} source_alg_t;

/* Định danh thuật toán mã hóa kênh hệ thống (Systematic Channel Coding) */
typedef enum {
    CHN_ALG_NONE        = 0x00, /* Không bảo vệ kênh */
    CHN_ALG_HAMMING_84  = 0x01, /* Systematic Extended Hamming (8,4) SEC-DED */
    CHN_ALG_CYCLIC_74   = 0x02  /* Systematic Cyclic (7,4) g(x) = x^3 + x + 1 */
} channel_alg_t;

/* Trạng thái giải mã kênh (Channel Decode Status) */
typedef enum {
    CHN_STATUS_OK           = 0, /* Không có lỗi bit */
    CHN_STATUS_CORRECTED    = 1, /* Phát hiện lỗi 1-bit và đã sửa thành công */
    CHN_STATUS_UNCORRECTABLE = 2, /* Phát hiện lỗi kép hoặc không sửa được */
    CHN_STATUS_ERROR        = 3  /* Lỗi định dạng/tham số */
} chn_decode_status_t;

/* Cấu trúc Frame Header */
#pragma pack(push, 1)
typedef struct {
    uint8_t sof;        /* 0x7E */
    uint8_t seq;        /* Số thứ tự gói tin (0 - 255) */
    uint8_t src_alg;    /* Mã thuật toán nguồn (source_alg_t) */
    uint8_t chn_alg;    /* Mã thuật toán kênh (channel_alg_t) */
    uint8_t payload_len;/* Số byte của payload thực tế */
} frame_header_t;
#pragma pack(pop)

/* Cấu trúc gói tin hoàn chỉnh */
#pragma pack(push, 1)
typedef struct {
    frame_header_t header;
    uint8_t payload[FRAME_MAX_PAYLOAD_LEN];
    uint16_t crc16;     /* CRC-16 CCITT tính trên header + payload */
    uint8_t eof;        /* 0x7F */
} frame_packet_t;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_DEFS_H */
