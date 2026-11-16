/* SPDX-License-Identifier: GPL-3.0-only
 * FM6: SLOOP's six-operator FM (docs/sloop/SPEC-MOTEURS.md), written for this project after the classic design (the
 * 32 algorithms, each operator's four-rate / four-level envelope in the log domain, keyboard level and rate scaling,
 * velocity, ratio or fixed frequency, detune, feedback, the pitch envelope, the LFO). The patch is the sound
 * (fm6.h: F1..F8, SLOOP's own); SLOOP's eight values are macros over it:
 *   E0 ALG  PAT (the patch's) or 1..32     E2 MLVL the modulators' levels, +-36 dB   E4 MEG  their envelopes slower / faster
 *   E1 FB   added to the feedback          E3 MRAT added to their coarse ratio       E5 VMOD added to their velocity
 *   E6 DTUN the carriers spread apart      E7 PTCH the patch
 * The operators' envelopes are the voice's (ownenv: the ADSR does nothing). The track's FLT moves MLVL, SHP the
 * feedback, PIT the pitch. */
#include "engine.h"
#include "fm6.h"

static const gb_pd ED[8] = { { 0, 32, 0 }, { -7, 7, 0 }, { -64, 63, 0 }, { -16, 16, 0 }, { -64, 63, 0 }, { -7, 7, 0 },
                             { 0, 127, 0 }, { 0, FM6_NFACTORY + FM6_NBANK - 1, 0 } };

/* the 32 algorithms: for each operator (OP1..OP6), the operators that modulate it (bits), the carriers, the
 * operator with the feedback and the one it comes from (the same, or a loop: 4 into 6, 5 into 6) */
