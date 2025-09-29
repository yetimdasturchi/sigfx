#include "dsp.h"

float db_to_lin(float db){ return powf(10.0f, db/20.0f); }
float randf_uniform(){ return (float)rand() / (float)RAND_MAX; }
float randf_normal() {
    float u1 = fmaxf(1e-12f, randf_uniform());
    float u2 = randf_uniform();
    return sqrtf(-2.0f*logf(u1)) * cosf(2.0f*(float)M_PI*u2);
}

Biquad biquad_lowpass(float sr, float fc, float q){
    float w0 = 2.0f*(float)M_PI*fc/sr;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s/(2.0f*q);
    float b0 = (1.0f - c)*0.5f, b1 = 1.0f - c, b2 = (1.0f - c)*0.5f;
    float a0 = 1.0f + alpha, a1 = -2.0f*c, a2 = 1.0f - alpha;
    Biquad qd = { b0/a0, b1/a0, b2/a0, a1/a0, a2/a0, 0, 0 };
    return qd;
}
Biquad biquad_highpass(float sr, float fc, float q){
    float w0 = 2.0f*(float)M_PI*fc/sr;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s/(2.0f*q);
    float b0 =  (1.0f + c)*0.5f, b1 = -(1.0f + c), b2 =  (1.0f + c)*0.5f;
    float a0 = 1.0f + alpha, a1 = -2.0f*c, a2 = 1.0f - alpha;
    Biquad qd = { b0/a0, b1/a0, b2/a0, a1/a0, a2/a0, 0, 0 };
    return qd;
}
Biquad biquad_peak(float sr, float fc, float q, float gain_db){
    float A = powf(10.0f, gain_db/40.0f);
    float w0 = 2.0f*(float)M_PI*fc/sr;
    float c = cosf(w0), s = sinf(w0);
    float alpha = s/(2.0f*q);
    float b0 = 1.0f + alpha*A;
    float b1 = -2.0f*c;
    float b2 = 1.0f - alpha*A;
    float a0 = 1.0f + alpha/A;
    float a1 = -2.0f*c;
    float a2 = 1.0f - alpha/A;
    Biquad qd = { b0/a0, b1/a0, b2/a0, a1/a0, a2/a0, 0, 0 };
    return qd;
}
void biquad_process_inplace(float *x, uint32_t n, Biquad *q){
    float z1=q->z1, z2=q->z2;
    float b0=q->b0, b1=q->b1, b2=q->b2, a1=q->a1, a2=q->a2;
    for (uint32_t i=0;i<n;i++){
        float in = x[i];
        float out = b0*in + z1;
        z1 = b1*in + z2 - a1*out;
        z2 = b2*in - a2*out;
        x[i] = out;
    }
    q->z1=z1; q->z2=z2;
}

void peak_normalize(float *x, uint32_t n, float target_db){
    float t = db_to_lin(target_db);
    float peak = 1e-9f;
    for (uint32_t i=0;i<n;i++){ float a=fabsf(x[i]); if (a>peak) peak=a; }
    float g = t/peak;
    for (uint32_t i=0;i<n;i++) x[i]*=g;
}
void rms_normalize(float *x, uint32_t n, float target_db){
    float t = db_to_lin(target_db);
    double acc=0.0;
    for (uint32_t i=0;i<n;i++){ acc += (double)x[i]*(double)x[i]; }
    float rms = sqrtf((float)(acc/(double)n));
    if (rms < 1e-9f) return;
    float g = t/rms;
    for (uint32_t i=0;i<n;i++) x[i]*=g;
}
void soft_limiter_tanh(float *x, uint32_t n, float drive_db){
    float d = db_to_lin(drive_db);
    float denom = tanhf(d);
    for (uint32_t i=0;i<n;i++){
        float y = tanhf(d * x[i]);
        x[i] = y / denom;
    }
}
void ring_mod(float *x, uint32_t n, float sr, float f_hz, float depth){
    float ph = 0.0f, dph = 2.0f*(float)M_PI*f_hz/sr;
    for (uint32_t i=0;i<n;i++){
        float lfo = sinf(ph);
        float y = (1.0f - depth)*x[i] + depth*(x[i]*lfo);
        x[i]=y;
        ph += dph; if (ph > 2.0f*(float)M_PI) ph -= 2.0f*(float)M_PI;
    }
}
void tremolo(float *x, uint32_t n, float sr, float rate_hz, float depth){
    float ph = 0.0f, dph = 2.0f*(float)M_PI*rate_hz/sr;
    for (uint32_t i=0;i<n;i++){
        float lfo = (1.0f - depth) + depth*(0.5f*(sinf(ph)+1.0f));
        x[i]*=lfo;
        ph += dph; if (ph > 2.0f*(float)M_PI) ph -= 2.0f*(float)M_PI;
    }
}
void bitcrush(float *x, uint32_t n, int bits){
    if (bits < 2) 
        bits = 2;
    if (bits > 16) 
        bits = 16;
    float levels = (float)(1<<bits);
    for (uint32_t i=0;i<n;i++){
        float v = (x[i]+1.0f)*0.5f;
        v = roundf(v*(levels-1.0f))/(levels-1.0f);
        x[i] = v*2.0f - 1.0f;
    }
}
void add_white_noise_snr(float *x, uint32_t n, float snr_db){
    double sp=0.0; for (uint32_t i=0;i<n;i++) sp += (double)x[i]*(double)x[i];
    float sig_pow = (float)(sp/(double)n);
    float noise_pow = sig_pow / powf(10.0f, snr_db/10.0f);
    float std = sqrtf(fmaxf(1e-12f, noise_pow));
    for (uint32_t i=0;i<n;i++){
        float z = randf_normal() * std;
        float y = x[i] + z;
        if (y>1.0f) y=1.0f; else if (y<-1.0f) y=-1.0f;
        x[i]=y;
    }
}
void clip_safe(float *x, uint32_t n){
    for (uint32_t i=0;i<n;i++){
        if (x[i]>1.0f) x[i]=1.0f; else if (x[i]<-1.0f) x[i]=-1.0f;
    }
}

