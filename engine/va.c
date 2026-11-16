/* SPDX-License-Identifier: GPL-3.0-only
 * ANALOG: SLOOP's parameters (docs/sloop/SPEC-MOTEURS.md), written for this project. Two oscillators of one wave
 * (band-limited saw and pulse, triangle, sine, PWM moved by SHAPE), the second DTN cents above; noise; a drive; a
 * state-variable filter, low-pass (or band / high-pass: the track's FTYPE), its cutoff following the key by KTR.
 *   E0 WAVE SAW SQR TRI SIN PWM   E2 MIX osc 2's share   E4 CUT  E6 DRV
 *   E1 DTN 0..127 cents           E3 NOIS                E5 RES  E7 KTR */
#include "engine.h"

#define CTL 16                       /* the filter's coefficients: every CTL frames */
static const gb_pd ED[8] = { { 0, 4, 0 }, { 0, 127, 10 }, { 0, 127, 64 }, { 0, 127, 0 }, { 0, 127, 90 }, { 0, 127, 30 },
                             { 0, 127, 0 }, { 0, 127, 64 } };

static void va_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    if (fresh) {                                                      /* from phase 0: an 808 starts the same each time */
        v->s.va.ph[0] = 0.0f; v->s.va.ph[1] = 0.25f;
        v->s.va.ic1 = v->s.va.ic2 = 0.0f;
        v->s.va.nz = v->seed;
    }
}
static void va_spread(gb_voice *v, uint32_t r)
{
    float x = (float)r * (1.0f / 4294967296.0f);
    v->s.va.ph[0] += x; v->s.va.ph[0] -= (float)(int)v->s.va.ph[0];
    v->s.va.ph[1] += x * 1.01f; v->s.va.ph[1] -= (float)(int)v->s.va.ph[1];
}

/* the oscillators, a short loop each (few live values: the FM-1 has 16 registers) */
GB_HOT __attribute__((noinline)) static void osc_saw(float *x, float *php, float dt, float gain)
{
    float ph = *php;
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) {
        float y = 2.0f * ph - 1.0f;
        if (ph < dt || ph > 1.0f - dt) y -= gb_blep(ph, dt);
        x[n] += y * gain;
        ph += dt; ph -= ph >= 1.0f ? 1.0f : 0.0f;
    }
    *php = ph;
}
GB_HOT __attribute__((noinline)) static void osc_pulse(float *x, float *php, float dt, float pw, float gain)
{
    float ph = *php;
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) {
        float pb = ph + 1.0f - pw; pb -= pb >= 1.0f ? 1.0f : 0.0f;
        float y = ph < pw ? 1.0f : -1.0f;
        if (ph < dt || ph > 1.0f - dt) y += gb_blep(ph, dt);
        if (pb < dt || pb > 1.0f - dt) y -= gb_blep(pb, dt);
        x[n] += y * gain;
        ph += dt; ph -= ph >= 1.0f ? 1.0f : 0.0f;
    }
    *php = ph;
}
GB_HOT __attribute__((noinline)) static void osc_tri(float *x, float *php, float dt, float gain)
{
    float ph = *php;
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) {
        float y = ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
        x[n] += y * gain;
        ph += dt; ph -= ph >= 1.0f ? 1.0f : 0.0f;
    }
    *php = ph;
}
GB_HOT __attribute__((noinline)) static void osc_sine(float *x, float *php, float dt, float gain)
{
    uint32_t ph = (uint32_t)(*php * 4294967296.0f), inc = (uint32_t)(dt * 4294967296.0f);
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) { x[n] += gb_sin_u32(ph) * gain; ph += inc; }
    *php = (float)ph * (1.0f / 4294967296.0f);
}
static void osc(int w, float *x, float *ph, float dt, float pw, float gain)
{
    switch (w) {
    case 1: osc_pulse(x, ph, dt, 0.5f, gain); break;
    case 2: osc_tri(x, ph, dt, gain); break;
    case 3: osc_sine(x, ph, dt, gain); break;
    case 4: osc_pulse(x, ph, dt, pw, gain); break;
    default: osc_saw(x, ph, dt, gain); break;
    }
}
GB_HOT __attribute__((noinline)) static void add_noise(float *x, uint32_t *seed, float gain)
{
    uint32_t r = *seed;
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) { r ^= r << 13; r ^= r >> 17; r ^= r << 5; x[n] += (float)(int32_t)r * (gain * (1.0f / 2147483648.0f)); }
    *seed = r;
}
/* DRV: 1x .. 4x into a soft clip, mixed in from dry as it grows (a clean range low) */
GB_HOT __attribute__((noinline)) static void drive(float *x, float d)
{
    float g = 1.0f + 3.0f * d;
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) { float y = gb_softclip(x[n] * g); x[n] += (y - x[n]) * d; }
}
/* the filter's knee: linear to half scale, then a soft limit (only the resonance's peaks saturate) */
GB_INLINE float knee(float y)
{
    float a = gb_absf(y);
    if (a <= 0.49f) return y;
    a = 0.49f + 0.5f * gb_softclip((a - 0.49f) * 2.0f);
    return y < 0.0f ? -a : a;
}