typedef struct { uint8_t mod[6], car, fbo, fbs; } alg_t;
#define B(o) (1u << ((o) - 1))
static const alg_t ALGS[32] = {
    { { B(2), 0, B(4), B(5), B(6), 0 }, B(1) | B(3), 6, 6 },                               /* 1 */
    { { B(2), 0, B(4), B(5), B(6), 0 }, B(1) | B(3), 2, 2 },                               /* 2 */
    { { B(2), B(3), 0, B(5), B(6), 0 }, B(1) | B(4), 6, 6 },                               /* 3 */
    { { B(2), B(3), 0, B(5), B(6), 0 }, B(1) | B(4), 6, 4 },                               /* 4 */
    { { B(2), 0, B(4), 0, B(6), 0 }, B(1) | B(3) | B(5), 6, 6 },                           /* 5 */
    { { B(2), 0, B(4), 0, B(6), 0 }, B(1) | B(3) | B(5), 6, 5 },                           /* 6 */
    { { B(2), 0, B(4) | B(5), 0, B(6), 0 }, B(1) | B(3), 6, 6 },                           /* 7 */
    { { B(2), 0, B(4) | B(5), 0, B(6), 0 }, B(1) | B(3), 4, 4 },                           /* 8 */
    { { B(2), 0, B(4) | B(5), 0, B(6), 0 }, B(1) | B(3), 2, 2 },                           /* 9 */
    { { B(2), B(3), 0, B(5) | B(6), 0, 0 }, B(1) | B(4), 3, 3 },                           /* 10 */
    { { B(2), B(3), 0, B(5) | B(6), 0, 0 }, B(1) | B(4), 6, 6 },                           /* 11 */
    { { B(2), 0, B(4) | B(5) | B(6), 0, 0, 0 }, B(1) | B(3), 2, 2 },                       /* 12 */
    { { B(2), 0, B(4) | B(5) | B(6), 0, 0, 0 }, B(1) | B(3), 6, 6 },                       /* 13 */
    { { B(2), 0, B(4), B(5) | B(6), 0, 0 }, B(1) | B(3), 6, 6 },                           /* 14 */
    { { B(2), 0, B(4), B(5) | B(6), 0, 0 }, B(1) | B(3), 2, 2 },                           /* 15 */
    { { B(2) | B(3) | B(5), 0, B(4), 0, B(6), 0 }, B(1), 6, 6 },                           /* 16 */
    { { B(2) | B(3) | B(5), 0, B(4), 0, B(6), 0 }, B(1), 2, 2 },                           /* 17 */
    { { B(2) | B(3) | B(4), 0, 0, B(5), B(6), 0 }, B(1), 3, 3 },                           /* 18 */
    { { B(2), B(3), 0, B(6), B(6), 0 }, B(1) | B(4) | B(5), 6, 6 },                        /* 19 */
    { { B(3), B(3), 0, B(5) | B(6), 0, 0 }, B(1) | B(2) | B(4), 3, 3 },                    /* 20 */
    { { B(3), B(3), 0, B(6), B(6), 0 }, B(1) | B(2) | B(4) | B(5), 3, 3 },                 /* 21 */
    { { B(2), 0, B(6), B(6), B(6), 0 }, B(1) | B(3) | B(4) | B(5), 6, 6 },                 /* 22 */
    { { 0, B(3), 0, B(6), B(6), 0 }, B(1) | B(2) | B(4) | B(5), 6, 6 },                    /* 23 */
    { { 0, 0, B(6), B(6), B(6), 0 }, B(1) | B(2) | B(3) | B(4) | B(5), 6, 6 },             /* 24 */
    { { 0, 0, 0, B(6), B(6), 0 }, B(1) | B(2) | B(3) | B(4) | B(5), 6, 6 },                /* 25 */
    { { 0, B(3), 0, B(5) | B(6), 0, 0 }, B(1) | B(2) | B(4), 6, 6 },                       /* 26 */
    { { 0, B(3), 0, B(5) | B(6), 0, 0 }, B(1) | B(2) | B(4), 3, 3 },                       /* 27 */
    { { B(2), 0, B(4), B(5), 0, 0 }, B(1) | B(3) | B(6), 5, 5 },                           /* 28 */
    { { 0, 0, B(4), 0, B(6), 0 }, B(1) | B(2) | B(3) | B(5), 6, 6 },                       /* 29 */
    { { 0, 0, B(4), B(5), 0, 0 }, B(1) | B(2) | B(3) | B(6), 5, 5 },                       /* 30 */
    { { 0, 0, 0, 0, B(6), 0 }, B(1) | B(2) | B(3) | B(4) | B(5), 6, 6 },                   /* 31 */
    { { 0, 0, 0, 0, 0, 0 }, 0x3F, 6, 6 },                                                  /* 32 */
};

/* the classic curves: an output / envelope level 0..99 on its log scale; the velocity's; the keyboard scaling's */
static int scale_level(int l) { static const uint8_t LUT[20] = { 0, 5, 9, 13, 17, 20, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 42, 43, 45, 46 }; return l >= 20 ? 28 + l : LUT[l < 0 ? 0 : l]; }
static int vel_scale(int vel, int sens)
{
    static const uint8_t VD[64] = { 0, 70, 86, 97, 106, 114, 121, 126, 132, 138, 142, 148, 152, 156, 160, 163, 166, 170, 173, 174, 178,
        181, 184, 186, 189, 190, 194, 196, 198, 200, 202, 205, 206, 209, 211, 214, 216, 218, 220, 222, 224, 225, 227, 229, 230, 232, 233,
        235, 237, 238, 240, 241, 242, 243, 244, 246, 246, 248, 249, 250, 251, 252, 253, 254 };
    int v = VD[(vel < 0 ? 0 : vel > 127 ? 127 : vel) >> 1] - 239;
    return ((sens * v + 7) >> 3) * 16;              /* (v < 0: a multiply, not a shift) */
}
static int scale_curve(int group, int depth, int curve)
{
    static const uint8_t EXPS[33] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 14, 16, 19, 23, 27, 33, 39, 47, 56, 66, 80, 94, 110, 126, 142, 158,
                                      174, 190, 206, 222, 238, 250 };
    int s = curve == 0 || curve == 3 ? (group * depth * 329) >> 12 : (EXPS[group > 32 ? 32 : group] * depth * 329) >> 15;
    return curve < 2 ? -s : s;
}
static int scale_kbd(int note, const fm6_op *o)
{
    int off = note - o->bp - 17;
    return off >= 0 ? scale_curve((off + 1) / 3, o->rd, o->rc) : scale_curve(-(off - 1) / 3, o->ld, o->lc);
}
static int scale_rate(int note, int sens) { int x = note / 3 - 7; x = x < 0 ? 0 : x > 31 ? 31 : x; return (sens * x) >> 3; }

