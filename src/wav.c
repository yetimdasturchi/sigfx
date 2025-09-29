#include "wav.h"

static int read_u32le(FILE *f, uint32_t *v) {
    uint8_t b[4]; if (fread(b,1,4,f)!=4) return 0;
    *v = (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
    return 1;
}
static int read_u16le(FILE *f, uint16_t *v) {
    uint8_t b[2]; if (fread(b,1,2,f)!=2) return 0;
    *v = (uint16_t)b[0] | ((uint16_t)b[1]<<8);
    return 1;
}
static void write_u32le(FILE *f, uint32_t v){
    uint8_t b[4]={v&0xFF,(v>>8)&0xFF,(v>>16)&0xFF,(v>>24)&0xFF}; fwrite(b,1,4,f);
}
static void write_u16le(FILE *f, uint16_t v){
    uint8_t b[2]={v&0xFF,(v>>8)&0xFF}; fwrite(b,1,2,f);
}

int read_wav_header(FILE *f, WavInfo *info){
    memset(info,0,sizeof(*info));
    uint8_t riff[4]; if (fread(riff,1,4,f)!=4 || memcmp(riff,"RIFF",4)!=0) return 0;
    uint32_t riff_size; if (!read_u32le(f,&riff_size)) return 0;
    uint8_t wave[4]; if (fread(wave,1,4,f)!=4 || memcmp(wave,"WAVE",4)!=0) return 0;

    int fmt_found=0, data_found=0;
    while (!fmt_found || !data_found){
        uint8_t id[4]; if (fread(id,1,4,f)!=4) return 0;
        uint32_t sz; if (!read_u32le(f,&sz)) return 0;
        long chunk_start = ftell(f);
        if (memcmp(id,"fmt ",4)==0){
            fmt_found=1;
            if (!read_u16le(f,&info->audio_format)) return 0;
            if (!read_u16le(f,&info->num_channels)) return 0;
            if (!read_u32le(f,&info->sample_rate)) return 0;
            if (!read_u32le(f,&info->byte_rate)) return 0;
            if (!read_u16le(f,&info->block_align)) return 0;
            if (!read_u16le(f,&info->bits_per_sample)) return 0;
            fseek(f, chunk_start + sz, SEEK_SET);
        } else if (memcmp(id,"data",4)==0){
            data_found=1;
            info->data_size = sz;
            info->data_offset = ftell(f);
            fseek(f, sz, SEEK_CUR);
        } else {
            fseek(f, sz, SEEK_CUR);
        }
        if (sz & 1) fseek(f, 1, SEEK_CUR);
        if (feof(f)) break;
    }
    if (!fmt_found || !data_found) return 0;
    if (info->audio_format != 1 || info->bits_per_sample != 16) return 0;
    return 1;
}

int write_wav_file(const char *path, const float *interleaved, uint32_t nframes,
                   uint16_t channels, uint32_t sample_rate)
{
    FILE *f = fopen(path, "wb"); if (!f) return 0;
    uint32_t data_bytes = nframes * channels * 2;

    fwrite("RIFF",1,4,f);
    write_u32le(f, 36 + data_bytes);
    fwrite("WAVE",1,4,f);

    fwrite("fmt ",1,4,f); write_u32le(f,16);
    write_u16le(f,1); // PCM
    write_u16le(f,channels);
    write_u32le(f,sample_rate);
    write_u32le(f,sample_rate * channels * 2);
    write_u16le(f,channels * 2);
    write_u16le(f,16);

    fwrite("data",1,4,f); write_u32le(f,data_bytes);

    for (uint32_t i=0;i<nframes*channels;i++){
        float s = interleaved[i];
        if (s>1.0f) s=1.0f; else if (s<-1.0f) s=-1.0f;
        int16_t v = (int16_t)lrintf(s * 32767.0f);
        uint8_t b[2] = { (uint8_t)(v & 0xFF), (uint8_t)((v>>8)&0xFF) };
        fwrite(b,1,2,f);
    }
    fclose(f);
    return 1;
}

void split_interleaved_to_planar(const float *in, float *L, float *R,
                                 uint32_t nframes, uint16_t ch){
    if (ch==1){ memcpy(L,in,sizeof(float)*nframes); }
    else {
        for (uint32_t i=0;i<nframes;i++){ L[i]=in[2*i]; R[i]=in[2*i+1]; }
    }
}
void join_planar_to_interleaved(float *out, const float *L, const float *R,
                                uint32_t nframes, uint16_t ch){
    if (ch==1){ memcpy(out,L,sizeof(float)*nframes); }
    else {
        for (uint32_t i=0;i<nframes;i++){ out[2*i]=L[i]; out[2*i+1]=R[i]; }
    }
}
