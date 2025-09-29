#include "presets.h"

Preset parse_preset(const char *s){
    if (!s) return PRESET_NONE;
    #define IS(name, val) if (strcmp(s,(name))==0) return (val)
    IS("none", PRESET_NONE);
    IS("normalize_peak", PRESET_NORMALIZE_PEAK);
    IS("normalize_rms", PRESET_NORMALIZE_RMS);
    IS("sweeten", PRESET_SWEETEN);
    IS("radio_vo", PRESET_RADIO_VO);
    IS("telephone", PRESET_TELEPHONE);
    IS("walkie_talkie", PRESET_WALKIE_TALKIE);
    IS("megaphone", PRESET_MEGAPHONE);
    IS("stadium_pa", PRESET_STADIUM_PA);
    IS("cave", PRESET_CAVE);
    IS("cathedral", PRESET_CATHEDRAL);
    IS("underwater", PRESET_UNDERWATER);
    IS("intercom", PRESET_INTERCOM);
    IS("vinyl_lofi", PRESET_VINYL_LOFI);
    IS("robot_ringmod", PRESET_ROBOT_RINGMOD);
    IS("alien_robot", PRESET_ALIEN_ROBOT);
    IS("chipmunk", PRESET_CHIPMUNK);
    IS("baritone", PRESET_BARITONE);
    IS("deep_voice", PRESET_DEEP_VOICE);
    IS("whisperish", PRESET_WHISPERISH);
    IS("pitch_up_fun", PRESET_PITCH_UP_FUN);
    return PRESET_NONE;
    #undef IS
}

