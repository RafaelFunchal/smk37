/* SPDX-License-Identifier: GPL-3.0-only
 * PHASE: phase distortion with SLOOP's parameters (docs/sloop/SPEC-MOTEURS.md), written for this project. A -cos
 * read through a bent phase; DCW how much it bends (ENV adds the envelope's share; the filter's and SHAPE's
 * modulations move it too). WAVE2 takes every other cycle. A second line DTN cents up, mixed or ring-modulated;
 * SUB a sine an octave down.
 *   E0 WAVE SAW SQR PLS DSIN SPLS RSAW RTRI RTRP   E2 DCW   E4 DTN cents   E6 SUB
 *   E1 WAVE2 - (none) or one of them               E3 ENV   E5 LINE MIX RING */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 7, 0 }, { 0, 8, 0 }, { 0, 127, 60 }, { 0, 127, 64 }, { 0, 127, 0 }, { 0, 1, 0 },
                             { 0, 127, 0 }, { 0, 0, 0 } };

/* a wave's bend for the block: its break points */
typedef struct { int w; float x, peak, rez, k0, k1; } bend;
static void bend_set(bend *b, int w, float d)
{
    b->w = w; b->x = 0.0f;
    switch (w) {
    case 0: b->x = 0.5f + 0.5f * d; b->k0 = 0.5f / b->x; b->k1 = 0.5f / (1.0f - b->x); break;           /* SAW */
    case 1: b->x = 0.5f - 0.475f * d; b->k0 = 0.5f / b->x; break;                                       /* SQR */
    case 2: b->x = 1.0f - 0.96875f * d; b->k0 = 1.0f / b->x; break;                                     /* PLS */
    case 3: b->x = 1.0f - 0.75f * d; b->k0 = 1.0f / b->x; b->k1 = 1.0f / gb_maxf(1.0f - b->x, 1e-4f); break;   /* DSIN */
    case 4: b->x = 1.0f - 0.5f * d; b->peak = gb_maxf(b->x * (0.5f + 0.45f * d), 1e-4f);              /* SPLS */
            b->k0 = 0.5f / b->peak; b->k1 = 0.5f / gb_maxf(b->x - b->peak, 1e-4f); break;
    default: b->rez = 0.6714f * d * 7.0f; break;                                                        /* the resonances */
    }
}
GB_INLINE float ncos(float w) { return -gb_sin_u32((uint32_t)(w * 4294967040.0f) + 0x40000000u); }   /* -cos(2 pi w) */
static float wave(const bend *b, float ph)
{
    float pd, x = b->x;
    switch (b->w) {
    case 0: pd = ph < x ? ph * b->k0 : 0.5f + (ph - x) * b->k1; break;
    case 1: pd = ph < x ? ph * b->k0 : ph < 0.5f ? 0.5f : ph < 0.5f + x ? 0.5f + (ph - 0.5f) * b->k0 : 0.9999f; break;
    case 2: pd = ph < x ? ph * b->k0 : 0.9999f; break;
    case 3: pd = ph < x ? ph * b->k0 : (ph - x) * b->k1; break;
    case 4: pd = ph >= x ? 0.9999f : ph < b->peak ? ph * b->k0 : 0.5f + (ph - b->peak) * b->k1; break;
    default: {                                                  /* a windowed cosine, rez its pitch above the note */
        float rp = ph + ph * b->rez; rp -= (float)(int)rp;
        float win = b->w == 5 ? 1.0f - ph : b->w == 6 ? (ph < 0.5f ? 2.0f * ph : 2.0f * (1.0f - ph))
                  : (ph < 0.25f ? 4.0f * ph : ph > 0.75f ? 4.0f * (1.0f - ph) : 1.0f);
        return ncos(rp) * 0.625f * win; }
    }
    return ncos(pd);
}

static void pd_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    if (fresh) v->s.pd.ph[0] = 0;
    v->s.pd.ph[1] = v->s.pd.ph[2] = 0; v->s.pd.w = 0;               /* line 2 and the sub from 0 at each note */
}
static void pd_spread(gb_voice *v, uint32_t r) { v->s.pd.ph[0] += r; v->s.pd.ph[1] += r; v->s.pd.ph[2] += r >> 1; }

GB_HOT static int pd_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    int w1 = gb_ev(p, 0), w2 = gb_ev(p, 1), det = gb_ev(p, 4), ring = gb_ev(p, 5), line2 = det || ring;
    float depth = (float)gb_ev(p, 2) + m->cut + m->env1 * (float)gb_ev(p, 3) + (m->shape - 64.0f);
    float d = gb_clampf(depth, 0.0f, 127.0f) * (0.8545f / 127.0f), sub = (float)gb_ev(p, 6) * 0.0061f;
    bend b1, b2;
    bend_set(&b1, w1, d); bend_set(&b2, w2 ? w2 - 1 : w1, d);
    uint32_t inc = (uint32_t)(m->dt * 4294967296.0f), inc2 = det ? (uint32_t)(gb_minf(m->dt * gb_exp2f((float)det * (1.0f / 1200.0f)), 0.45f) * 4294967296.0f) : inc;
    uint32_t p0 = v->s.pd.ph[0], p1 = v->s.pd.ph[1], p2 = v->s.pd.ph[2];
    int tg0 = v->s.pd.w & 1, tg1 = (v->s.pd.w >> 1) & 1;
    for (int i = 0; i < GB_BLOCK; i++) {
        float s = wave(tg0 ? &b2 : &b1, (float)p0 * (1.0f / 4294967296.0f));
        uint32_t o = p0; p0 += inc; if (p0 < o) tg0 ^= 1;
        if (line2) {
            float s2 = wave(tg1 ? &b2 : &b1, (float)p1 * (1.0f / 4294967296.0f));
            o = p1; p1 += inc2; if (p1 < o) tg1 ^= 1;
            s = ring ? s * s2 : 0.5f * (s + s2);
        }
        if (sub > 0.0f) { p2 += inc >> 1; s += gb_sin_u32(p2) * sub; }
        out[i] = s * 0.5f;
    }
    v->s.pd.ph[0] = p0; v->s.pd.ph[1] = p1; v->s.pd.ph[2] = p2; v->s.pd.w = (uint8_t)(tg0 | tg1 << 1);
    return 1;
}

const gb_engine gb_eng_pd = { "PHASE", ED, 0, 0, pd_start, pd_render, 0, pd_spread, 0 };
