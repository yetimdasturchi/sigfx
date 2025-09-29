#include "compat.h"
#include "wav.h"
#include "presets.h"

int main(int argc, char **argv){
    if (argc < 2){
        fprintf(stderr, "Foydalanish: %s in.wav\n", argv[0]);
        return 1;
    }
    const char *inpath = argv[1];
    srand((unsigned)time(NULL));

    FILE *f = fopen(inpath,"rb");
    if (!f){ fprintf(stderr,"Kirish faylini ochishda xatolik.\n"); return 1; }

    WavInfo wi;
    if (!read_wav_header(f,&wi)){
        fprintf(stderr,"Qo'llab-quvvatlanilmaydigan WAV fayli. Faqatgina PCM 16-bit bo'lishi lozim\n");
        fclose(f); return 1;
    }
    fseek(f, wi.data_offset, SEEK_SET);
    uint32_t nframes = wi.data_size / (wi.num_channels * 2);

    int16_t *pcm = (int16_t*)malloc(wi.data_size);
    if (!pcm){ fclose(f); fprintf(stderr,"Xotira ajratishda xatolik.\n"); return 1; }
    if (fread(pcm,1,wi.data_size,f)!=wi.data_size){ free(pcm); fclose(f); fprintf(stderr,"Auduio uzunligi yetarli emas.\n"); return 1; }
    fclose(f);

    float *buf_orig = (float*)malloc(sizeof(float)*nframes*wi.num_channels);
    if (!buf_orig){ free(pcm); fprintf(stderr,"Xotira ajratishda xatolik.\n"); return 1; }
    for (uint32_t i=0;i<nframes*wi.num_channels;i++) buf_orig[i] = (float)pcm[i] / 32768.0f;
    free(pcm);

    const char *preset_names[] = {
        "none","normalize_peak","normalize_rms","sweeten","radio_vo",
        "telephone","walkie_talkie","megaphone","stadium_pa","cave",
        "cathedral","underwater","intercom","vinyl_lofi","robot_ringmod",
        "alien_robot","chipmunk","baritone","deep_voice","whisperish","pitch_up_fun"
    };
    int num_presets = (int)(sizeof(preset_names)/sizeof(preset_names[0]));

    MKDIR_P("out");

    for (int pi=0; pi<num_presets; pi++){
        Preset preset = parse_preset(preset_names[pi]);

        float *buf = (float*)malloc(sizeof(float)*nframes*wi.num_channels);
        memcpy(buf, buf_orig, sizeof(float)*nframes*wi.num_channels);

        float *L = (float*)malloc(sizeof(float)*nframes);
        float *R = (wi.num_channels==2) ? (float*)malloc(sizeof(float)*nframes) : NULL;
        split_interleaved_to_planar(buf,L,R,nframes,wi.num_channels);

        apply_preset_chain(L, nframes, (float)wi.sample_rate, preset);
        if (wi.num_channels==2) apply_preset_chain(R, nframes, (float)wi.sample_rate, preset);

        join_planar_to_interleaved(buf,L,R,nframes,wi.num_channels);

        char outname[512];
        snprintf(outname,sizeof(outname),"out/%s.wav",preset_names[pi]);

        int ok = write_wav_file(outname, buf, nframes, wi.num_channels, wi.sample_rate);
        if (ok) printf("Chiqish %s\n", outname);
        else fprintf(stderr,"Faylni saqlashda xatolik %s\n", outname);

        free(buf); free(L); if (R) free(R);
    }

    free(buf_orig);
    return 0;
}
