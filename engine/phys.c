/* SPDX-License-Identifier: GPL-3.0-only
 * PHYS: SLOOP's physical models (docs/sloop/SPEC-MOTEURS.md), written for this project in floating point after the
 * published designs SLOOP's models follow: the modal voice and the string voice of DaisySP (Electrosmith) and
 * Plaits, the sympathetic strings of Rings (Emilie Gillet; all MIT), with SLOOP's parameter curves.
 *   MODAL  12 band-pass modes struck by a filtered impulse (or bowed: random hits): STRC stretches the partials,
 *          BRIT keeps the high ones and opens the strike, DAMP the decay, POS the strike point
 *   STRNG  a Karplus-Strong string plucked by a noise burst a period long: STRC below 0.25 a curved bridge (a sitar),
 *          above a dispersion (stiff); BRIT, DAMP; POS the pluck point
 *   MEMB   a drum head: the modes of a circular membrane, ideal (inharmonic) to loaded (harmonic: HARM), POS from
 *          the centre to the rim, BEND the pitch's drop after the strike
 *   SYMP   the string with three sympathetic strings tuned by CHRD (a chord above the note, or ROOT), SYMP how much
 *          they take, BUZZ the curved bridge
 * ACC: the accent at full velocity; BOW: sustained by random hits; EXC: the exciter itself heard. Three voices a
 * track (SYMP two); their state lives in slots here (a string's lines are large), not in the voice. */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 3, 0 }, { 0, 127, 64 }, { 0, 127, 80 }, { 0, 127, 80 }, { 0, 127, 20 }, { 0, 127, 90 },
                             { 0, 127, 0 }, { 0, 127, 0 } };
enum { PM_MODAL, PM_STRING, PM_MEMB, PM_SYMP };
#define NMODE 12
#define LINE 512
#define LMASK (LINE - 1)
#define SLINE 128
#define SMASK (SLINE - 1)
#define YLINE 256
#define YMASK (YLINE - 1)
#define NSYMP 3
#define NSLOT 4
static const float GAIN[4] = { 56000.0f / 32768.0f, 48000.0f / 32768.0f, 135000.0f / 32768.0f, 36000.0f / 32768.0f };

typedef struct { float g, rpg, gh, a; } mode_cf;
typedef struct { float low, band; } csvf_st;
typedef struct { float freq, damp, drive; } csvf;
typedef struct {
    float s1[NMODE], s2[NMODE], e1, e2, benv;
    uint32_t rng;
} modal_t;
typedef struct {
    float line[LINE], sline[SLINE];
    uint32_t wp, swp, rng, nz_a, nz_b, bk, bn, bd;
    float disp, bridge, dcx, dcy, lp, ein, src, o0, o1;
    csvf_st ex; csvf exc;
} string_t;
typedef struct {
    string_t s;
    int16_t line[NSYMP][YLINE];                      /* the sympathetic strings, x 4096 */
    float dcx, dcy, yl, cin, h[NSYMP][3];
    uint32_t wp, d[NSYMP], on;
} symp_t;
typedef struct {
    uint32_t tok, last;                              /* the voice that owns it, the block it last sounded in */
    uint8_t model;
    union { modal_t m; string_t s; symp_t y; } u;
} slot_t;
static slot_t SL[NSLOT] GB_LOWRAM;                   /* (written before read: a slot is cleared when it is taken) */
static uint32_t next_tok = 1;

