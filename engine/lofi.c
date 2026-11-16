/* SPDX-License-Identifier: GPL-3.0-only
 * LOFI: a chip voice with SLOOP's parameters (docs/sloop/SPEC-MOTEURS.md), written for this project. A pulse,
 * triangle, saw, noise (an LFSR) or a 32-step 4-bit wave table, quantised to the chip's bits and held (a lower
 * rate: CRSH), a pitch sweep down, a vibrato, a fast arpeggio, a one-pole TONE. CHIP STEP: the chip's stepped
 * volume envelope (CRSH sets it) instead of the ADSR, a 32-step triangle, the duties 12.5 / 25 / 50 / 75 %.
 *   E0 CHIP 4BIT 4B/2 8BIT 1BIT STEP   E2 DUTY (WRAM: the table, DUTY / 8)   E4 SWP   E6 ARP OFF OCT MAJ MIN
 *   E1 WAVE PLS TRI SAW NOIS WRAM      E3 CRSH (STEP: the envelope)          E5 VIB   E7 TONE */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 4, 0 }, { 0, 4, 0 }, { 0, 127, 64 }, { 0, 127, 0 }, { 0, 127, 0 }, { 0, 127, 0 },
                             { 0, 3, 0 }, { 0, 127, 127 } };
enum { CHIP_4BIT, CHIP_4B2, CHIP_8BIT, CHIP_1BIT, CHIP_STEP };
enum { RW_PLS, RW_TRI, RW_SAW, RW_NOIS, RW_WRAM };

/* the wave tables: 16 shapes of 32 steps, 4 bits (0..15), made at start from formulas (lofi_tables) */
static uint8_t WT[16][32];
static int wt_ready;
static float wt_mean[16];
static void lofi_tables(void)
{
    for (int t = 0; t < 16; t++) {
        float sum = 0.0f;
        for (int i = 0; i < 32; i++) {
            float x = (float)i / 32.0f, y;                          /* y in -1..1 */
            float s1 = gb_sin_turn(x), s2 = gb_sin_turn(2 * x), s3 = gb_sin_turn(3 * x), s4 = gb_sin_turn(4 * x);
            switch (t) {
            case 0: y = x < 0.5f ? 1 : -1; break;                   /* SQ50 */
            case 1: y = x < 0.25f ? 1 : -1; break;                  /* SQ25 */
            case 2: y = x < 0.125f ? 1 : -1; break;                 /* SQ12 */
            case 3: y = s1; break;                                  /* SINE */
            case 4: y = x < 0.5f ? 4 * x - 1 : 3 - 4 * x; break;    /* TRI */
            case 5: y = 2 * x - 1; break;                           /* SAW */
            case 6: { float a = 2 * x - 1, b = 2 * (2 * x - (x >= 0.5f)) - 1; y = 0.66f * a + 0.34f * b; break; }   /* SAW2 */
            case 7: y = gb_clampf(3.0f * s1, -1, 1); break;         /* BAS1: a sloped square */
            case 8: y = (s1 + 0.6f * s2 + 0.4f * s3) / 1.6f; break; /* BAS2 */
            case 9: y = (s1 + 0.7f * s2 + 0.5f * s3 + 0.35f * s4) / 1.9f; break;   /* ORGN */
            case 10: y = (s1 + 0.45f * s3 + 0.25f * gb_sin_turn(5 * x)) / 1.3f; break;   /* HOLW: the odd ones */
            case 11: y = (0.5f * s1 + 0.9f * gb_sin_turn(7 * x) * (1 - x)) / 1.1f; break;   /* VOXA */
            case 12: y = (0.8f * s1 + 0.6f * s3 * (1 - x)) / 1.2f; break;          /* VOXO */
            case 13: y = (0.6f * s1 + 0.7f * gb_sin_turn(11 * x) * (1 - x)) / 1.1f; break;   /* VOXE */
            case 14: y = i == 0 ? 1 : i == 1 ? -1 : -0.6f + 0.1f * gb_sin_turn(4 * x); break;   /* BUZZ */
            default: y = 1.0f - 2.0f * (float)(i / 8) / 3.0f; break; /* STAIR */
            }
            int v = (int)((y + 1.0f) * 7.5f + 0.5f); v = v < 0 ? 0 : v > 15 ? 15 : v;
            WT[t][i] = (uint8_t)v; sum += (float)(2 * v - 15) / 15.0f;
        }
        wt_mean[t] = sum / 32.0f;
    }
    wt_ready = 1;
}
static int step_period(int b) { return b <= 16 ? b - 1 : b - 16; }