/* a track's patch, through the macros (made once a block when anything changed: one core) */
typedef struct { fm6_patch p; float dt[6]; int8_t e[8]; uint8_t ok, alg; int8_t uid; } eff_t;
/* the bank's voices in use (B1..B27), a track each */
fm6_patch fm6_user[GB_NTRACK];
volatile int8_t fm6_user_id[GB_NTRACK] = { -1, -1, -1, -1, -1, -1, -1, -1 };
/* the classic packed voice: six operators of 17 bytes (OP6 first), then the pitch envelope, the algorithm, the
 * feedback and key sync, the LFO, the transpose, the name */
GB_SMALL void fm6_user_set(int t, int ptch, const uint8_t *m)
{
    fm6_patch *p = &fm6_user[t];
    fm6_user_id[t] = -1;                                   /* (the sound uses the init voice meanwhile) */
    for (int k = 0; k < 6; k++) {
        const uint8_t *b = m + 17 * (5 - k);
        fm6_op *o = &p->op[k];
        for (int j = 0; j < 4; j++) { o->r[j] = b[j] > 99 ? 99 : b[j]; o->l[j] = b[4 + j] > 99 ? 99 : b[4 + j]; }
        o->bp = b[8] > 99 ? 99 : b[8]; o->ld = b[9] > 99 ? 99 : b[9]; o->rd = b[10] > 99 ? 99 : b[10];
        o->lc = b[11] & 3; o->rc = (b[11] >> 2) & 3; o->rs = b[12] & 7; o->det = (b[12] >> 3) & 15;
        o->ams = b[13] & 3; o->kvs = (b[13] >> 2) & 7; o->ol = b[14] > 99 ? 99 : b[14];
        o->mode = b[15] & 1; o->fc = (b[15] >> 1) & 31; o->ff = b[16] > 99 ? 99 : b[16];
        if (o->det > 14) o->det = 7;
    }
    for (int j = 0; j < 4; j++) { p->pr[j] = m[102 + j] > 99 ? 99 : m[102 + j]; p->pl[j] = m[106 + j] > 99 ? 99 : m[106 + j]; }
    p->alg = (uint8_t)((m[110] & 31) + 1); p->fb = m[111] & 7; p->oks = (m[111] >> 3) & 1;
    p->lfs = m[112] > 99 ? 99 : m[112]; p->lfd = m[113] > 99 ? 99 : m[113]; p->lpmd = m[114] > 99 ? 99 : m[114]; p->lamd = m[115] > 99 ? 99 : m[115];
    p->lks = m[116] & 1; p->lfw = (m[116] >> 1) & 7; p->lpms = (m[116] >> 4) & 7; p->trnsp = m[117] > 48 ? 24 : m[117];
    if (p->lfw > 5) p->lfw = 0;
    p->name = "BANK";
    fm6_user_id[t] = (int8_t)ptch;
}
static eff_t EF[GB_NTRACK];
typedef struct { float ph, delay, held, v, d; uint32_t rnd, t; } lfo_t;
static lfo_t LF[GB_NTRACK];
static const float LFO_HZ[100] = {
    0.062541f, 0.125031f, 0.312393f, 0.437120f, 0.624610f, 0.750694f, 0.936330f, 1.125302f, 1.249609f, 1.436782f,
    1.560915f, 1.752081f, 1.875117f, 2.062494f, 2.247191f, 2.374451f, 2.560492f, 2.686728f, 2.873976f, 2.998950f,
    3.188013f, 3.369840f, 3.500175f, 3.682224f, 3.812065f, 4.000800f, 4.186202f, 4.310716f, 4.501260f, 4.623209f,
    4.814636f, 4.930480f, 5.121901f, 5.315191f, 5.434783f, 5.617346f, 5.750431f, 5.946717f, 6.062811f, 6.248438f,
    6.431695f, 6.564264f, 6.749460f, 6.868132f, 7.052186f, 7.250580f, 7.375719f, 7.556294f, 7.687577f, 7.877738f,
    7.993605f, 8.181967f, 8.372405f, 8.504848f, 8.685079f, 8.810573f, 8.986341f, 9.122423f, 9.300595f, 9.500285f,
    9.607994f, 9.798158f, 9.950249f, 10.117361f, 11.251125f, 11.384335f, 12.562814f, 13.676149f, 13.904338f, 15.092062f,
    16.366612f, 16.638935f, 17.869907f, 19.193858f, 19.425019f, 20.833333f, 21.034918f, 22.502250f, 24.003841f, 24.260068f,
    25.746653f, 27.173913f, 27.578599f, 29.052876f, 30.693677f, 31.191516f, 32.658393f, 34.317090f, 34.674064f, 36.416606f,
    38.197097f, 38.550501f, 40.387722f, 40.749796f, 42.625746f, 44.326241f, 44.883303f, 46.772685f, 48.590865f, 49.261084f };

