#ifndef DSP_H
#define DSP_H

#include "compat.h"

typedef struct {
    float b0,b1,b2,a1,a2;
    float z1,z2;
} Biquad;

Biquad biquad_lowpass(float sr, float fc, float q);
Biquad biquad_highpass(float sr, float fc, float q);
Biquad biquad_peak(float sr, float fc, float q, float gain_db);
void   biquad_process_inplace(float *x, uint32_t n, Biquad *q);

float db_to_lin(float db);
float randf_uniform(void);
float randf_normal(void);

void peak_normalize(float *x, uint32_t n, float target_db);
void rms_normalize(float *x, uint32_t n, float target_db);
void soft_limiter_tanh(float *x, uint32_t n, float drive_db);
void ring_mod(float *x, uint32_t n, float sr, float f_hz, float depth);
void tremolo(float *x, uint32_t n, float sr, float rate_hz, float depth);
void bitcrush(float *x, uint32_t n, int bits);
void add_white_noise_snr(float *x, uint32_t n, float snr_db);
void clip_safe(float *x, uint32_t n);

void bandlimit(float *x, uint32_t n, float sr, float f_lo, float f_hi);
void bandpass_boost(float *x, uint32_t n, float sr, float f_center, float q, float gain_db);

float*   resample_linear(const float *x, uint32_t n, float ratio, uint32_t *out_n);
uint32_t pitch_shift_change_duration(float **px, uint32_t n, float semitones);
void     pitch_shift_simple(float *x, uint32_t n, float semitones);

#endif