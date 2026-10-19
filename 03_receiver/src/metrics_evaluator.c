#include "metrics_evaluator.h"

static global_metrics_t s_metrics;

void metrics_init(void) {
    metrics_reset();
}

void metrics_reset(void) {
    s_metrics.total_frames_received = 0;
    s_metrics.total_crc_valid_frames = 0;
    s_metrics.total_single_errors_corrected = 0;
    s_metrics.total_double_errors_detected = 0;
    s_metrics.total_raw_bytes_recovered = 0;
    s_metrics.total_compressed_bytes_received = 0;
    s_metrics.fer = 0.0f;
    s_metrics.correction_success_rate = 100.0f;
    s_metrics.avg_compression_ratio = 1.0f;
}

void metrics_update_frame(bool crc_valid, size_t single_err, size_t double_err,
                          size_t raw_bytes, size_t comp_bytes) {
    s_metrics.total_frames_received++;
    if (crc_valid) {
        s_metrics.total_crc_valid_frames++;
    }
    s_metrics.total_single_errors_corrected += single_err;
    s_metrics.total_double_errors_detected += double_err;
    s_metrics.total_raw_bytes_recovered += raw_bytes;
    s_metrics.total_compressed_bytes_received += comp_bytes;

    /* Tính FER */
    if (s_metrics.total_frames_received > 0) {
        uint32_t lost_frames = s_metrics.total_frames_received - s_metrics.total_crc_valid_frames;
        s_metrics.fer = ((float)lost_frames / (float)s_metrics.total_frames_received) * 100.0f;
    }

    /* Tính tỷ lệ sửa lỗi thành công */
    uint32_t total_errors = s_metrics.total_single_errors_corrected + s_metrics.total_double_errors_detected;
    if (total_errors > 0) {
        s_metrics.correction_success_rate = ((float)s_metrics.total_single_errors_corrected / (float)total_errors) * 100.0f;
    } else {
        s_metrics.correction_success_rate = 100.0f;
    }

    /* Tính CR trung bình */
    if (s_metrics.total_compressed_bytes_received > 0) {
        s_metrics.avg_compression_ratio = (float)s_metrics.total_raw_bytes_recovered / (float)s_metrics.total_compressed_bytes_received;
    }
}

void metrics_get(global_metrics_t *out_metrics) {
    if (out_metrics) {
        *out_metrics = s_metrics;
    }
}
