#ifndef SRC_DECODER_LZW_H
#define SRC_DECODER_LZW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t lzw_decode(const uint8_t *comp_data, size_t comp_len,
                  uint8_t *out_text, size_t max_text_len);

#ifdef __cplusplus
}
#endif

#endif /* SRC_DECODER_LZW_H */
