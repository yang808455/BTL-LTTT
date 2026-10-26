#ifndef PROTOCOL_DEFS_H
#define PROTOCOL_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. CÁC HẰNG SỐ ĐÓNG KHUNG UART
 * ========================================================================= */
#define FRAME_SOF               0x7E    /* Start of Frame Data: '~' */
#define FRAME_SOF_DATA          0x7E    /* Đồng nghĩa FRAME_SOF */
#define FRAME_SOF_FEEDBACK      0x7C    /* Start of Frame Feedback: '|' */
#define FRAME_EOF               0x7F    /* End of Frame: DEL */

#define FRAME_MAX_PAYLOAD_LEN   256     /* Dung lượng tối đa của một khối payload */
#define FRAME_HEADER_LEN        5       /* SOF(1) + SEQ(1) + SRC_ALG(1) + CHN_ALG(1) + LEN(1) */
#define FRAME_OVERHEAD_LEN      8       /* HEADER(5) + CRC16(2) + EOF(1) */
#define FRAME_MAX_TOTAL_LEN     (FRAME_MAX_PAYLOAD_LEN + FRAME_OVERHEAD_LEN)

/* =========================================================================
 * 2. THÔNG SỐ TRUYỀN ẢNH BITMAP 64x64
 * ========================================================================= */
#define IMAGE_WIDTH             64
#define IMAGE_HEIGHT            64
#define IMAGE_SIZE_BYTES        512     /* 64 * 64 / 8 = 512 bytes */
#define IMAGE_NUM_CHUNKS        4       /* Chia ảnh thành 4 mảnh */
#define IMAGE_CHUNK_SIZE        128     /* 512 / 4 = 128 bytes mỗi mảnh (16 dòng ảnh) */

/* =========================================================================
 * 3. ĐỊNH DANH THUẬT TOÁN
 * ========================================================================= */
/* Thuật toán mã hóa nguồn (Source Coding Algorithm) */
typedef enum {
    SRC_ALG_NONE        = 0x00, /* Không nén (Raw data) */
    SRC_ALG_HUFFMAN     = 0x01, /* Nén Huffman (Cây tiền tố tĩnh tối ưu) */
    SRC_ALG_ARITHMETIC  = 0x02, /* Nén Mã số học (Arithmetic Coding) */
    SRC_ALG_LZW         = 0x03  /* Nén Từ điển LZW */
} source_alg_t;

/* Thuật toán mã hóa kênh hệ thống (Systematic Channel Coding) */
typedef enum {
    CHN_ALG_NONE        = 0x00, /* Direct (Không bảo vệ kênh / Kênh tốt) */
    CHN_ALG_HAMMING_84  = 0x01, /* Systematic Extended Hamming (8,4) SEC-DED / Hamming (7,4) */
    CHN_ALG_CYCLIC_74   = 0x02  /* Systematic Cyclic (7,4) g(x) = x^3 + x + 1 */
} channel_alg_t;

/* Trạng thái giải mã kênh (Channel Decode Status) */
typedef enum {
    CHN_STATUS_OK           = 0, /* Không có lỗi bit */
    CHN_STATUS_CORRECTED    = 1, /* Phát hiện lỗi 1-bit và đã sửa thành công */
    CHN_STATUS_UNCORRECTABLE = 2, /* Phát hiện lỗi kép hoặc không sửa được */
    CHN_STATUS_ERROR        = 3  /* Lỗi định dạng/tham số */
} chn_decode_status_t;

/* =========================================================================
 * 4. CẤU TRÚC GÓI TIN DỮ LIỆU XUÔI (TX -> CH -> RX)
 * ========================================================================= */
#pragma pack(push, 1)
typedef struct {
    uint8_t sof;        /* 0x7E (FRAME_SOF_DATA) */
    uint8_t seq;        /* Số thứ tự gói tin (0 - 255) */
    uint8_t src_alg;    /* Mã thuật toán nguồn (source_alg_t) */
    uint8_t chn_alg;    /* Mã thuật toán kênh (channel_alg_t) */
    uint8_t payload_len;/* Số byte của payload thực tế */
} frame_header_t;

typedef struct {
    frame_header_t header;
    uint8_t payload[FRAME_MAX_PAYLOAD_LEN];
    uint16_t crc16;     /* CRC-16 CCITT tính trên header + payload */
    uint8_t eof;        /* 0x7F (FRAME_EOF) */
} frame_packet_t;
#pragma pack(pop)

/* =========================================================================
 * 5. CẤU TRÚC GÓI TIN PHẢN HỒI THÍCH NGHI (RX -> CH -> TX)
 * ========================================================================= */
#define FEEDBACK_FRAME_LEN      9       /* Cố định 9 bytes */

#pragma pack(push, 1)
typedef struct {
    uint8_t sof;                /* 0x7C (FRAME_SOF_FEEDBACK) */
    uint8_t seq;                /* Số thứ tự gói phản hồi */
    uint8_t measured_ber_x10;   /* Tỷ lệ lỗi bit đo được x 10 (Ví dụ: 15 là 1.5%) */
    uint16_t corrected_bits;    /* Tổng số bit đã sửa thành công */
    uint8_t recommended_mode;   /* channel_alg_t: CHN_ALG_NONE, CHN_ALG_HAMMING_84, CHN_ALG_CYCLIC_74 */
    uint16_t crc16;             /* CRC-16 CCITT tính trên các byte từ seq đến recommended_mode */
    uint8_t eof;                /* 0x7F (FRAME_EOF) */
} feedback_frame_t;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_DEFS_H */