GB_HOT static int va_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    float x[GB_BLOCK];
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) x[n] = 0.0f;
    int w = gb_ev(p, 0), det = gb_ev(p, 1), mix = gb_ev(p, 2), nz = gb_ev(p, 3), dr = gb_ev(p, 6);
    float dt1 = m->dt, dt2 = det ? gb_minf(dt1 * gb_exp2f((float)det * (1.0f / 1200.0f)), 0.45f) : dt1;
    float pw = gb_clampf(0.5f + (m->shape - 64.0f) * (1.0f / 512.0f), 0.05f, 0.95f);
    float m2 = (float)mix * (1.0f / 127.0f), m1 = 1.0f - m2;
    if (mix) { osc(w, x, &v->s.va.ph[0], dt1, pw, m1); osc(w, x, &v->s.va.ph[1], dt2, pw, m2); }
    else { osc(w, x, &v->s.va.ph[0], dt1, pw, 1.0f); v->s.va.ph[1] = v->s.va.ph[0] + 0.25f; v->s.va.ph[1] -= (float)(int)v->s.va.ph[1]; }
    if (nz) add_noise(x, &v->s.va.nz, (float)nz * 0.00305f);
    if (dr) drive(x, (float)dr * (1.0f / 127.0f));
    /* the filter: its coefficients every CTL frames, the cutoff's modulation across the block */
    int ft = gb_pv(p->p, SP_FTYPE);
    float k = 2.0f - (float)gb_ev(p, 5) * (1.855f / 127.0f);
    float base = (float)gb_ev(p, 4) + (float)gb_ev(p, 7) * (v->pitch - 60.0f) * (1.0f / 256.0f);
    float c0 = m->cut0, dc = (m->cut - m->cut0) * ((float)CTL / (float)GB_BLOCK);
    float ic1 = v->s.va.ic1, ic2 = v->s.va.ic2;
    gb_svf f;
    for (int c = 0; c < GB_BLOCK; c += CTL) {
        gb_svf_set(&f, gb_cut_hz(gb_clampf(base + c0, 0.0f, 127.0f)), k);
        c0 += dc;
        float a1 = f.a1, a2 = f.a2, a3 = f.a3;
        if (!ft) {
            GB_UNROLL4
            for (int n = c; n < c + CTL; n++) {
                float v3 = 0.5f * x[n] - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
                out[n] = v2;
            }
        } else {
            for (int n = c; n < c + CTL; n++) {
                float in = 0.5f * x[n], v3 = in - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
                out[n] = ft == 1 ? v1 : in - k * v1 - v2;
            }
        }
        /* (3.1.8: + GB_BIAS: through an open filter the band's state falls to nothing within each flat half of a square,
         * every period of a held note: it settles at the bias instead) */
        ic1 = gb_clampf(ic1, -4.5f, 4.5f) + GB_BIAS; ic2 = gb_clampf(ic2, -4.5f, 4.5f);
    }
    v->s.va.ic1 = ic1; v->s.va.ic2 = gb_flush(ic2);
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) out[n] = knee(out[n]) * 2.0f * 0.5f;
    return 1;
}

const gb_engine gb_eng_va = { "ANALOG", ED, 0, 0, va_start, va_render, 0, va_spread, 0 };
