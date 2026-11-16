/* SPDX-License-Identifier: GPL-3.0-only
 * DIGITAL: SLOOP's four-operator FM (docs/sloop/SPEC-MOTEURS.md), written for this project. Four sines, eight
 * algorithms, operator 4 with feedback, one modulation index shaped by a modulator envelope (it falls to a quarter
 * in MDEC), moved by the filter's and SHAPE's modulations and the velocity.
 *   E0 ALG 1..8   E1..E3 R2 R3 R4 (.5 1 2 3 .. 12 14 16)   E4 IDX   E5 MDEC   E6 FB */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 7, 0 }, { 0, 14, 1 }, { 0, 14, 1 }, { 0, 14, 1 }, { 0, 127, 60 }, { 0, 127, 60 },
                             { 0, 127, 0 }, { 0, 0, 0 } };
static const float RATIO[15] = { 0.5f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16 };

static void fm_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    if (fresh) { for (int i = 0; i < 4; i++) v->s.fm.ph[i] = 0; v->s.fm.fb[0] = v->s.fm.fb[1] = 0.0f; }
    v->s.fm.menv = 1.0f;                                              /* the modulator envelope starts over */
}
static void fm_spread(gb_voice *v, uint32_t r)
{
    for (int i = 0; i < 4; i++) v->s.fm.ph[i] += r * (uint32_t)(i + 1);
}
#define TURN 4294967296.0f
GB_HOT static int fm_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    int alg = gb_ev(p, 0);
    float dt = m->dt;
    uint32_t i1 = (uint32_t)(dt * TURN), i2 = (uint32_t)(gb_minf(dt * RATIO[gb_ev(p, 1)], 0.45f) * TURN);
    uint32_t i3 = (uint32_t)(gb_minf(dt * RATIO[gb_ev(p, 2)], 0.45f) * TURN), i4 = (uint32_t)(gb_minf(dt * RATIO[gb_ev(p, 3)], 0.45f) * TURN);
    /* the modulator envelope: towards a quarter, 99 % of the way in T(MDEC) */
    float n = gb_time_ms((float)gb_ev(p, 5)) * (0.001f * GB_SR);
    float k = n < 1.0f ? 0.0f : gb_exp2f(-6.6365f * (float)GB_BLOCK / n);
    v->s.fm.menv = 0.25f + (v->s.fm.menv - 0.25f) * k;
    float idx = (float)gb_ev(p, 4) * v->s.fm.menv + (m->cut + m->shape - 64.0f) + (v->vel * 127.0f - 96.0f) * 0.25f;
    /* in quarter turns of 2^32 (x << 2 wraps the phase, no overflow): 127 = two turns */
    idx = gb_clampf(idx, 0.0f, 127.0f) * (1.999f / 127.0f) * (TURN * 0.25f);
    float fb = (float)gb_ev(p, 6) * (1.999f / 127.0f) * (TURN * 0.25f);
    uint32_t p0 = v->s.fm.ph[0], p1 = v->s.fm.ph[1], p2 = v->s.fm.ph[2], p4 = v->s.fm.ph[3];
    float f1 = v->s.fm.fb[0], f2 = v->s.fm.fb[1];
#define M(x) ((uint32_t)(int32_t)((x) * idx) << 2)
    for (int i = 0; i < GB_BLOCK; i++) {
        float o1, o2, o3, o4 = gb_sin_u32(p4 + ((uint32_t)(int32_t)(0.5f * (f1 + f2) * fb) << 2)), s;
        f2 = f1; f1 = o4;
        switch (alg) {
        case 0: o3 = gb_sin_u32(p2 + M(o4)); o2 = gb_sin_u32(p1 + M(o3)); s = gb_sin_u32(p0 + M(o2)); break;
        case 1: o3 = gb_sin_u32(p2); o2 = gb_sin_u32(p1 + M(0.5f * (o3 + o4))); s = gb_sin_u32(p0 + M(o2)); break;
        case 2: o3 = gb_sin_u32(p2); o2 = gb_sin_u32(p1 + M(o3)); s = gb_sin_u32(p0 + M(0.5f * (o2 + o4))); break;
        case 3: o3 = gb_sin_u32(p2 + M(o4)); o2 = gb_sin_u32(p1); s = gb_sin_u32(p0 + M(0.5f * (o2 + o3))); break;
        case 4: o3 = gb_sin_u32(p2 + M(o4)); o2 = gb_sin_u32(p1); o1 = gb_sin_u32(p0 + M(o2)); s = 0.5f * (o1 + o3); break;
        case 5: { uint32_t d = M(o4); o3 = gb_sin_u32(p2 + d); o2 = gb_sin_u32(p1 + d); o1 = gb_sin_u32(p0 + d); s = (o1 + o2 + o3) * (1.0f / 3.0f); break; }
        case 6: o3 = gb_sin_u32(p2 + M(o4)); o2 = gb_sin_u32(p1); o1 = gb_sin_u32(p0); s = (o1 + o2 + o3) * (1.0f / 3.0f); break;
        default: o3 = gb_sin_u32(p2); o2 = gb_sin_u32(p1); o1 = gb_sin_u32(p0); s = 0.25f * (o1 + o2 + o3 + o4); break;
        }
        p0 += i1; p1 += i2; p2 += i3; p4 += i4;
        out[i] = s * 0.25f;                                           /* (full-scale sines: 6 dB below the others) */
    }
#undef M
    v->s.fm.ph[0] = p0; v->s.fm.ph[1] = p1; v->s.fm.ph[2] = p2; v->s.fm.ph[3] = p4;
    v->s.fm.fb[0] = f1; v->s.fm.fb[1] = f2;
    return 1;
}

const gb_engine gb_eng_fm = { "DIGITAL", ED, 0, 0, fm_start, fm_render, 0, fm_spread, 0 };
