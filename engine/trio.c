/* SPDX-License-Identifier: GPL-3.0-only
 * TRIO: SLOOP's three-oscillator chip voice (docs/sloop/SPEC-MOTEURS.md), written for this project. Three
 * oscillators (triangle, saw, pulse, pitched noise, saw AND triangle, pulse AND saw) in one of 16 sets with their
 * levels, a ring (osc 1's sign flipped by osc 3's top phase bit) and a hard sync (osc 2 restarts with osc 1), into a
 * multimode state-variable filter (LP BP HP notch) that saturates gently, never self-oscillates, and drifts a little.
 * The steps (saw, pulse, sync, ring) are band-limited (polyBLEP; the voice runs one sample late for it).
 *   E0 WAVE (the 16 sets)   E1 INT2  E2 INT3 (semitones)   E3 DTN (osc 2 up, osc 3 down, cents)
 *   E4 MODE LP BP HP NOT    E5 CUT   E6 RES                E7 PW (+ SHAPE) */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 15, 0 }, { -24, 24, 0 }, { -24, 24, -12 }, { 0, 50, 6 }, { 0, 3, 0 }, { 0, 127, 80 },
                             { 0, 127, 40 }, { 0, 127, 64 } };
enum { TW_OFF, TW_TRI, TW_SAW, TW_PLS, TW_NOI, TW_ST, TW_PS };
#define RING 1
#define SYNC 2
#define G3 (11000.0f / 32768.0f)
#define G2 (15000.0f / 32768.0f)
#define G1 (24000.0f / 32768.0f)
#define K(x) ((x) / 32768.0f)
static const struct { uint8_t w[3], flags; float g[3]; } SETS[16] = {
    { { TW_SAW, TW_SAW, TW_SAW }, 0, { G3, G3, G3 } },                                  /* SAW3 */
    { { TW_PLS, TW_PLS, TW_PLS }, 0, { G3, G3, G3 } },                                  /* PLS3 */
    { { TW_PLS, TW_PLS, TW_TRI }, 0, { G3, G3, G3 + K(3000) } },                        /* PPT */
    { { TW_SAW, TW_SAW, TW_TRI }, 0, { G3, G3, G3 + K(3000) } },                        /* SST */
    { { TW_TRI, TW_TRI, TW_TRI }, 0, { G3 + K(2000), G3 + K(2000), G3 + K(2000) } },    /* TRI3 */
    { { TW_ST, TW_ST, TW_TRI }, 0, { G3 + K(3000), G3 + K(3000), G3 } },                /* S&T */
    { { TW_PS, TW_PS, TW_TRI }, 0, { G3 + K(3000), G3 + K(3000), G3 } },                /* P&S */
    { { TW_PLS, TW_OFF, TW_NOI }, 0, { G2, 0, G2 - K(4000) } },                         /* P+N */
    { { TW_NOI, TW_OFF, TW_OFF }, 0, { G1, 0, 0 } },                                    /* NOIS */
    { { TW_SAW, TW_SAW, TW_OFF }, SYNC, { G2 - K(5000), G2 + K(5000), 0 } },            /* SYNC */
    { { TW_PLS, TW_PLS, TW_OFF }, SYNC, { G2 - K(5000), G2 + K(5000), 0 } },            /* SYNCP */
    { { TW_SAW, TW_SAW, TW_TRI }, SYNC, { G3 - K(3000), G3 + K(4000), G3 } },           /* SYNC3 */
    { { TW_TRI, TW_OFF, TW_OFF }, RING, { G1, 0, 0 } },                                 /* RING */
    { { TW_TRI, TW_OFF, TW_TRI }, RING, { G2 + K(2000), 0, G2 - K(3000) } },            /* RING3 */
    { { TW_TRI, TW_SAW, TW_OFF }, RING | SYNC, { G2 + K(2000), G2, 0 } },               /* R+S */
    { { TW_TRI, TW_PLS, TW_OFF }, RING | SYNC, { G2 + K(2000), G2, 0 } },               /* R+SP */
};
#define TURN 4294967296.0f
#define INV (1.0f / 4294967296.0f)
typedef struct { uint32_t pw; float pdc, psm; } pwq;
GB_INLINE float tri(uint32_t ph) { float x = (float)ph * INV; return x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x; }
GB_INLINE float saw(uint32_t ph) { return (float)ph * (2.0f * INV) - 1.0f; }
GB_INLINE float tri16(uint32_t ph) { return (float)((((ph & 0x80000000u) ? ~ph : ph) >> 15) & 0xFFFFu); }
GB_INLINE float st(uint32_t ph) { return ((float)((uint32_t)(ph >> 16) & (uint32_t)tri16(ph)) - 16384.0f) * (2.0f / 65536.0f); }
GB_INLINE float ps(uint32_t ph, const pwq *q) { return ((ph >= q->pw ? (float)(ph >> 16) : 0.0f) * (1.0f / 32768.0f)) - q->psm; }
static float naive(int w, uint32_t ph, const pwq *q)
{
    switch (w) {
    case TW_TRI: return tri(ph);
    case TW_SAW: return saw(ph);
    case TW_PLS: return (ph < q->pw ? 1.0f : -1.0f) - q->pdc;
    case TW_ST: return st(ph);
    case TW_PS: return ps(ph, q);
    default: return 0.0f;
    }
}
/* a step j (after minus before, at level g) that lies x (of a sample) before sample i: cr[i] the sample before
 * it, cr[i + 1] sample i (one sample late) */
