/* SPDX-License-Identifier: GPL-3.0-only
 * NOISE: SLOOP's parameters (docs/sloop/SPEC-MOTEURS.md), written for this project. A source (white noise and
 * random impulses, the impulses alone, a long shift register clocked above the key, a short register's sequence
 * once per period: a metallic tone), its colour (white, pink, brown), a resonant filter morphing from low-pass to
 * a narrow band-pass, key tracking, a slow random drift of the frequency (the level following it), a crush.
 *   E0 MODE ANLG DUST LFSR META   E2 FREQ   E4 TRK              E6 DRFT
 *   E1 COLR                       E3 RES    E5 DENS (LFSR, META: LEN)   E7 CRSH (LFSR, META: CLK semitones) */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 3, 0 }, { 0, 127, 64 }, { 0, 127, 80 }, { 0, 127, 30 }, { 0, 127, 64 }, { 0, 127, 0 },
                             { 0, 127, 0 }, { 0, 127, 0 } };
enum { NZ_ANLG, NZ_DUST, NZ_LFSR, NZ_META };
/* the long registers (Galois taps, maximal length): 23 20 17 15 13 11 9 7 bits */
static const uint32_t TAPS[8] = { 0x420000u, 0x90000u, 1u << 16 | 1u << 13, 0x6000u, 0x100Du, 0x500u, 0x110u, 0x60u };
/* META: one period of a short register's output, made at start (meta_tables): 7-bit (two taps), the 15-bit
 * register fed back from bits 0 and 6 (93 steps), 6-bit (two taps), 5, 4, 3 bits */
static uint8_t SEQ[8][128];
static uint8_t SEQ_N[8];
static float SEQ_HI[8], SEQ_LO[8];
static int seq_ready;
static void meta_tables(void)
{
    static const uint16_t T[8] = { 0x60, 0x78, 0, 0x30, 0x2D, 0x14, 0x0C, 0x06 };
    for (int s = 0; s < 8; s++) {
        int n = 0, ones = 0;
        if (s == 2) {                                                 /* 15 bits, feedback from bits 0 and 6 */
            uint32_t r = 1;
            do { SEQ[s][n] = (uint8_t)(r & 1u); ones += r & 1u; uint32_t b = (r ^ (r >> 6)) & 1u; r = (r >> 1) | b << 14; n++; } while (r != 1u && n < 128);
        } else {
            uint32_t r = 1;
            do { SEQ[s][n] = (uint8_t)(r & 1u); ones += r & 1u; r = (r >> 1) ^ (-(r & 1u) & T[s]); n++; } while (r != 1u && n < 128);
        }
        SEQ_N[s] = (uint8_t)n;
        /* the levels of a 1 and a 0: no DC, the larger at 0.5 */
        float hi = (float)(n - ones), lo = -(float)ones, m = gb_maxf(hi, -lo);
        SEQ_HI[s] = 0.5f * hi / m; SEQ_LO[s] = 0.5f * lo / m;
    }
    seq_ready = 1;
}

GB_SMALL static void nz_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    if (!seq_ready) meta_tables();
    uint32_t seed = (uint32_t)(v->note + 1) * 0x9E3779B9u ^ (uint32_t)(v->fine * 1000.0f) * 0x85EBCA6Bu;
    v->s.noise.rng = seed ? seed : 1u;                               /* a note from silence: the same each time */
    if (fresh) { v->s.noise.ic1 = v->s.noise.ic2 = 0.0f; v->s.noise.b0 = v->s.noise.b1 = v->s.noise.b2 = v->s.noise.br = 0.0f; }
    v->s.noise.dcnt = 0; v->s.noise.ph = 0; v->s.noise.reg = 0; v->s.noise.dc = 0.0f; v->s.noise.hold = 0; v->s.noise.drift = 0.0f;
}
static void nz_spread(gb_voice *v, uint32_t r) { v->s.noise.rng ^= r | 1u; v->s.noise.ph += r; }
GB_INLINE uint32_t rnd(uint32_t *s) { uint32_t r = *s; r ^= r << 13; r ^= r >> 17; r ^= r << 5; *s = r; return r; }