GB_SMALL static void fm6_block(const gb_patch *gp, int t, float beat)
{
    (void)beat;
    eff_t *E = &EF[t];
    int8_t e[8]; for (int k = 0; k < 8; k++) e[k] = (int8_t)gb_ev(gp, k);
    int8_t uid = e[7] >= FM6_NFACTORY && fm6_user_id[t] == e[7] ? e[7] : -1;
    int same = E->ok && E->uid == uid; for (int k = 0; k < 8 && same; k++) same = E->e[k] == e[k];
    if (!same) {
        const fm6_patch *src = e[7] < FM6_NFACTORY ? &fm6_factory[e[7]] : uid >= 0 ? &fm6_user[t] : &fm6_init_voice;   /* (a bank slot not read yet: the init voice) */
        E->uid = uid;
        E->p = *src;
        for (int k = 0; k < 8; k++) E->e[k] = e[k];
        E->alg = (uint8_t)(e[0] >= 1 ? e[0] - 1 : (src->alg - 1) & 31);
        int fb = src->fb + e[1]; E->p.fb = (uint8_t)(fb < 0 ? 0 : fb > 7 ? 7 : fb);
        uint8_t car = ALGS[E->alg].car;
        int meg = e[4] * 40 / 64, n = 0;
        float step = (float)e[6] * (12.0f / 127.0f);                     /* DTUN: cents a step */
        static const int8_t SPREAD[6] = { 0, 1, -1, 2, -2, 3 };
        for (int k = 0; k < 6; k++) {
            fm6_op *o = &E->p.op[k];
            E->dt[k] = 0.0f;
            if ((car >> k) & 1u) { E->dt[k] = (float)SPREAD[n++] * step; continue; }
            if (!o->mode) { int c = o->fc + e[3]; o->fc = (uint8_t)(c < 0 ? 0 : c > 31 ? 31 : c); }
            for (int j = 0; j < 4; j++) { int r = o->r[j] - meg; o->r[j] = (uint8_t)(r < 0 ? 0 : r > 99 ? 99 : r); }
            int kv = o->kvs + e[5]; o->kvs = (uint8_t)(kv < 0 ? 0 : kv > 7 ? 7 : kv);
        }
        E->ok = 1;
    }
    /* the LFO: its speed, wave, delay (once a block) */
    lfo_t *L = &LF[t];
    const fm6_patch *p = &E->p;
    float dph = LFO_HZ[p->lfs % 100] * (float)GB_BLOCK * GB_ISR;
    L->ph += dph;
    if (L->ph >= 1.0f) { L->ph -= (float)(int)L->ph; L->rnd = L->rnd * 1664525u + 1013904223u; L->held = (float)(int32_t)L->rnd * (1.0f / 2147483648.0f); }
    float ph = L->ph, v;
    switch (p->lfw) {
    case 0: v = ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph; break;   /* TRI */
    case 1: v = 1.0f - 2.0f * ph; break;                                  /* SAW DOWN */
    case 2: v = 2.0f * ph - 1.0f; break;                                  /* SAW UP */
    case 3: v = ph < 0.5f ? 1.0f : -1.0f; break;                          /* SQUARE */
    case 4: v = gb_sin_turn(ph); break;                                   /* SINE */
    default: v = L->held; break;                                          /* S&H */
    }
    L->v = v;
    float dl = (float)p->lfd * (1.0f / 99.0f), dsec = dl * dl * 4.0f;     /* the delay: up to ~4 s, then it fades in */
    L->t += GB_BLOCK;
    float tt = (float)L->t * GB_ISR;
    L->d = dsec <= 0.0f ? 1.0f : tt < dsec ? 0.0f : gb_minf(1.0f, (tt - dsec) / (0.5f * dsec + 0.01f));
}