GB_INLINE void blep(float j, float x, float g, float *cr) { float h = 0.5f * j * g, y = 1.0f - x; cr[0] += h * x * x; cr[1] -= h * y * y; }
GB_INLINE float frac(uint32_t past, uint32_t inc) { float x = inc ? (float)past / (float)inc : 0.0f; return x > 1.0f ? 1.0f : x; }

/* one oscillator over the block into mix (its steps into cr); wx: where it starts a cycle (SYNC's osc 1), else 0 */
static void pass(int w, uint32_t *php, uint32_t inc, float g, const pwq *q, float *nz, uint32_t *nst, float *mix, float *cr, float *wx)
{
    uint32_t ph = *php, o;
    float z = *nz;
    for (int i = 0; i < GB_BLOCK; i++) {
        o = ph; ph += inc;
        switch (w) {
        case TW_TRI: mix[i] += tri(ph) * g; break;
        case TW_SAW:
            if (ph < o) { float x = frac(ph, inc); blep(-2.0f, x, g, cr + i); if (wx) wx[i] = x; }
            mix[i] += saw(ph) * g; break;
        case TW_PLS:
            if (ph < o) { float x = frac(ph, inc); blep(2.0f, x, g, cr + i); if (wx) wx[i] = x; }
            if (ph - q->pw < o - q->pw) blep(-2.0f, frac(ph - q->pw, inc), g, cr + i);
            mix[i] += ((ph < q->pw ? 1.0f : -1.0f) - q->pdc) * g; break;
        case TW_NOI:
            if ((o ^ ph) & 0x08000000u) { uint32_t r = *nst; r ^= r << 13; r ^= r >> 17; r ^= r << 5; *nst = r; z = (float)(int32_t)r * (1.0f / 2147483648.0f); }
            mix[i] += z * g; break;
        case TW_ST: mix[i] += st(ph) * g; break;
        case TW_PS: mix[i] += ps(ph, q) * g; break;
        default: break;
        }
    }
    *php = ph; *nz = z;
}
/* osc 1 a triangle whose sign osc 3's top phase bit flips (osc 3 from p3): each flip a step of twice its value */
static void ring(uint32_t *php, uint32_t inc, float g, uint32_t p3, uint32_t inc3, float *mix, float *cr, float *wx)
{
    uint32_t ph = *php;
    for (int i = 0; i < GB_BLOCK; i++) {
        uint32_t o = ph, o3 = p3;
        ph += inc; p3 += inc3;
        if (wx && ph < o) wx[i] = frac(ph, inc);
        float s = tri(ph), sg = (p3 & 0x80000000u) ? -g : g;
        if ((o3 ^ p3) & 0x80000000u) blep(2.0f * s, frac(p3 & 0x7FFFFFFFu, inc3), sg, cr + i);
        mix[i] += s * sg;
    }
    *php = ph;
}
/* osc 2 (saw or pulse) restarted where osc 1 starts a cycle: x of its own cycle in; the jump one step */
static void hsync(int w, uint32_t *php, uint32_t inc, float g, const pwq *q, const float *wx, float *mix, float *cr)
{
    uint32_t ph = *php;
    for (int i = 0; i < GB_BLOCK; i++) {
        uint32_t o = ph;
        ph += inc;
        if (wx[i] >= 0.0f) {
            uint32_t back = (uint32_t)(wx[i] * (float)inc);
            blep(naive(w, 0, q) - naive(w, ph - back, q), wx[i], g, cr + i);
            ph = o = back;
        }
        if (w == TW_PLS) {
            if (ph < o) blep(2.0f, frac(ph, inc), g, cr + i);
            if (ph - q->pw < o - q->pw) blep(-2.0f, frac(ph - q->pw, inc), g, cr + i);
            mix[i] += ((ph < q->pw ? 1.0f : -1.0f) - q->pdc) * g;
        } else {
            if (ph < o) blep(-2.0f, frac(ph, inc), g, cr + i);
            mix[i] += saw(ph) * g;
        }
    }
    *php = ph;
}
GB_INLINE float knee(float y)
{
    float a = gb_absf(y);
    if (a <= 0.49f) return y;
    a = 0.49f + 0.5f * gb_softclip((a - 0.49f) * 2.0f);
    return y < 0.0f ? -a : a;
}

