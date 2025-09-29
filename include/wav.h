#ifndef WAV_H
#define WAV_H

#include "compat.h"

typedef struct {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_size;
    long     data_offset;
} WavInfo;

int  read_wav_header(FILE *f, WavInfo *info);
int  write_wav_file(const char *path, const float *interleaved, uint32_t nframes,
                    uint16_t channels, uint32_t sample_rate);

void split_interleaved_to_planar(const float *in, float *L, float *R,
                                 uint32_t nframes, uint16_t ch);
void join_planar_to_interleaved(float *out, const float *L, const float *R,
                                uint32_t nframes, uint16_t ch);

#endif