/* ---------------------------------------------------------------- an operator's envelope (log domain, 1/256 octave) */
typedef struct { float level, target, inc; uint8_t ix, rising; } env_t;
static void env_advance(env_t *e, const fm6_op *o, int ix, float outlevel, int rscale)
{
    e->ix = (uint8_t)ix;
    if (ix >= 4) return;
    float actual = (float)((scale_level(o->l[ix]) >> 1) << 6) + outlevel - 4256.0f;
    e->target = actual < 16.0f ? 16.0f : actual;
    e->rising = e->target > e->level;
    int q = ((o->r[ix] * 41) >> 6) + rscale; q = q > 63 ? 63 : q;
    e->inc = (float)((4 + (q & 3)) << (2 + 6 + (q >> 2))) * (1.0f / 65536.0f);   /* (a block of 64) */
}
static void env_step(env_t *e, const fm6_op *o, int down, float outlevel, int rscale)
{
    if (e->ix < 3 || (e->ix < 4 && !down)) {
        if (e->rising) {
            if (e->level < 1716.0f) e->level = 1716.0f;
            e->level += (float)(int)(17.0f - e->level * (1.0f / 256.0f)) * e->inc;
            if (e->level >= e->target) { e->level = e->target; env_advance(e, o, e->ix + 1, outlevel, rscale); }
        } else {
            e->level -= e->inc;
            if (e->level <= e->target) { e->level = e->target; env_advance(e, o, e->ix + 1, outlevel, rscale); }
        }
    }
}

