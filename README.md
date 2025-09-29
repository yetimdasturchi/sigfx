# voxfx

A tiny C DSP for quick **voice effects**. 

---

## Features

- Reads standard **16-bit PCM WAV** (mono or stereo), writes WAV.
- Clean, dependency-light C code (only `libm`).
- A collection of ready-to-use **presets**:
  - `none`
  - `normalize_peak`, `normalize_rms`
  - `sweeten`, `radio_vo`
  - `telephone`, `walkie_talkie`, `megaphone`, `stadium_pa`
  - `cave`, `cathedral`
  - `underwater`, `intercom`, `vinyl_lofi`
  - `robot_ringmod`, `alien_robot`
  - `chipmunk`, `baritone`, `deep_voice`, `whisperish`, `pitch_up_fun`
- Simple **biquad EQ** (LP/HP/peaking), limiter (`tanh`), tremolo, ring mod, bitcrush, band-limiting.
- Minimal linear **resampler** and toy **pitch-shift** helpers.

---

# Build
Requirements: a C compiler (GCC/Clang), `make`, and the C math library.


```bash
make clean
make
```

# Usage

```
./voxfx in.wav
```

After converting chech `out/` folder


# Tips

-   Input must be **16-bit PCM** WAV. If you have another format, convert first (e.g., with `ffmpeg -i input.mp3 -ac 1 -ar 44100 -sample_fmt s16 in.wav`).
    
-   Stereo files are split to L/R, processed per-channel, then re-interleaved.