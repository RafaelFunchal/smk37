/* SPDX-License-Identifier: GPL-3.0-only
 * VOICE: sung vowels with SLOOP's parameters (docs/sloop/SPEC-MOTEURS.md), written for this project. A glottal
 * source (a Rosenberg-like pulse: a rising half-sine while it opens, a falling quarter-sine while it closes) into
 * four resonators in cascade (Klatt 1980); the vowel formants of Hillenbrand et al. (1995), A E I O U. TALK glides
 * from VOWL to VOWL2 after the note's start; SHIFT moves the formants; BUZZ the open quotient and the tilt; BRTH
 * the breath; Q the bandwidths; RAND pulls both vowels towards one at random for each note.
 *   E0 VOWL  E1 VOWL2  E2 TALK  E3 SHIFT -12..12  E4 BUZZ  E5 BRTH  E6 Q  E7 RAND */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 127, 0 }, { 0, 127, 64 }, { 0, 127, 0 }, { -12, 12, 0 }, { 0, 127, 64 }, { 0, 127, 10 },
                             { 0, 127, 64 }, { 0, 127, 0 } };
static const float VF[5][3] = { { 768, 1333, 2522 }, { 580, 1799, 2605 }, { 342, 2322, 3000 }, { 497, 910, 2459 }, { 378, 997, 2343 } };
static const float VB[5][3] = { { 90, 100, 160 }, { 70, 100, 170 }, { 50, 120, 200 }, { 70, 80, 150 }, { 60, 80, 150 } };
static uint32_t vnz = 0x2545F491u;