/* the voice: six operators (their phase, envelope, last gain), the feedback's last two, the pitch envelope */
typedef struct { uint32_t ph[6]; env_t env[6]; float g[6], out[6], fb0, fb1, pe, pe_t, pe_inc, dc; int16_t ol[6]; uint8_t pe_ix, down, rs[6], live; } note_t;
_Static_assert(sizeof(note_t) <= sizeof(((gb_voice *)0)->s), "an FM6 voice in the voice's state");
#define N(v) ((note_t *)(void *)(v)->s.raw)
static float pe_semis(int l) { float s = (float)(l - 50) * (1.0f / 50.0f); return s * (12.0f + 36.0f * s * s); }
static void pe_advance(note_t *n, const fm6_patch *p, int ix)
{
    n->pe_ix = (uint8_t)ix;
    if (ix >= 4) return;
    n->pe_t = pe_semis(p->pl[ix]);
    n->pe_inc = 0.004f * gb_exp2f((float)p->pr[ix] * 0.12f);
}
GB_SMALL static void fm6_start(gb_voice *v, const gb_patch *gp, int fresh)
{
    int t = v->track;
    fm6_block(gp, t, 0.0f);                                           /* (the patch as it is now) */
    const eff_t *E = &EF[t];
    const fm6_patch *p = &E->p;
    note_t *n = N(v);
    int note = v->note + p->trnsp - 24, vel = (int)(v->vel * 127.0f + 0.5f);
    if (fresh || !n->live) { memset(n, 0, sizeof *n); n->pe = pe_semis(p->pl[3]); }
    for (int k = 0; k < 6; k++) {
        const fm6_op *o = &p->op[k];
        int ol = scale_level(o->ol) + scale_kbd(note, o);
        ol = ol > 127 ? 127 : ol;
        ol = (ol << 5) + vel_scale(vel, o->kvs);
        n->ol[k] = (int16_t)(ol < 0 ? 0 : ol);
        n->rs[k] = (uint8_t)scale_rate(note, o->rs);
        if (fresh || !n->live) n->env[k].level = 0.0f;
        env_advance(&n->env[k], o, 0, (float)n->ol[k], n->rs[k]);
        if (p->oks && (fresh || !n->live)) n->ph[k] = 0;
    }
    pe_advance(n, p, 0);
    n->down = 1; n->live = 1;
    if (p->lks) { LF[t].ph = 0.0f; LF[t].t = 0; }
}
static void fm6_release(gb_voice *v, const gb_patch *gp)
{
    (void)gp;
    note_t *n = N(v);
    const fm6_patch *p = &EF[v->track].p;
    n->down = 0;
    for (int k = 0; k < 6; k++) env_advance(&n->env[k], &p->op[k], 3, (float)n->ol[k], n->rs[k]);
    pe_advance(n, p, 3);
}
static void fm6_spread(gb_voice *v, uint32_t r) { for (int k = 0; k < 6; k++) N(v)->ph[k] += r * (uint32_t)(k + 1); }