GB_INLINE uint32_t rnd(uint32_t *s) { uint32_t r = *s; r ^= r << 13; r ^= r >> 17; r ^= r << 5; *s = r; return r; }
GB_INLINE float urnd(uint32_t *s) { return (float)(rnd(s) >> 8) * (1.0f / 16777216.0f); }        /* 0..1 */
GB_INLINE float cosw(float t) { return gb_sin_turn(t + 0.25f); }
static float atan_(float x)                          /* atan, |x| any: an odd polynomial on 0..1, folded */
{
    float a = gb_absf(x), r = a > 1.0f ? 1.0f / a : a, r2 = r * r;
    float y = r * (0.99997726f + r2 * (-0.33262347f + r2 * (0.19354346f + r2 * (-0.11643287f + r2 * (0.05265332f - r2 * 0.01172120f)))));
    if (a > 1.0f) y = 1.5707963f - y;
    return x < 0.0f ? -y : y;
}
static float atan2t(float y, float x)                /* atan2 in turns */
{
    float a;
    if (x > 0.0f) a = atan_(y / x);
    else if (x < 0.0f) a = atan_(y / x) + (y >= 0.0f ? 3.14159265f : -3.14159265f);
    else a = y > 0.0f ? 1.5707963f : y < 0.0f ? -1.5707963f : 0.0f;
    return a * (1.0f / 6.2831853f);
}

/* ---------------------------------------------------------------- the slots */
static slot_t *slot_of(gb_voice *v) { slot_t *S = &SL[v->s.phys.slot % NSLOT]; return S->tok == v->s.phys.tok ? S : 0; }
static void slot_reset(slot_t *S, int md, uint32_t seed)
{
    memset(&S->u, 0, md == PM_SYMP ? sizeof(symp_t) : md == PM_STRING ? sizeof(string_t) : sizeof(modal_t));
    S->model = (uint8_t)md;
    seed |= 1u;
    if (md == PM_STRING || md == PM_SYMP) { S->u.s.rng = seed; S->u.s.src = 1.0f; }
    else S->u.m.rng = seed;
}
GB_SMALL static void phys_start(gb_voice *v, const gb_patch *p, int fresh)
{
    int md = gb_ev(p, 0);
    slot_t *S = fresh ? 0 : slot_of(v);
    if (!S) {                                        /* a slot: one not heard in the last two blocks, else the longest silent */
        int best = 0; uint32_t age = 0;
        for (int i = 0; i < NSLOT; i++) {
            uint32_t a = SL[i].tok ? gb_blocks - SL[i].last : 0xFFFFFFFFu;
            if (a > age) { age = a; best = i; }
        }
        S = &SL[best];
        S->tok = next_tok++; if (!next_tok) next_tok = 1;
        v->s.phys.slot = (uint8_t)best; v->s.phys.tok = S->tok;
        slot_reset(S, md, v->seed);
    } else if (S->model != md) slot_reset(S, md, v->seed);
    S->last = gb_blocks;
    v->s.phys.trig = 1;                              /* (played again: the body struck again, ringing) */
}