static void vowel_at(float pos, float *f, float *b)
{
    float x = gb_clampf(pos, 0.0f, 127.0f) * (4.0f / 127.0f);
    int i = (int)x; float fr = x - (float)i;
    if (i >= 4) { i = 3; fr = 1.0f; }
    for (int j = 0; j < 3; j++) { f[j] = VF[i][j] + (VF[i + 1][j] - VF[i][j]) * fr; b[j] = VB[i][j] + (VB[i + 1][j] - VB[i][j]) * fr; }
}
typedef struct { float a, b, c; } res;
static void res_set(res *r, float f, float bw)
{
    f = gb_clampf(f, 60.0f, 7000.0f); bw = gb_clampf(bw, 20.0f, 1500.0f);
    float e = gb_exp2f(-1.442695f * 3.14159265f * bw * GB_ISR), cs = gb_sin_turn(f * GB_ISR + 0.25f);
    r->b = 2.0f * e * cs; r->c = -e * e; r->a = 1.0f - r->b - r->c;
}
static void vo_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    vnz ^= vnz << 13; vnz ^= vnz >> 17; vnz ^= vnz << 5;
    v->s.voice.rv = (float)(vnz >> 25);                               /* this note's random vowel (RAND) */
    v->s.voice.talk = 0.0f;                                           /* TALK starts over */
    if (fresh) { v->s.voice.ph = 0; for (int i = 0; i < 8; i++) v->s.voice.y[i] = 0.0f; v->s.voice.lp = 0.0f; }
}
GB_HOT static int vo_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    /* the vowel: VOWL -> VOWL2 over TALK (an ease-out), the filter's modulation moves both, RAND pulls them */
    float k = 0.0f;
    int tk = gb_ev(p, 2);
    if (tk) {
        v->s.voice.talk = gb_minf(1.0f, v->s.voice.talk + (float)GB_BLOCK / (gb_time_ms((float)tk) * 0.001f * GB_SR));
        k = v->s.voice.talk * (2.0f - v->s.voice.talk);
    }
    float rnd = (float)gb_ev(p, 7) * (1.0f / 127.0f), v0 = (float)gb_ev(p, 0), v1 = (float)gb_ev(p, 1);
    v0 += (v->s.voice.rv - v0) * rnd; v1 += (v->s.voice.rv - v1) * rnd;
    float fa[3], ba[3], fb[3], bb[3];
    vowel_at(v0 + m->cut, fa, ba); vowel_at(v1 + m->cut, fb, bb);
    float ratio = gb_exp2f(((float)gb_ev(p, 3) + 30.0f / 127.0f * (v->pitch - 60.0f)) * (1.0f / 12.0f));
    float bws = gb_exp2f((float)(64 - gb_ev(p, 6)) * (4.0f / 192.0f)), f0 = gb_note_hz(m->pitch);
    res r[4];
    for (int j = 0; j < 3; j++) {
        float f = (fa[j] + (fb[j] - fa[j]) * k) * ratio, b = (ba[j] + (bb[j] - ba[j]) * k) * ratio * bws;
        if (j == 0 && f < f0 * 1.0625f && f > 0.0f) { b *= f0 * 1.0625f / f; f = f0 * 1.0625f; }   /* sung high: F1 follows F0 */
        res_set(&r[j], f, b);
    }
    res_set(&r[3], 3300.0f * ratio, 250.0f * ratio * bws);
    /* the source: BUZZ (+ SHAPE, the velocity) -> the open quotient and the tilt */
    float buzz = gb_clampf((float)gb_ev(p, 4) + m->shape - 64.0f + (v->vel * 127.0f - 96.0f) * (40.0f / 256.0f), 0.0f, 127.0f);
    float oq = 0.9f - buzz * (0.55f / 127.0f), tilt = (2500.0f + buzz * 109.0f) * (1.0f / 16384.0f);
    float tp = oq * 0.65f, br = (float)gb_ev(p, 5);
    float gv = 1.0f - br * br * (2.0f / 32767.0f), gn = br * (1.0f / 127.0f);
    float og = (13000.0f - (float)(gb_ev(p, 6) - 64) * 50.0f) * (1.0f / 32768.0f) * gb_exp2f((60.0f - v->pitch) * (1.0f / 48.0f));
    float ogk = og * (1.0f + buzz * (1.0f / 16.0f)) * 0.25f;
    float ph = (float)v->s.voice.ph * (1.0f / 4294967296.0f), dt = m->dt, lp = v->s.voice.lp;
    float *y = v->s.voice.y;
    uint32_t nst = vnz;
    for (int i = 0; i < GB_BLOCK; i++) {
        float e;
        if (ph < tp) e = gb_sin_turn(0.5f * ph / tp) * 0.538f;        /* opening: + sin(pi u) x Tn / Tp */
        else if (ph < oq) e = -gb_sin_turn(0.25f * (ph - tp) / (oq - tp));
        else e = 0.0f;
        float d = ph - oq; d -= d < -0.5f ? -1.0f : 0.0f;              /* the closure step, band-limited */
        if (d >= 0.0f && d < dt) e += 0.5f * gb_blep(d, dt); else if (d < 0.0f && d > -dt) e += 0.5f * gb_blep(d + 1.0f, dt);
        lp += (e + GB_BIAS - lp) * tilt;
        float x = lp * gv;
        if (gn > 0.0f) { nst ^= nst << 13; nst ^= nst >> 17; nst ^= nst << 5; float nz = (float)(int32_t)nst * (gn / 2147483648.0f); x += ph < oq ? nz : 0.5f * nz; }
        float a = r[0].a * x + r[0].b * y[0] + r[0].c * y[1]; y[1] = y[0]; y[0] = a;
        a = r[1].a * a + r[1].b * y[2] + r[1].c * y[3]; y[3] = y[2]; y[2] = a;
        a = r[2].a * a + r[2].b * y[4] + r[2].c * y[5]; y[5] = y[4]; y[4] = a;
        a = r[3].a * a + r[3].b * y[6] + r[3].c * y[7]; y[7] = y[6]; y[6] = gb_clampf(a, -64.0f, 64.0f);
        float s = y[6] * og + (y[6] - y[7]) * ogk, as = gb_absf(s);
        if (as > 0.73f) { as = 0.73f + 0.27f * gb_softclip((as - 0.73f) * (1.0f / 0.27f)); s = s < 0.0f ? -as : as; }
        ph += dt; ph -= ph >= 1.0f ? 1.0f : 0.0f;
        out[i] = s;
    }
    vnz = nst;
    for (int i = 0; i < 8; i++) y[i] = gb_flush(y[i]);
    v->s.voice.lp = lp; v->s.voice.ph = (uint32_t)(ph * 4294967040.0f);
    return 1;
}

const gb_engine gb_eng_voice = { "VOICE", ED, 4, 0, vo_start, vo_render, 0, 0, 0 };