GB_SMALL static void lofi_start(gb_voice *v, const gb_patch *p, int fresh)
{
    if (!wt_ready) lofi_tables();
    if (fresh) { v->s.lofi.ph = 0; v->s.lofi.lp = 0.0f; }
    v->s.lofi.hold = 0.0f; v->s.lofi.t = 0; v->s.lofi.lfsr = 0x7FFF; v->s.lofi.sweep = 0.0f; v->s.lofi.step = 0;
    v->s.lofi.vib = 0.0f; v->s.lofi.arp = 0;
    int b = gb_ev(p, 3) >> 2;                                        /* STEP: its envelope starts over */
    v->s.lofi.vamp = 15.0f; v->s.lofi.step = (uint32_t)(step_period(b) & 15);
}
static void lofi_spread(gb_voice *v, uint32_t r) { v->s.lofi.ph += r; }

/* CHIP STEP: the stepped volume (15..0) at a 240 Hz frame clock; held, the ADSR only gates */
static float step_env(gb_voice *v, const gb_patch *p)
{
    int b = gb_ev(p, 3) >> 2;
    if (!b) return 15.0f;
    int vol = (int)v->s.lofi.vamp, div = (int)(v->s.lofi.step & 15);
    float fr = (float)(v->s.lofi.step >> 4) * (1.0f / 4096.0f) + 240.0f * (float)GB_BLOCK * GB_ISR;
    while (fr >= 1.0f) {
        fr -= 1.0f;
        if (div) div--;
        else { div = step_period(b); if (vol) vol--; else if (b > 16) vol = 15; }
    }
    v->s.lofi.vamp = (float)vol;
    v->s.lofi.step = (uint32_t)div | (uint32_t)(fr * 4096.0f) << 4;
    return (float)vol;
}