/* ---------------------------------------------------------------- the modal voice */
static float stiff_of(float s)                       /* STRC -> the partials' stretch */
{
    if (s < 0.25f) return -(0.25f - s) * 0.25f;
    if (s < 0.3f) return 0.0f;
    if (s < 0.9f) return (s - 0.3f) * (1.0f / 0.6f);
    s = (s - 0.9f) * 10.0f; s *= s;
    return 1.5f - 0.5f * cosw(0.5f * s);
}
typedef struct { mode_cf m[NMODE]; int n; float ex_g, ex_rpg, ex_gh, strike, dust_thr, dust_gain, dust_dc; } modal_blk;
static void svf_cf(float f, float q, float *g, float *rpg, float *gh)
{
    float gg = gb_tan_pi(gb_minf(f, 0.497f)), r = 1.0f / q, h = 1.0f / (1.0f + r * gg + gg * gg);
    *g = gg; *rpg = r + gg; *gh = gg * h;
}
GB_SMALL static void modal_exciter(modal_blk *B, modal_t *M, float f0, float brightness, float b, float d, float acc, float bow, int *trig)
{
    float range = bow > 0.0f ? 36.0f : 60.0f, f = (bow > 0.0f ? 4.0f : 2.0f) * f0;
    float cut = gb_clampf(f * gb_exp2f((b * (2.0f - b) - 0.5f) * range * (1.0f / 12.0f)), 0.00002f, 0.499f);
    svf_cf(cut, bow > 0.0f ? 0.7f : 1.5f, &B->ex_g, &B->ex_rpg, &B->ex_gh);
    B->strike = 0.0f; B->dust_thr = 0.0f;
    if (bow > 0.0f) {                                /* Dust: hits at 0.3 (5e-5 + b^4) */
        float dn = brightness * brightness; dn *= dn;
        float df = 0.00005f + 0.99995f * dn;
        B->dust_thr = 0.3f * df;
        B->dust_gain = (4.0f - 3.0f * df) * acc * bow * gb_exp2f(-3.3219f * d);
        B->dust_dc = 0.5f * B->dust_thr * B->dust_gain;
    } else if (*trig) {                              /* the strike: (0.12 + 0.08 a)(1 - d / 2) 2^(2 cut^2) / cut */
        B->strike = (0.12f + 0.08f * acc) * (1.0f - 0.5f * d) * gb_exp2f(2.0f * cut * cut) / cut;
        *trig = 0;
    }
    (void)M;
}
static float modal_q(float structure, float b, float d, float *ql)
{
    float bb = b * (1.0f - 0.3f * structure) * (1.0f - 0.3f * d);
    *ql = bb * (2.0f - bb) * 0.85f + 0.15f;
    return 500.0f * gb_exp2f(d * 13.2833f);
}
static void modal_block(modal_blk *B, modal_t *M, float f0, float structure, float brightness, float damping, float acc,
                        float pos, float bow, int *trig)
{
    float b = brightness + 0.25f * acc * (1.0f - brightness), d = damping + 0.25f * acc * (1.0f - damping);
    modal_exciter(B, M, gb_minf(f0, 0.25f), brightness, b, d, acc, bow, trig);
    float st = stiff_of(structure), ql;
    f0 /= 1.0f + st + st * (st < 0.0f ? 0.93f : 0.98f);  /* the third partial where it would be */
    float q = modal_q(structure, b, d, &ql), stretch = 1.0f, st2 = st, harm = f0;
    int i;
    for (i = 0; i < NMODE; i++) {
        float mf = harm * stretch;
        if (mf >= 0.499f) break;
        mode_cf *c = &B->m[i];
        svf_cf(mf, 1.0f + mf * q, &c->g, &c->rpg, &c->gh);
        c->a = cosw(pos * (float)i) * 0.25f * (1.0f - 2.0f * mf);
        stretch += st2; st2 *= st2 < 0.0f ? 0.93f : 0.98f;
        harm += f0; q *= ql;
    }
    B->n = i;
    for (; i < NMODE; i++) M->s1[i] = M->s2[i] = 0.0f;
}
/* MEMB: the modes of a circular membrane (zeros of the Bessel functions), ideal to loaded (Raman, 1934) */
static const float MEMB_IDEAL[10] = { 1.0f, 1.5933f, 2.1355f, 2.2954f, 2.6531f, 2.9173f, 3.1555f, 3.5001f, 3.5984f, 3.6475f };
static const float MEMB_LOADED[10] = { 1.0f, 2.0f, 3.0f, 3.01f, 4.0f, 3.99f, 5.0f, 5.02f, 4.98f, 6.0f };
static const uint8_t MEMB_M[10] = { 0, 1, 2, 0, 3, 1, 4, 2, 0, 5 };    /* their nodal diameters */
static void memb_block(modal_blk *B, modal_t *M, float f0, float harm, float brightness, float damping, float acc, float pos,
                       float bend, int *trig)
{
    float b = brightness + 0.25f * acc * (1.0f - brightness), d = damping + 0.25f * acc * (1.0f - damping);
    if (*trig) M->benv = 1.0f;
    float f = f0;
    if (M->benv > 0.0f) {                            /* the drop after the strike, ~60 ms */
        f *= gb_exp2f(bend * M->benv * (1.0f / 12.0f));
        M->benv *= 0.9604f; if (M->benv < 0.001f) M->benv = 0.0f;
    }
    f = gb_minf(f, 0.25f);
    modal_exciter(B, M, f, brightness, b, d, acc, 0.0f, trig);
    float ql, q = modal_q(0.0f, b, d, &ql), w[6];
    w[0] = 1.0f - 0.6f * pos; w[1] = gb_minf(1.5f * pos, 1.0f);
    for (int k = 2; k < 6; k++) w[k] = w[k - 1] * w[1];
    int i;
    for (i = 0; i < 10; i++) {
        float r = MEMB_IDEAL[i] + (MEMB_LOADED[i] - MEMB_IDEAL[i]) * harm, mf = f * r;
        if (mf >= 0.499f) break;
        mode_cf *c = &B->m[i];
        svf_cf(mf, 1.0f + mf * q, &c->g, &c->rpg, &c->gh);
        c->a = w[MEMB_M[i]] * 0.25f * (1.0f - 2.0f * mf) / (r * r);
        q *= ql;
    }
    B->n = i;
    for (; i < NMODE; i++) M->s1[i] = M->s2[i] = 0.0f;
}
static void modal_run(const modal_blk *B, modal_t *M, float *out, float *aux)
{
    float x[GB_BLOCK];
    float e1 = M->e1, e2 = M->e2;
    uint32_t rs = M->rng;
    for (int i = 0; i < GB_BLOCK; i++) {             /* the exciter: an impulse or Dust through a low-pass */
        float in = 0.0f;
        if (B->dust_thr > 0.0f) { float u = urnd(&rs); if (u < B->dust_thr) in = u / B->dust_thr * B->dust_gain; }
        else if (!i) in = B->strike;
        float t = B->ex_gh * (in - B->ex_rpg * e1 - e2), bp = e1 + t;
        e1 = bp + t; t = B->ex_g * bp; float lp = e2 + t; e2 = lp + t;
        aux[i] = lp - (B->dust_thr > 0.0f ? B->dust_dc : 0.0f);
        x[i] = lp;
    }
    if (gb_absf(e1) < 1e-7f && gb_absf(e2) < 1e-7f) e1 = e2 = 0.0f;
    M->e1 = e1; M->e2 = e2; M->rng = rs;
    for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f;
    for (int k = 0; k < B->n; k++) {                 /* the modes, band-pass, a mode at a time */
        const mode_cf *c = &B->m[k];
        float s1 = M->s1[k], s2 = M->s2[k], g = c->g, rpg = c->rpg, gh = c->gh, a = c->a;
        for (int i = 0; i < GB_BLOCK; i++) {
            float t = gh * (x[i] - rpg * s1 - s2), v = s1 + t;
            s1 = v + t; t = g * v; s2 = s2 + t + t;
            out[i] += a * v;
        }
        M->s1[k] = gb_flush(gb_clampf(s1, -64.0f, 64.0f)); M->s2[k] = gb_flush(gb_clampf(s2, -64.0f, 64.0f));
    }
}

