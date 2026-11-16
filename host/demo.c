/* SPDX-License-Identifier: GPL-3.0-only
 * A demo on the PC: the demo song (app/demo_song.c, as the FM-1's sound bench plays it), written to a WAV file,
 * with the counters the SYSTEM page will show. Usage: demo out.wav */
#include "../engine/engine.h"
#include "../app/demo_song.h"
#include <stdio.h>
#include <math.h>

static int16_t fxmem[GB_FX_MEM];
static gb_sound S;

static void wav_header(FILE *f, uint32_t frames)
{
    uint32_t bytes = frames * 4, sr = 44100, br = sr * 4, sz = 36 + bytes;
    uint16_t pcm = 1, ch = 2, ba = 4, bits = 16;
    uint32_t fmt = 16;
    fwrite("RIFF", 1, 4, f); fwrite(&sz, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmt, 4, 1, f);
    fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f); fwrite(&ba, 2, 1, f);
    fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
}
int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "demo.wav";
    snd_init(&S, fxmem, 0);
    demo_song song;
    demo_song_init(&song, &S, 44100.0f, 124.0f, 16);
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); return 1; }
    const int bars = 16, frames = bars * 16 * (int)song.step_frames;
    wav_header(f, (uint32_t)((frames + GB_BLOCK - 1) / GB_BLOCK * GB_BLOCK));
    int32_t out[GB_BLOCK * 2];
    int16_t pcm[GB_BLOCK * 2];
    double peak = 0, acc = 0; long n = 0;
    for (int fr = 0; fr < frames; fr += GB_BLOCK) {
        demo_song_block(&song, &S);
        snd_render(&S, out);
        for (int i = 0; i < GB_BLOCK * 2; i++) {
            pcm[i] = (int16_t)(out[i] >> 8);
            double x = out[i] / 8388607.0; if (fabs(x) > peak) peak = fabs(x); acc += x * x; n++;
        }
        fwrite(pcm, sizeof pcm, 1, f);
    }
    fclose(f);
    printf("%s: %.1f s, peak %.1f dBFS, rms %.1f dBFS, voices peak %u, steals %u, tails cut %u\n", path,
           frames / 44100.0, 20 * log10(peak), 10 * log10(acc / n), S.voices_peak, S.steals, S.tails_cut);
    return 0;
}