GB_HOT static int lofi_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    static const int8_t ARPS[4][3] = { { 0, 0, 0 }, { 0, 12, 0 }, { 0, 4, 7 }, { 0, 3, 7 } };
    static const uint8_t STEP_DUTY[4] = { 1, 2, 4, 6 };
    int chip = gb_ev(p, 0), wave = gb_ev(p, 1), dv = gb_ev(p, 2), stepc = chip == CHIP_STEP;
    int crush = stepc ? 0 : gb_ev(p, 3);
    int bits = chip == CHIP_8BIT ? 8 : chip == CHIP_1BIT ? 1 : 4;
    int hold = 1 + crush / 8 + (chip == CHIP_4B2);
    float duty = stepc ? (float)STEP_DUTY[(dv / 32) & 3] * 0.125f : (float)(1 + dv / 32) * 0.125f;
    /* the sweep (down, 1/16 semitone a 32 frames), the arpeggio (a step each ~20 ms), the vibrato (~5.4 Hz) */
    if (v->s.lofi.sweep < 256.0f) v->s.lofi.sweep += (float)gb_ev(p, 4) * (1.0f / 128.0f) * ((float)GB_BLOCK / 32.0f);
    v->s.lofi.t += GB_BLOCK;
    float pitch = m->pitch - v->s.lofi.sweep + (float)ARPS[gb_ev(p, 6)][(v->s.lofi.t / (28 * 32)) % 3];
    float dt = gb_minf(gb_note_hz(pitch) * GB_ISR, 0.45f);
    int vib = gb_ev(p, 5);
    if (vib) { v->s.lofi.vib += 5.38f * (float)GB_BLOCK * GB_ISR; v->s.lofi.vib -= (float)(int)v->s.lofi.vib; dt *= 1.0f + gb_sin_turn(v->s.lofi.vib) * (float)vib * 0.000122f; }
    float k = gb_minf(1.0f, (3000.0f + 256.0f * ((float)gb_ev(p, 7) + gb_maxf(m->cut, 0.0f))) * (1.0f / 32768.0f));
    float levels = (float)((1 << bits) - 1);                         /* the steps of a sample in `bits` */
    const uint8_t *wt = WT[(dv >> 3) & 15];
    float wm = wt_mean[(dv >> 3) & 15];
    uint32_t ph = v->s.lofi.ph, inc = (uint32_t)(dt * 4294967296.0f), lfsr = v->s.lofi.lfsr;
    float held = v->s.lofi.hold, lp = v->s.lofi.lp;
    int cnt = (int)v->s.lofi.arp;
    for (int i = 0; i < GB_BLOCK; i++) {
        if (--cnt <= 0) {
            float s; int q = bits < 16;
            cnt = hold;
            switch (wave) {
            case RW_TRI:
                if (stepc) { uint32_t kk = ph >> 27; s = (float)(int)(kk < 16 ? 15 - kk : kk - 16) * (2.0f / 15.0f) - 1.0f; q = 0; }
                else { float x = (float)ph * (1.0f / 4294967296.0f); s = x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x; }
                break;
            case RW_SAW: s = (float)ph * (2.0f / 4294967296.0f) - 1.0f; break;
            case RW_NOIS:
                if (ph + inc * (uint32_t)hold < ph || chip == CHIP_1BIT) lfsr = (lfsr >> 1) | (((lfsr ^ (lfsr >> 1)) & 1u) << 14);
                s = (lfsr & 1u) ? 1.0f : -1.0f;
                break;
            case RW_WRAM: {
                int b = wt[ph >> 27];
                s = (bits == 1 ? (b >= 8 ? 1.0f : -1.0f) : (float)(2 * b - 15) * (1.0f / 15.0f)) - (bits == 1 ? 0.0f : wm);
                q = 0; break; }
            default: { float x = (float)ph * (1.0f / 4294967296.0f); s = (x < duty ? 1.0f : -1.0f) - (2.0f * duty - 1.0f); break; }
            }
            if (q) { float y = (s + 1.0f) * 0.5f * levels; s = (float)(int)(y + 0.5f) / levels * 2.0f - 1.0f; }
            if (crush > 64) s = (float)(int)(s * 7.5f + (s >= 0.0f ? 0.5f : -0.5f)) * (1.0f / 7.5f);
            held = s;
        }
        ph += inc;
        lp += (held + GB_BIAS - lp) * k;
        out[i] = lp * 0.25f;
    }
    v->s.lofi.ph = ph; v->s.lofi.lfsr = lfsr; v->s.lofi.hold = held; v->s.lofi.lp = lp;
    v->s.lofi.arp = (uint8_t)(cnt < 0 ? 0 : cnt > 255 ? 255 : cnt);
    v->flat = (uint8_t)stepc;
    if (stepc) {                                                     /* the chip's volume, not the ADSR's curve */
        float g = step_env(v, p) * (1.0f / 15.0f);
        for (int i = 0; i < GB_BLOCK; i++) out[i] *= g;
        int b = gb_ev(p, 3) >> 2;
        if (b >= 1 && b <= 16 && v->s.lofi.vamp <= 0.0f) return 0;   /* a one-shot decay at 0: the end */
    }
    return 1;
}
/* STEP holds the ADSR at full while held (its own volume rules); the voice layer's envelope still ends it */
const gb_engine gb_eng_lofi = { "LOFI", ED, 0, 0, lofi_start, lofi_render, 0, lofi_spread, 0 };