/* ---------------------------------------------------------------- the string */
typedef struct { float src_ratio, dl, G, ap, mn, ap_gain, noise_amt, noise_flt, curving, dust_thr, dust_gain, dust_dc; int curved; } string_blk;
static void csvf_set(csvf *c, float f, float res, float drive)
{
    f = gb_minf(f, 0.3333f);
    float fr = 2.0f * gb_sin_turn(0.5f * gb_minf(0.25f, 0.5f * f));
    float r4 = gb_sqrtf(gb_sqrtf(res)), d1 = 2.0f * (1.0f - r4), d2 = fr > 0.0f ? gb_minf(2.0f, 2.0f / fr - 0.5f * fr) : 2.0f;
    c->freq = fr; c->damp = gb_minf(d1, d2); c->drive = drive * res;
}
GB_INLINE void csvf_pass(const csvf *c, csvf_st *s, float in)
{
    float b = s->band, notch = in - c->damp * b;
    s->low += c->freq * b;
    float high = notch - s->low;
    s->band = b + c->freq * high - c->drive * b * b * b;
}
GB_SMALL static void string_block(string_blk *B, string_t *S, float f0, float structure, float brightness, float damping, float acc,
                         float bow, float pos, int *trig)
{
    float b = brightness + 0.25f * acc * (1.0f - brightness), d = damping + 0.25f * acc * (1.0f - damping);
    float f = gb_clampf(f0, 0.0000153f, 0.25f);
    /* the non-linearity: below ~0.25 a curved bridge, above a dispersion */
    float nl = structure < 0.24f ? (structure - 0.24f) * 4.166f : structure > 0.26f ? (structure - 0.26f) * 1.351f : 0.0f;
    nl = gb_clampf(nl, -1.0f, 1.0f);
    if (*trig || bow > 0.0f) {                       /* the exciter's low-pass (and, struck, a burst a period long) */
        csvf_set(&S->exc, gb_minf(4.0f * f * gb_exp2f((b * (2.0f - b) - 0.5f) * 6.0f), 0.499f), bow > 0.0f ? 1.0f : 0.5f, 0.5f);
        if (*trig) {
            float per = 1.0f / f;
            S->bk = 0; S->bn = (uint32_t)per; S->bd = (uint32_t)(per * pos * 0.5f);
            S->nz_a = S->nz_b = rnd(&S->rng) | 1u;
            *trig = 0;
        }
    }
    B->dust_thr = 0.0f;
    if (bow > 0.0f) {
        float dn = brightness * brightness; dn *= dn;
        float df = 0.00005f + 0.99995f * dn;
        B->dust_thr = 0.3f * df;
        B->dust_gain = (8.0f - 6.0f * df) * acc * bow * gb_exp2f(-3.3219f * d);
        B->dust_dc = 0.5f * B->dust_thr * B->dust_gain;
    }
    B->curved = nl < 0.0f; nl = gb_absf(nl);
    float delay = gb_clampf(1.0f / f, 4.0f, (float)(LINE - 4));
    float ratio = delay * f;                         /* below its lowest pitch the string runs resampled */
    B->src_ratio = ratio >= 0.9999f ? 1.0f : ratio;
    if (B->src_ratio >= 1.0f) S->src = 1.0f;
    float dcut = gb_minf(12.0f + 60.0f * d * d + 24.0f * b, 84.0f), df = gb_minf(f * gb_exp2f(dcut * (1.0f / 12.0f)), 0.499f);
    if (d >= 0.95f) {                                /* towards an endless decay */
        float ti = (d - 0.95f) * 20.0f;
        b += ti * (1.0f - b); df += (0.4999f - df) * ti;
    }
    if (B->src_ratio < 1.0f) df = gb_minf(df / B->src_ratio, 0.497f);
    df = gb_minf(df, 0.497f);
    float g = gb_tan_pi(df);
    B->G = g / (1.0f + g);
    float sp = nl * (2.0f - nl) * 0.225f, na = nl > 0.75f ? (nl - 0.75f) * 4.0f : 0.0f;
    B->noise_amt = na * na * 0.1f; B->noise_flt = 0.06f + 0.94f * b * b;
    B->curving = nl * nl * 0.01f; B->ap_gain = -0.618f * nl / (0.15f + nl);
    /* in tune: the loop's delays and its filters' phase lags at the fundamental add up to one period */
    float ph = 1.0f / delay;                         /* the fundamental, turns a string step */
    float lag = atan2t(gb_sin_turn(0.5f * ph) * cosw(0.5f * df), cosw(0.5f * ph) * gb_sin_turn(0.5f * df))   /* the one-pole */
              - (0.25f - 0.5f * ph - atan2t(0.99977f * gb_sin_turn(ph), 1.0f - 0.99977f * cosw(ph)));      /* the DC blocker's lead */
    float ap = delay * sp, mn = 0.0f;
    B->dl = delay - lag * delay;
    if (ap >= 4.0f) {
        float psi = (float)(int)ap * ph;
        float lap = psi + 2.0f * atan2t(B->ap_gain * gb_sin_turn(psi), 1.0f - B->ap_gain * cosw(psi));
        mn = delay - (lag + lap) * delay;
        float a = mn - (float)(int)mn;               /* the linear read's own lag */
        lap = atan2t(a * gb_sin_turn(ph), 1.0f - a + a * cosw(ph)) - a * ph;
        mn -= lap * delay;
    }
    B->ap = ap; B->mn = mn;
}
GB_INLINE float hermite(const float *l, uint32_t wp, float delay)
{
    uint32_t t = wp + (uint32_t)delay;
    float f = delay - (float)(int)delay;
    float xm1 = l[(t - 1u) & LMASK], x0 = l[t & LMASK], x1 = l[(t + 1u) & LMASK], x2 = l[(t + 2u) & LMASK];
    float c = (x1 - xm1) * 0.5f, v = x0 - x1, w = c + v, a = w + v + (x2 - x0) * 0.5f, bn = w + a;
    return ((a * f - bn) * f + c) * f + x0;
}
GB_SMALL static void string_excite(const string_blk *B, string_t *S, float *aux)
{
    uint32_t bk = S->bk, bend = S->bn + S->bd;
    csvf_st ex = S->ex;
    for (int i = 0; i < GB_BLOCK; i++) {
        float x = 0.0f, in = 0.0f;
        if (B->dust_thr > 0.0f) { float u = urnd(&S->rng); if (u < B->dust_thr) x = u / B->dust_thr * B->dust_gain; }
        else if (bk < bend) {                        /* the burst; with a pluck point, minus itself pos / 2 periods on */
            if (bk < S->bn) x = S->bd ? urnd(&S->nz_a) - 0.5f : 2.0f * urnd(&S->nz_a) - 1.0f;
            if (S->bd && bk >= S->bd) x -= urnd(&S->nz_b) - 0.5f;
            bk++;
        }
        if (x != 0.0f || ex.low != 0.0f || ex.band != 0.0f) {
            csvf_pass(&S->exc, &ex, x); float l0 = ex.low;
            csvf_pass(&S->exc, &ex, x);
            in = 0.5f * (l0 + ex.low);
            ex.band = gb_clampf(ex.band, -5.0f, 5.0f); ex.low = gb_clampf(ex.low, -16.0f, 16.0f);
            if (x == 0.0f && gb_absf(ex.low) < 1e-7f && gb_absf(ex.band) < 1e-7f) ex.low = ex.band = 0.0f;
        }
        aux[i] = in - (B->dust_thr > 0.0f ? B->dust_dc : 0.0f);
    }
    S->bk = bk; S->ex = ex;
}
static void string_run(const string_blk *B, string_t *S, const float *in, float *out)
{
    uint32_t wp = S->wp, swp = S->swp, rs = S->rng;
    float src = S->src, o0 = S->o0, o1 = S->o1, lp = S->lp, disp = S->disp, bridge = S->bridge, dcx = S->dcx, dcy = S->dcy, ein = S->ein;
    for (int i = 0; i < GB_BLOCK; i++) {
        ein += in[i];
        src += B->src_ratio;
        if (src > 1.0f) {
            float s;
            src -= 1.0f;
            if (!B->curved) {
                float nz = urnd(&rs) - 0.5f;
                disp += B->noise_flt * (nz - disp);
                float q = 1.0f + disp * B->noise_amt, ap = B->ap * q, mn = B->mn * q;
                if (ap >= 4.0f && mn >= 4.0f) {      /* the dispersion: a line, then an all-pass */
                    uint32_t t = wp + (uint32_t)mn;
                    float a = S->line[t & LMASK], bb = S->line[(t + 1u) & LMASK];
                    s = a + (bb - a) * (mn - (float)(int)mn);
                    float rd = S->sline[(swp + (uint32_t)ap) & SMASK], w = s + B->ap_gain * rd;
                    S->sline[swp] = w; swp = (swp - 1u) & SMASK;
                    s = rd - w * B->ap_gain;
                } else s = hermite(S->line, wp, B->dl * q);
            } else {                                 /* the curved bridge: the delay shortened as it hits */
                s = hermite(S->line, wp, B->dl * (1.0f - bridge * B->curving));
                float as = gb_absf(s), val = as - 0.025f;
                bridge = val > 0.0f ? (s > 0.0f ? 2.0f * val : -3.0f * val) : 0.0f;
            }
            s = gb_clampf(s + ein, -20.0f, 20.0f); ein = 0.0f;
            float y = s - dcx + 0.99977f * dcy; dcx = s; dcy = y; s = y;   /* the DC blocker */
            float v = B->G * (s - lp); s = lp + v; lp = s + v;            /* the one-pole low-pass */
            S->line[wp] = s; wp = (wp - 1u) & LMASK;
            o1 = o0; o0 = s;
        }
        out[i] = o1 + (o0 - o1) * src;
    }
    S->wp = wp; S->swp = swp; S->rng = rs; S->src = src; S->o0 = o0; S->o1 = o1;
    S->lp = gb_flush(lp); S->disp = gb_flush(disp); S->bridge = gb_flush(bridge); S->dcx = gb_flush(dcx); S->dcy = gb_flush(dcy);
    S->ein = gb_clampf(ein, -20.0f, 20.0f);
}