static void trio_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    v->s.trio.ph[0] = 0; v->s.trio.ph[1] = 0x1C000000u; v->s.trio.ph[2] = 0x38000000u;   /* (a little apart) */
    if (fresh) { v->s.trio.ic1 = v->s.trio.ic2 = 0.0f; v->s.trio.prev = 0.0f; v->s.trio.drift = 0.0f; v->s.trio.nst = v->seed; }
    v->s.trio.nz = 0.0f;
}
static void trio_spread(gb_voice *v, uint32_t r) { v->s.trio.ph[0] += r; v->s.trio.ph[1] += r; v->s.trio.ph[2] += r; }

GB_HOT static int trio_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    int set = gb_ev(p, 0), mode = gb_ev(p, 4), dtn = gb_ev(p, 3);
    int w1 = SETS[set].w[0], w2 = SETS[set].w[1], w3 = SETS[set].w[2], fl = SETS[set].flags;
    float g1 = SETS[set].g[0], g2 = SETS[set].g[1], g3 = SETS[set].g[2];
    uint32_t inc1 = (uint32_t)(m->dt * TURN);
    uint32_t inc2 = (uint32_t)(gb_minf(gb_note_hz(m->pitch + (float)gb_ev(p, 1) + (float)dtn * 0.01f) * GB_ISR, 0.45f) * TURN);
    uint32_t inc3 = (uint32_t)(gb_minf(gb_note_hz(m->pitch + (float)gb_ev(p, 2) - (float)dtn * 0.01f) * GB_ISR, 0.45f) * TURN);
    float duty = gb_clampf(((float)gb_ev(p, 7) + m->shape - 64.0f) * (1.0f / 128.0f), 0.0234f, 0.977f);
    pwq q = { (uint32_t)(duty * TURN), 2.0f * duty - 1.0f, 1.0f - duty * duty };
    q.psm = (1.0f - duty * duty);                                     /* (the mean of pulse AND saw, taken off) */
    float k = 2.0f - (float)gb_ev(p, 6) * (1.75f / 127.0f);           /* 2.0 .. 0.25: never self-oscillating */
    static const float MIX[4][3] = { { 0, 0, 1 }, { 0, 1.367f, 0 }, { 1, 0, -1 }, { 1, 0, 0 } };   /* in, band, low */
    float ma = MIX[mode][0], mb = mode >= 2 ? -k : MIX[mode][1], mc = MIX[mode][2];
    /* the cutoff wanders a little, a block at a time (the chips' unsteady filter) */
    uint32_t r = v->s.trio.nst; r ^= r << 13; r ^= r >> 17; r ^= r << 5; v->s.trio.nst = r;
    v->s.trio.drift += ((float)((int32_t)(r >> 22) - 512) * (1.0f / 256.0f) - v->s.trio.drift) * 0.125f;
    gb_svf f; gb_svf_set(&f, gb_cut_hz(gb_clampf((float)gb_ev(p, 5) + m->cut + v->s.trio.drift, 0.0f, 127.0f)), k);
    float mix[GB_BLOCK], cr[GB_BLOCK + 1], wx[GB_BLOCK];
    for (int i = 0; i < GB_BLOCK; i++) { mix[i] = cr[i] = 0.0f; wx[i] = -1.0f; }
    cr[GB_BLOCK] = 0.0f;
    uint32_t p3s = v->s.trio.ph[2];
    pass(w3, &v->s.trio.ph[2], inc3, g3, &q, &v->s.trio.nz, &v->s.trio.nst, mix, cr, 0);   /* first: the ring reads its start */
    if (fl & RING) ring(&v->s.trio.ph[0], inc1, g1, p3s, inc3, mix, cr, (fl & SYNC) ? wx : 0);
    else pass(w1, &v->s.trio.ph[0], inc1, g1, &q, &v->s.trio.nz, &v->s.trio.nst, mix, cr, (fl & SYNC) ? wx : 0);
    if (fl & SYNC) hsync(w2, &v->s.trio.ph[1], inc2, g2, &q, wx, mix, cr);
    else pass(w2, &v->s.trio.ph[1], inc2, g2, &q, &v->s.trio.nz, &v->s.trio.nst, mix, cr, 0);
    float cur = v->s.trio.prev, ic1 = v->s.trio.ic1, ic2 = v->s.trio.ic2, a1 = f.a1, a2 = f.a2, a3 = f.a3;
    for (int i = 0; i < GB_BLOCK; i++) {
        float x = knee(cur + cr[i]) * 0.5f;                            /* one sample late: its corrections complete */
        cur = mix[i];
        float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = gb_clampf(2.0f * v2 - ic2, -4.5f, 4.5f);
        float c = gb_clampf(ic1, -1.5f, 1.5f); ic1 = c + (ic1 - c) * 0.25f + GB_BIAS;   /* (3.1.8: a pulse's flat halves: va.c) */   /* the band's state compressed: gritty, not ringing */
        out[i] = knee(x * ma + v1 * mb + v2 * mc) * 2.0f * 0.5f;
    }
    v->s.trio.prev = cur + cr[GB_BLOCK]; v->s.trio.ic1 = ic1; v->s.trio.ic2 = gb_flush(ic2);
    return 1;
}

const gb_engine gb_eng_trio = { "TRIO", ED, 0, 0, trio_start, trio_render, 0, trio_spread, 0 };
