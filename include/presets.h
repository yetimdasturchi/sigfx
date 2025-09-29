#ifndef PRESETS_H
#define PRESETS_H

#include "compat.h"
#include "dsp.h"

typedef enum {
    PRESET_NONE,
    PRESET_NORMALIZE_PEAK,
    PRESET_NORMALIZE_RMS,
    PRESET_SWEETEN,
    PRESET_RADIO_VO,
    PRESET_TELEPHONE,
    PRESET_WALKIE_TALKIE,
    PRESET_MEGAPHONE,
    PRESET_STADIUM_PA,
    PRESET_CAVE,
    PRESET_CATHEDRAL,
    PRESET_UNDERWATER,
    PRESET_INTERCOM,
    PRESET_VINYL_LOFI,
    PRESET_ROBOT_RINGMOD,
    PRESET_ALIEN_ROBOT,
    PRESET_CHIPMUNK,
    PRESET_BARITONE,
    PRESET_DEEP_VOICE,
    PRESET_WHISPERISH,
    PRESET_PITCH_UP_FUN
} Preset;

Preset parse_preset(const char *s);
void   apply_preset_chain(float *x, uint32_t n, float sr, Preset p);

#endif
