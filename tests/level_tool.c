/* SPDX-License-Identifier: GPL-3.0-only
 * The factory sounds' loudness. level_tool: writes tools/levels.txt (each synth sound's gain, so that all are as
 * loud as the middle one); level_tool check: each within 2 dB (a test). Loudness: a note's loudest 100 ms in its first
 * second, the middle key (C4) with the sound's TRANS, without the effects. */
#include "../app/presets.h"
#include <stdio.h>
#include "../engine/sample.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
static int16_t fx[GB_FX_MEM];
static gb_sound S;
static double loud_at(int i, int with_gain, int note);
static double loud(int i, int with_gain)
{
    double a = loud_at(i, with_gain, 60 + preset_trans(i));
    return a > 1e-6 ? a : loud_at(i, with_gain, 38);          /* (a kit on a synth track: its snare) */
}
static double loud_at(int i, int with_gain, int note)
{
    snd_init(&S, fx, 0); sm_jump(&S.master, 0.3f);
    S.trk[1].patch.engine = gb_presets[i].engine;
    preset_apply(&S, 1, i);
    if (!with_gain) S.trk[1].patch.gain = 1.0f;
    sm_jump(&S.trk[1].level, 1.0f); sm_jump(&S.trk[1].send_dly, 0.0f); sm_jump(&S.trk[1].send_rev, 0.0f);
    snd_note_on(&S, GB_OWNER(OWN_KEY, 0), 1, note, 0.8f);
    /* the loudest 100 ms of the first second (a pluck and a slow pad compared at their fullest) */
    int32_t o[GB_BLOCK * 2]; enum { NB = 689, WIN = 69 }; static double eb[NB]; double best = 0, run = 0;
    for (int b = 0; b < NB; b++) {
        snd_render(&S, o); double e = 0;
        for (int n = 0; n < GB_BLOCK; n++) e += (double)snd_last_l[n] * snd_last_l[n] + (double)snd_last_r[n] * snd_last_r[n];
        eb[b] = e; run += e; if (b >= WIN) run -= eb[b - WIN];
        if (run > best) best = run;
    }
    return sqrt(best / (2.0 * WIN * GB_BLOCK)) / 0.3;
}
static int cmp(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
int main(int argc, char **argv)
{
    smp_bank_load("assets/samples.bin");
    int check = argc > 1 && !strcmp(argv[1], "check");
    static double v[512]; int n = 0;
    for (int i = 0; i < gb_npresets; i++) if (gb_presets[i].engine != ENG_DRUM) v[n++] = loud(i, 0);
    static double s[512]; memcpy(s, v, sizeof(double) * n); qsort(s, n, sizeof(double), cmp);
    double target = s[n / 2];
    if (!check) {
        FILE *f = fopen("tools/levels.txt", "w");
        for (int i = 0; i < gb_npresets; i++) {
            if (gb_presets[i].engine == ENG_DRUM) continue;
            double g = target / loud(i, 0); g = g < 0.05 ? 0.05 : g > 20.0 ? 20.0 : g;
            fprintf(f, "%s\t%.3f\n", gb_presets[i].name, g);
        }
        fclose(f); printf("tools/levels.txt written (%d sounds)\n", n);
        return 0;
    }
    int bad = 0; double lo = 99, hi = -99;
    for (int i = 0; i < gb_npresets; i++) {
        if (gb_presets[i].engine == ENG_DRUM) continue;
        double db = 20.0 * log10(loud(i, 1) / target);
        if (db < lo) lo = db;
        if (db > hi) hi = db;
        if (fabs(db) > 2.0) { bad++; printf("  %s: %+.1f dB\n", gb_presets[i].name, db); }
    }
    printf("levels: %d synth sounds within %+.1f .. %+.1f dB, %d off by more than 2 dB\n", n, lo, hi, bad);
    return bad != 0;
}