/* ---------------------------------------------------------------- the sympathetic strings (half rate) */
static void symp_block(symp_t *Y, const float *f, float brightness, float damping, float amount)
{
    float b = brightness * (2.0f - brightness);
    b = b * (2.0f - b); b = 0.5f + 0.5f * b;          /* the loop's FIR: 0.5 .. 1 */
    float dd = 0.25f + 0.72f * damping, lfd = dd * (2.0f - dd), k8 = gb_exp2f(-8.0f * lfd);
    Y->cin = amount * (1.0f / 16.0f);
    Y->on = 0;
    for (int k = 0; k < NSYMP; k++) {
        if (f[k] < 0.0000153f) continue;
        float p = f[k] >= 0.25f ? 0.0f : 0.5f / f[k];   /* the period, half-rate steps */
        while (p > (float)(YLINE - 4)) p *= 0.5f;     /* folded up an octave */
        p -= 1.0f - b;
        if (p < 2.0f) continue;
        float c = gb_minf(gb_exp2f(-p * (10.0f / 1543.5f) * k8), 0.9999f);   /* RT60 0.07 s / k8 */
        float fr = p - (float)(int)p;
        Y->d[k] = (uint32_t)p;
        Y->h[k][0] = c * b * (1.0f - fr); Y->h[k][1] = c * (b * fr + (1.0f - b) * (1.0f - fr)); Y->h[k][2] = c * (1.0f - b) * fr;
        Y->on |= 1u << k;
    }
}
static void symp_run(symp_t *Y, float *io)
{
    float x[GB_BLOCK / 2], acc[GB_BLOCK / 2];
    float dcx = Y->dcx, dcy = Y->dcy, yl = Y->yl;
    for (int i = 0; i < GB_BLOCK / 2; i++) {          /* their input: the string, a pair summed, less its DC */
        float v = (io[2 * i] + io[2 * i + 1]) * Y->cin;
        dcy = v - dcx + 0.99709f * dcy; dcx = v;
        x[i] = dcy; acc[i] = 0.0f;
    }
    for (int k = 0; k < NSYMP; k++) {
        if (!((Y->on >> k) & 1u)) continue;
        int16_t *l = Y->line[k];
        uint32_t wp = Y->wp, d = Y->d[k];
        float h0 = Y->h[k][0] * (1.0f / 4096.0f), h1 = Y->h[k][1] * (1.0f / 4096.0f), h2 = Y->h[k][2] * (1.0f / 4096.0f);
        for (int i = 0; i < GB_BLOCK / 2; i++) {
            uint32_t t = wp + d;
            float y = gb_clampf(h0 * (float)l[t & YMASK] + h1 * (float)l[(t + 1u) & YMASK] + h2 * (float)l[(t + 2u) & YMASK] + x[i], -7.99f, 7.99f);
            l[wp] = (int16_t)(y * 4096.0f);
            acc[i] += y;
            wp = (wp - 1u) & YMASK;
        }
    }
    for (int i = 0; i < GB_BLOCK / 2; i++) { io[2 * i] += 0.5f * (yl + acc[i]); io[2 * i + 1] += acc[i]; yl = acc[i]; }
    Y->wp = (Y->wp - GB_BLOCK / 2) & YMASK;
    Y->yl = gb_flush(yl); Y->dcx = gb_flush(dcx); Y->dcy = gb_flush(dcy);
}
/* SYMP: the sympathetic strings above the note (cents), or ROOT: C3's octave of the key, its fifth and octave */
static const int16_t CHORDS[7][NSYMP] = { { -1200, 1200, 2400 }, { 700, 1200, 1900 }, { 500, 1200, 1700 }, { 400, 700, 1200 },
                                          { 300, 700, 1200 }, { 200, 700, 1400 }, { 400, 700, 1000 } };