GB_HOT static int nz_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    int mode = gb_ev(p, 0), res = gb_ev(p, 3), drft = gb_ev(p, 6), sel = gb_ev(p, 5) >> 4, crsh = mode < NZ_LFSR ? gb_ev(p, 7) : 0;
    uint32_t rng = v->s.noise.rng;
    /* the drift: a new random target every 0.7 .. 3 s, followed with a 0.7 s lag */
    float dsemi = 0.0f, gain = 1.0f;
    if (drft) {
        if (--v->s.noise.dcnt <= 0) { uint32_t r = rnd(&rng); v->s.noise.dtg = (float)(int32_t)(r >> 16 << 16) * (1.0f / 2147483648.0f); v->s.noise.dcnt = (int16_t)((1024 + (int)(r & 3071u)) / 2); }
        v->s.noise.drift += (v->s.noise.dtg - v->s.noise.drift) * (2.0f / 1024.0f);
        float d = v->s.noise.drift;
        dsemi = d * (float)(drft * drft) * (16.0f / 16129.0f);        /* up to about +-16 semitones */
        gain = 1.0f - 0.5f * (1.0f - d) * (float)drft * (1.0f / 256.0f);
    }
    /* COLR (+ SHAPE): white -> pink -> brown */
    float colr = gb_clampf((float)gb_ev(p, 1) + m->shape - 64.0f, 0.0f, 127.0f);
    float cf = colr <= 64.0f ? colr * (1.0f / 64.0f) : (colr - 64.0f) * (1.0f / 63.0f);
    /* FREQ (+ the filter's modulation), key-tracked around C4 by TRK, the drift */
    float cut = gb_clampf((float)gb_ev(p, 2) + m->cut + (v->pitch - 60.0f) * (float)gb_ev(p, 4) * (1.171f / 127.0f) + dsemi * 1.168f, 0.0f, 127.0f);
    gb_svf f; gb_svf_set(&f, gb_cut_hz(cut), 2.0f - (float)res * (1.904f / 127.0f));
    float bpg = 1.5f * gb_sqrtf(f.k), mix = (float)res * (1.0f / 127.0f);
    /* the level: most of what the filter takes back (less for pink and brown), DUST +12 dB, the drift's */
    float wf = 1.0f - colr * (230.0f / 32767.0f), mk = (127.0f - cut) * 7.0f * wf + 214.0f + (mode == NZ_DUST ? 512.0f : 0.0f);
    float g = gb_exp2f(gb_minf(mk, 1279.0f) * (1.0f / 256.0f)) * gain;
    float thr = 0.0f;
    if (mode <= NZ_DUST && gb_ev(p, 5)) thr = gb_exp2f((float)gb_ev(p, 5) * (11.0f / 127.0f)) * GB_ISR;   /* 1 .. 2000 a second */
    uint32_t ph = v->s.noise.ph, reg = v->s.noise.reg, taps = TAPS[sel];
    float inc = 0.0f;
    if (mode >= NZ_LFSR) inc = gb_note_hz(m->pitch + (float)gb_ev(p, 7) + dsemi) * GB_ISR;
    if (mode == NZ_LFSR) {
        uint32_t mask = taps; mask |= mask >> 1; mask |= mask >> 2; mask |= mask >> 4; mask |= mask >> 8; mask |= mask >> 16;
        if (!(reg & mask)) reg = rng | 1u;                            /* a new note: the register from its seed */
        reg &= mask;
    }
    float ic1 = v->s.noise.ic1, ic2 = v->s.noise.ic2, b0 = v->s.noise.b0, b1 = v->s.noise.b1, b2 = v->s.noise.b2, br = v->s.noise.br;
    float a1 = f.a1, a2 = f.a2, a3 = f.a3, dc = v->s.noise.dc, held = v->s.noise.held;
    float frac = (float)(ph & 0xFFFFu) * (1.0f / 65536.0f);
    int hold = v->s.noise.hold;
    for (int i = 0; i < GB_BLOCK; i++) {
        float w;
        if (mode <= NZ_DUST) {
            uint32_t r = rnd(&rng);
            w = mode == NZ_ANLG ? ((float)(r >> 17) - 16384.0f) * (1.0f / 32768.0f) : 0.0f;
            if ((float)(r & 0xFFFFu) * (1.0f / 65536.0f) < thr) { float a = 0.25f + (float)((r >> 18) & 0x7FFFu) * (1.0f / 32768.0f); w += (r & 0x20000u) ? a : -a; }
        } else if (mode == NZ_LFSR) {
            frac += inc;
            int ns = (int)frac; frac -= (float)ns; ns = ns > 64 ? 64 : ns;
            while (ns--) reg = (reg >> 1) ^ (-(reg & 1u) & taps);
            w = (reg & 1u) ? 0.366f : -0.366f;
        } else {
            frac += inc; frac -= (float)(int)frac;
            int j = (int)(frac * (float)SEQ_N[sel]);
            w = SEQ[sel][j] ? SEQ_HI[sel] : SEQ_LO[sel];
        }
        float x = w;
        if (colr > 0.0f) {                                            /* pink: three leaky poles; brown: a leaky integrator */
            b0 = 0.99765f * b0 + w * 0.0990f; b1 = 0.963f * b1 + w * 0.2965f; b2 = 0.570f * b2 + w * 1.0527f;
            float pk = 0.25f * (b0 + b1 + b2 + w * 0.1848f);
            if (colr <= 64.0f) x = w + (pk - w) * cf;
            else { br = 0.99426f * br + w * 0.1062f; x = pk + (br - pk) * cf; }
        }
        float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        float y = v2 + (v1 * bpg - v2) * mix;
        y *= g;
        float ay = gb_absf(y);
        if (ay > 0.73f) { ay = 0.73f + 0.27f * gb_softclip((ay - 0.73f) * (1.0f / 0.27f)); y = y < 0.0f ? -ay : ay; }
        if (mode == NZ_META) { dc += (y - dc) * (1.0f / 512.0f); y -= dc; }   /* a DC blocker, ~14 Hz */
        if (crsh) {                                                   /* hold 1..16 samples, fewer bits */
            if (--hold <= 0) { hold = 1 + crsh / 8; float q = (float)(1 << (crsh / 11)) * (1.0f / 32768.0f); held = (float)(int)(y / q) * q; }
            y = held;
        }
        out[i] = y * 0.5f;
    }
    v->s.noise.ic1 = gb_flush(gb_clampf(ic1, -4.5f, 4.5f)); v->s.noise.ic2 = gb_flush(gb_clampf(ic2, -4.5f, 4.5f));
    v->s.noise.b0 = gb_flush(b0); v->s.noise.b1 = gb_flush(b1); v->s.noise.b2 = gb_flush(b2); v->s.noise.br = gb_flush(br);
    v->s.noise.rng = rng; v->s.noise.reg = reg; v->s.noise.dc = dc; v->s.noise.held = held; v->s.noise.hold = (uint8_t)(hold < 0 ? 0 : hold);
    v->s.noise.ph = (ph & 0xFFFF0000u) | (uint32_t)(frac * 65535.0f);
    return 1;
}

const gb_engine gb_eng_noise = { "NOISE", ED, 0, 0, nz_start, nz_render, 0, nz_spread, 0 };