void bandlimit(float *x, uint32_t n, float sr, float f_lo, float f_hi){
    float ny = 0.5f * sr;
    if (f_lo < 10.0f) f_lo = 10.0f;
    if (f_hi > ny - 200.0f) f_hi = ny - 200.0f;
    if (f_hi < f_lo + 50.0f) f_hi = f_lo + 50.0f;
    Biquad hp = biquad_highpass(sr, f_lo, 0.707f);
    Biquad lp = biquad_lowpass(sr, f_hi, 0.707f);
    biquad_process_inplace(x,n,&hp);
    biquad_process_inplace(x,n,&lp);
}
void bandpass_boost(float *x, uint32_t n, float sr, float f_center, float q, float gain_db){
    float ny = 0.5f * sr;
    float flo = fmaxf(50.0f, f_center/3.0f);
    float fhi = fminf(ny - 200.0f, f_center*3.0f);
    if (fhi < flo + 50.0f) fhi = flo + 50.0f;
    bandlimit(x,n,sr,flo,fhi);
    Biquad pk = biquad_peak(sr,f_center,q,gain_db);
    biquad_process_inplace(x,n,&pk);
}

float* resample_linear(const float *x, uint32_t n, float ratio, uint32_t *out_n){
    if (ratio <= 0.0001f) ratio = 0.0001f;
    uint32_t m = (uint32_t)fmaxf(1.0f, floorf((float)n * ratio));
    float *y = (float*)malloc(sizeof(float)*m);
    if (!y){ *out_n = 0; return NULL; }
    for (uint32_t i=0;i<m;i++){
        float pos = (float)i / ratio;
        if (pos >= (float)(n-1)){ y[i] = x[n-1]; continue; }
        uint32_t i0 = (uint32_t)floorf(pos);
        float frac = pos - (float)i0;
        uint32_t i1 = i0 + 1;
        float v = (1.0f-frac)*x[i0] + frac*x[i1];
        y[i]=v;
    }
    *out_n = m; return y;
}

uint32_t pitch_shift_change_duration(float **px, uint32_t n, float semitones){
    float r = powf(2.0f, semitones / 12.0f);
    if (!isfinite(r) || r <= 0.0f) return n;
    uint32_t n_out = 0;
    float *y = resample_linear(*px, n, 1.0f / r, &n_out);
    if (!y) return n;
    free(*px);
    *px = y;
    return n_out;
}

void pitch_shift_simple(float *x, uint32_t n, float semitones){
    float r = powf(2.0f, semitones / 12.0f);
    if (!isfinite(r) || r <= 0.0f) return;
    uint32_t n1=0; float *y = resample_linear(x, n, 1.0f / r, &n1);
    if (!y) return;
    uint32_t n2=0; float *z = resample_linear(y, n1, r, &n2);
    free(y);
    if (!z) return;
    uint32_t cpy = (n2 < n) ? n2 : n;
    memcpy(x, z, sizeof(float)*cpy);
    if (n2 < n) memset(x+cpy, 0, sizeof(float)*(n - cpy));
    free(z);
}