GB_HOT static int phys_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    slot_t *S = slot_of(v);
    if (!S) { for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f; return 0; }   /* (its slot went to another voice) */
    int md = gb_ev(p, 0), trig = v->s.phys.trig;
    if (S->model != md) slot_reset(S, md, v->seed);
    S->last = gb_blocks;
    v->s.phys.trig = 0;
    float f0 = m->dt, y[GB_BLOCK], ax[GB_BLOCK];
    float strc = (float)gb_ev(p, 1) * (1.0f / 127.0f), brit = gb_clampf((float)gb_ev(p, 2) * (1.0f / 127.0f) + m->cut * (1.0f / 128.0f), 0.0f, 1.0f);
    float damp = (float)gb_ev(p, 3) * (1.0f / 127.0f), pos = (float)gb_ev(p, 4), exc = (float)gb_ev(p, 7) * (1.0f / 127.0f);
    float bow = (float)gb_ev(p, 6) * (0.4f / 127.0f), acc = gb_clampf((float)gb_ev(p, 5) * (1.0f / 127.0f) * v->vel, 0.0f, 1.0f);
    switch (md) {
    case PM_MODAL: { modal_blk B; modal_block(&B, &S->u.m, f0, strc, brit, damp, acc, pos * (0.25f / 127.0f), bow, &trig); modal_run(&B, &S->u.m, y, ax); break; }
    case PM_MEMB: { modal_blk B; memb_block(&B, &S->u.m, f0, strc, brit, damp, acc, pos * (1.0f / 127.0f), (float)gb_ev(p, 6) * (12.0f / 127.0f), &trig); modal_run(&B, &S->u.m, y, ax); break; }
    case PM_STRING: { string_blk B; string_block(&B, &S->u.s, f0, strc, brit, damp, acc, bow, pos * (1.0f / 127.0f), &trig); string_excite(&B, &S->u.s, ax); string_run(&B, &S->u.s, ax, y); break; }
    default: {                                       /* SYMP */
        string_blk B; float f[NSYMP];
        int c = gb_ev(p, 1) >> 4;
        for (int k = 0; k < NSYMP; k++) {
            static const int8_t DT[NSYMP] = { 1, -1, 0 };   /* (1/16 semitone: the strings beat a little) */
            float n = c < 7 ? m->pitch + (float)CHORDS[c][k] * 0.01f : 48.0f + (float)(k == 1 ? 7 : k == 2 ? 12 : 0);
            f[k] = gb_minf(gb_note_hz(n + (float)DT[k] * 0.0625f) * GB_ISR, 0.45f);
        }
        string_block(&B, &S->u.y.s, f0, 0.25f - (float)gb_ev(p, 6) * (0.25f / 127.0f), brit, damp, acc, 0.0f, 0.0f, &trig);
        string_excite(&B, &S->u.y.s, ax); string_run(&B, &S->u.y.s, ax, y);
        symp_block(&S->u.y, f, brit, damp, pos * (1.0f / 127.0f));
        symp_run(&S->u.y, y);
        break; }
    }
    float g = GAIN[md];
    for (int i = 0; i < GB_BLOCK; i++) {
        float s = (y[i] + ax[i] * exc) * g, a = gb_absf(s);
        if (a > 0.73f) { a = 0.73f + 0.5f * gb_softclip(gb_minf(a - 0.73f, 32.0f) * 2.0f); s = s < 0.0f ? -a : a; }
        out[i] = s;
    }
    return 1;
}

const gb_engine gb_eng_phys = { "PHYS", ED, 3, 0, phys_start, phys_render, 0, 0, 0 };