GB_HOT static int fm6_render(gb_voice *v, const gb_patch *gp, const gb_mod *m, float *out)
{
    note_t *n = N(v);
    const eff_t *E = &EF[v->track];
    const fm6_patch *p = &E->p;
    const alg_t *A = &ALGS[E->alg];
    const lfo_t *L = &LF[v->track];
    if (!n->live) { for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f; return 0; }
    /* the pitch: the voice's (glide, bend, LFO PIT) + the patch's transpose, pitch envelope and LFO */
    if (n->pe_ix < 4 || n->pe != n->pe_t) {
        float d = n->pe_t - n->pe;
        if (gb_absf(d) <= n->pe_inc) { n->pe = n->pe_t; if (n->pe_ix < 3 || (n->pe_ix == 3 && !n->down)) pe_advance(n, p, n->pe_ix + 1); }
        else n->pe += d > 0.0f ? n->pe_inc : -n->pe_inc;
    }
    static const float PMS[8] = { 0.0f, 0.04f, 0.08f, 0.13f, 0.22f, 0.36f, 0.6f, 1.0f };
    float pitch = m->pitch + (float)(p->trnsp - 24) + n->pe + L->v * L->d * (float)p->lpmd * (1.0f / 99.0f) * PMS[p->lpms & 7] * 12.0f;
    float base = gb_note_hz(pitch);
    int fb = gb_clampi(p->fb + (int)((m->shape - 64.0f) * (1.0f / 8.0f)), 0, 7);
    float fbk = fb ? gb_exp2f((float)(fb - 8)) : 0.0f;                 /* the feedback: turns per unit of the last two's mean */
    float mlvl = (float)gb_ev(gp, 2) * 24.0f + gb_clampf(m->cut, -150.0f, 150.0f) * 16.0f;   /* the modulators, 1/256 octave */
    static const float AMS[4] = { 0.0f, 0.3f, 0.7f, 1.4f };
    float amu = 0.5f * (L->v + 1.0f) * L->d * (float)p->lamd * (1.0f / 99.0f);
    uint32_t inc[6]; float g0[6], dg[6];
    int alive = 0;
    for (int k = 0; k < 6; k++) {
        const fm6_op *o = &p->op[k];
        env_step(&n->env[k], o, n->down, (float)n->ol[k], n->rs[k]);
        float f;
        if (o->mode) f = gb_exp2f(((float)((o->fc & 3) * 100 + o->ff)) * 0.0332193f + (float)(o->det - 7) * 0.0008f);
        else {
            float lf = 3.031f + (pitch - 0.0f) * (1.0f / 12.0f);
            f = base * (o->fc ? (float)o->fc : 0.5f) * (1.0f + (float)o->ff * 0.01f) * gb_exp2f((float)(o->det - 7) * 0.0209f * gb_exp2f(-0.571f * lf) / 7.0f * lf);
        }
        if (E->dt[k] != 0.0f) f *= gb_exp2f(E->dt[k] * (1.0f / 1200.0f));
        inc[k] = (uint32_t)(gb_minf(f * GB_ISR, 0.49f) * 4294967296.0f);
        float lv = n->env[k].level - 3584.0f + (((A->car >> k) & 1u) ? 0.0f : mlvl) - AMS[o->ams & 3] * amu * 256.0f;
        float g = lv < -3600.0f ? 0.0f : gb_exp2f(lv * (1.0f / 256.0f));   /* (at full: 2, a modulator two turns) */
        g0[k] = n->g[k]; dg[k] = (g - n->g[k]) * (1.0f / GB_BLOCK); n->g[k] = g;
        if (((A->car >> k) & 1u) && (n->env[k].ix < 4 || g > 0.0002f || g0[k] > 0.0002f)) alive = 1;
    }
    /* the samples: OP6 down to OP1 (a modulator's number is above its carrier's) */
    float y[6], f0 = n->fb0, f1 = n->fb1, dc = n->dc;
    for (int k = 0; k < 6; k++) y[k] = n->out[k];
    int fo = A->fbo - 1, fs = A->fbs - 1;
    for (int i = 0; i < GB_BLOCK; i++) {
        float s = 0.0f;
        for (int k = 5; k >= 0; k--) {
            float in = 0.0f;
            uint8_t mm = A->mod[k];
            for (int j = k + 1; mm >> j; j++) if ((mm >> j) & 1u) in += y[j];
            if (k == fo && fbk > 0.0f) in += 0.5f * (f0 + f1) * fbk;
            in -= (float)(int)in;                                         /* (turns: a modulator at 1.0 moves a turn) */
            uint32_t ph = n->ph[k] + ((uint32_t)(int32_t)(in * 1073741824.0f) << 2);
            float g = g0[k] + dg[k] * (float)(i + 1);
            y[k] = gb_sin_u32(ph) * g;
            n->ph[k] += inc[k];
            if ((A->car >> k) & 1u) s += y[k];
        }
        f1 = f0; f0 = y[fs];
        float x = s - dc; dc += x * (1.0f / 1024.0f);                 /* (~7 Hz: FM puts sidebands at 0 Hz) */
        out[i] = x * 0.125f;                                          /* (a carrier at full: half SLOOP's unit) */
    }
    for (int k = 0; k < 6; k++) n->out[k] = y[k];
    n->fb0 = gb_flush(f0); n->fb1 = gb_flush(f1); n->dc = gb_flush(dc);
    if (!alive) n->live = 0;
    return alive;
}

const gb_engine gb_eng_fm6 = { "FM6", ED, 6, 1, fm6_start, fm6_render, fm6_release, fm6_spread, fm6_block };