void apply_preset_chain(float *x, uint32_t n, float sr, Preset p){
    switch (p){
        case PRESET_NONE: break;

        case PRESET_NORMALIZE_PEAK:
            peak_normalize(x,n,-1.0f); break;

        case PRESET_NORMALIZE_RMS:
            rms_normalize(x,n,-20.0f); break;

        case PRESET_SWEETEN: {
            Biquad hp = biquad_highpass(sr, 60.0f, 0.707f);
            biquad_process_inplace(x,n,&hp);
            Biquad pk = biquad_peak(sr, 3000.0f, 1.0f, 3.0f);
            biquad_process_inplace(x,n,&pk);
            rms_normalize(x,n,-20.0f);
            soft_limiter_tanh(x,n,4.0f);
            peak_normalize(x,n,-1.0f);
        } break;

        case PRESET_RADIO_VO: {
            bandlimit(x,n,sr,90.0f,9000.0f);
            Biquad pk = biquad_peak(sr, 1800.0f, 0.9f, 4.0f);
            biquad_process_inplace(x,n,&pk);
            rms_normalize(x,n,-20.0f);
            soft_limiter_tanh(x,n,3.0f);
            peak_normalize(x,n,-1.0f);
        } break;

        case PRESET_TELEPHONE:
            bandlimit(x,n,sr,300.0f,3400.0f);
            peak_normalize(x,n,-1.0f);
            break;

        case PRESET_WALKIE_TALKIE:
            bandlimit(x,n,sr,600.0f,3000.0f);
            bitcrush(x,n,6);
            soft_limiter_tanh(x,n,4.0f);
            peak_normalize(x,n,-1.0f);
            break;

        case PRESET_MEGAPHONE:
            bandpass_boost(x,n,sr,2000.0f,0.8f,6.0f);
            soft_limiter_tanh(x,n,10.0f);
            peak_normalize(x,n,-1.0f);
            break;

        case PRESET_STADIUM_PA:
            bandlimit(x,n,sr,120.0f,6500.0f);
            add_white_noise_snr(x,n,35.0f);
            peak_normalize(x,n,-1.0f);
            soft_limiter_tanh(x,n,2.0f);
            break;

        case PRESET_CAVE: {
            int d1 = (int)(0.12f*sr), d2 = (int)(0.27f*sr);
            float *src = (float*)malloc(sizeof(float)*n); if (!src) break;
            memcpy(src, x, sizeof(float)*n);
            for (uint32_t i = 0; i < n; i++) {
                if ((int)i + d1 < (int)n) x[i + d1] += 0.35f * src[i];
                if ((int)i + d2 < (int)n) x[i + d2] += 0.22f * src[i];
            }
            free(src);
            bandlimit(x,n,sr,120.0f,6000.0f);
            peak_normalize(x,n,-1.0f);
            soft_limiter_tanh(x,n,2.0f);
        } break;

        case PRESET_CATHEDRAL: {
            const float delays[] = {0.15f, 0.33f, 0.51f, 0.72f};
            const float gains [] = {0.35f, 0.25f, 0.18f, 0.12f};
            float *src = (float*)malloc(sizeof(float)*n); if (!src) break;
            memcpy(src, x, sizeof(float)*n);
            for (uint32_t i=0;i<n;i++){
                for (int k=0;k<4;k++){
                    int d = (int)(delays[k]*sr);
                    if ((int)i + d < (int)n) x[i + d] += gains[k] * src[i];
                }
            }
            free(src);
            bandlimit(x,n,sr,80.0f,8000.0f);
            peak_normalize(x,n,-1.0f);
            soft_limiter_tanh(x,n,2.0f);
        } break;

        case PRESET_UNDERWATER:
            bandlimit(x,n,sr,100.0f,800.0f);
            tremolo(x,n,sr,5.0f,0.4f);
            peak_normalize(x,n,-1.0f);
            break;

        case PRESET_INTERCOM:
            bandlimit(x,n,sr,700.0f,2800.0f);
            soft_limiter_tanh(x,n,6.0f);
            peak_normalize(x,n,-1.0f);
            break;

        case PRESET_VINYL_LOFI: {
            float ny = 0.5f * sr;
            float target = 8000.0f;
            if (sr <= 10000.0f) target = fmaxf(1000.0f, ny * 0.6f);

            uint32_t n_ds=0, n_us=0;
            float *ds = resample_linear(x,n, target / sr, &n_ds);
            if (!ds) break;
            float *us = resample_linear(ds,n_ds, sr / target, &n_us);
            free(ds);

            uint32_t cpy = (n_us < n) ? n_us : n;
            memset(x,0,sizeof(float)*n);
            if (us) { memcpy(x,us,sizeof(float)*cpy); free(us); }

            bandlimit(x,n,sr,150.0f, fminf(5000.0f, 0.5f*sr - 500.0f));
            bitcrush(x,n,7);
            add_white_noise_snr(x,n,28.0f);
            peak_normalize(x,n,-1.0f);
            soft_limiter_tanh(x,n,2.0f);
        } break;

        case PRESET_ROBOT_RINGMOD:
            ring_mod(x,n,sr,40.0f,1.0f);
            tremolo(x,n,sr,12.0f,0.25f);
            peak_normalize(x,n,-1.0f);
            soft_limiter_tanh(x,n,3.0f);
            break;

        case PRESET_ALIEN_ROBOT:
            ring_mod(x,n,sr,70.0f,1.0f);
            peak_normalize(x,n,-1.0f);
            soft_limiter_tanh(x,n,2.0f);
            break;

        case PRESET_CHIPMUNK: {
            float *buf = x; uint32_t nn = n;
            float *tmp = (float*)malloc(sizeof(float)*nn);
            if (tmp){ memcpy(tmp, buf, sizeof(float)*nn); x = tmp; }
            uint32_t n2 = pitch_shift_change_duration(&x, nn, +7.0f);
            uint32_t cpy = (n2 < n) ? n2 : n;
            memcpy(buf, x, sizeof(float)*cpy);
            if (n2 < n) memset(buf + cpy, 0, sizeof(float)*(n - cpy));
            if (tmp){ free(x); }
            peak_normalize(buf,n,-1.0f);
            soft_limiter_tanh(buf,n,1.5f);
            return;
        }

        case PRESET_BARITONE:
        case PRESET_DEEP_VOICE: {
            float *buf = x; uint32_t nn = n;
            float *tmp = (float*)malloc(sizeof(float)*nn);
            if (tmp){ memcpy(tmp, buf, sizeof(float)*nn); x = tmp; }
            uint32_t n2 = pitch_shift_change_duration(&x, nn, -5.0f);
            uint32_t cpy = (n2 < n) ? n2 : n;
            memcpy(buf, x, sizeof(float)*cpy);
            if (n2 < n) memset(buf + cpy, 0, sizeof(float)*(n - cpy));
            if (tmp){ free(x); }
            bandlimit(buf,n,sr,80.0f,4500.0f);
            peak_normalize(buf,n,-1.0f);
            soft_limiter_tanh(buf,n,3.0f);
            return;
        }

        case PRESET_WHISPERISH: {
            Biquad hp = biquad_highpass(sr,2000.0f,0.707f);
            biquad_process_inplace(x,n,&hp);
            add_white_noise_snr(x,n,20.0f);
            rms_normalize(x,n,-22.0f);
            clip_safe(x,n);
        } break;

        case PRESET_PITCH_UP_FUN: {
            float *buf = x; uint32_t nn = n;
            float *tmp = (float*)malloc(sizeof(float)*nn);
            if (tmp){ memcpy(tmp, buf, sizeof(float)*nn); x = tmp; }
            uint32_t n2 = pitch_shift_change_duration(&x, nn, +3.0f);
            uint32_t cpy = (n2 < n) ? n2 : n;
            memcpy(buf, x, sizeof(float)*cpy);
            if (n2 < n) memset(buf + cpy, 0, sizeof(float)*(n - cpy));
            if (tmp){ free(x); }
            peak_normalize(buf,n,-1.0f);
            soft_limiter_tanh(buf,n,2.0f);
            return;
        }
    }
}
