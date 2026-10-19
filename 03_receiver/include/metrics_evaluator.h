#ifndef METRICS_EVALUATOR_H
#define METRICS_EVALUATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_frames_received;
    uint32_t total_crc_valid_frames;
    uint32_t total_single_errors_corrected;
    uint32_t total_double_errors_detected;
    uint32_t total_raw_bytes_recovered;
    uint32_t total_compressed_bytes_received;
    float fer;                      /* Frame Error Rate (%) */
    float correction_success_rate;  /* Tỷ lệ sửa lỗi thành công (%) */
    float avg_compression_ratio;    /* Tỷ số nén trung bình */
} global_metrics_t;

void metrics_init(void);
void metrics_update_frame(bool crc_valid, size_t single_err, size_t double_err,
                          size_t raw_bytes, size_t comp_bytes);
void metrics_get(global_metrics_t *out_metrics);
void metrics_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* METRICS_EVALUATOR_H */
