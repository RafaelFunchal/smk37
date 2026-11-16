/* SPDX-License-Identifier: GPL-3.0-only
 * GRAIN: SLOOP's granular engine (docs/sloop/SPEC-MOTEURS.md), written for this project. It reads SAMPLE's sets: a
 * note picks its zone, as SAMPLE does, and short grains of it are played over each other.
 *   A grain: SIZE long (10..390 ms), a Hann window, a rate (the note against the zone's root, PTCH, a random detune
 *   up to an octave with RAND), forwards or (RAND: up to half of them) backwards, centred on POS (+ the track's
 *   SHAPE modulation: an LFO scans, ENV > SHP sweeps) spread at random by SPRD, fitted into its zone. A voice starts
 *   one DENS times a second (1..98, the interval jittered by SPRD), its level 1 / sqrt(the grains it overlaps).
 *   TONE: a one-pole low-pass (+ the track's FLT modulation).
 * The grains of a track share a pool of 12 (a voice: its share, at least 2), and they decode the ADPCM as they play:
 * from the bank's seek index (the decoder's state every 512 samples of every zone, made by tools/mksamples.py), a
 * reverse grain through a 64-sample window it fills again as it moves down. The cost is bounded: the grains of a
 * track decode at most 8 source samples an output sample (a reverse one counts three times), a voice starts at most
 * one grain a block. Two pools: two tracks of GRAIN at once (a third waits for one to fall silent).
 *   E0 SRC  E1 POS  E2 SIZE  E3 DENS  E4 PTCH +-24  E5 SPRD  E6 RAND  E7 TONE */
#include "engine.h"
#include "sample.h"

static const gb_pd ED[8] = { { 0, 11, 0 }, { 0, 127, 32 }, { 0, 127, 80 }, { 0, 127, 80 }, { -24, 24, 0 }, { 0, 127, 30 },
                             { 0, 127, 10 }, { 0, 127, 127 } };
#define NG 12                    /* grains a track */
#define RB 64                    /* a reverse grain's window, source samples */
#define NSLOT 2                  /* tracks of GRAIN at once */
#define LOAD_MAX 8.0f            /* source samples decoded an output sample, a track's grains in all */
#define REV_W 3.0f               /* what a reverse grain's sample costs (its window filled again from the index) */

typedef struct {
    uint32_t tag;                /* the voice's (0: free) */
    uint32_t pos;                /* forward: the next sample to decode */
    uint32_t rlo;                /* reverse: the source index of rb[0] */
    uint32_t last;               /* the block it last sounded in */
    float fr;                    /* forward: between s0 and s1; reverse: the read position (source samples) */
    float st;                    /* source samples an output sample */
    float s0, s1, ph, inc, g;    /* the window's phase 0..1 and its step, the level */
    int16_t pred, delay;         /* the decoder; the samples to wait in its first block */
    uint16_t zone;
    uint8_t idx, rev, rbn;       /* rbn: the samples in the window (0: to fill) */
    int16_t rb[RB];
} grain;
/* a user slot's seek index (its zones' decoder state every 512 samples), made a little each block (grain_block) */
#define UXS 512u
#define UXN 336
typedef struct {
    grain g[NG]; uint32_t rng; uint8_t nact, cnt;
    int8_t ux_k; uint8_t ux_done, ux_z, ux_i; int16_t ux_p; uint16_t ux_n; uint32_t ux_pos;
    uint16_t ux_first[17]; int16_t ux_pred[UXN]; uint8_t ux_idx[UXN];
} pool;
static pool SL[NSLOT];                  /* (a pool is cleared when it is taken) */
static uint8_t slot_trk[NSLOT];         /* the track + 1 (0: none) */
static uint32_t slot_last[NSLOT], next_tag = 1;

#define GS(v) (v)->s.grain
void grain_forget(void)                 /* every grain dropped, the user slots' indexes made again (a slot rewritten) */
{
    for (int i = 0; i < NSLOT; i++) { for (int j = 0; j < NG; j++) SL[i].g[j].tag = 0; SL[i].ux_k = -1; }
}
static pool *pool_find(int t)
{
    for (int i = 0; i < NSLOT; i++) if (slot_trk[i] == t + 1) return &SL[i];
    return 0;
}
static pool *pool_take(int t)
{
    int best = -1; uint32_t age = 1;
    for (int i = 0; i < NSLOT; i++) {
        if (slot_trk[i] == t + 1) { slot_last[i] = gb_blocks; return &SL[i]; }
        uint32_t a = slot_trk[i] ? gb_blocks - slot_last[i] : 0xFFFFFFFFu;
        if (a > age) { age = a; best = i; }      /* free, or the longest unheard (not in the last two blocks) */
    }
    if (best < 0) return 0;
    pool *P = &SL[best];
    for (int i = 0; i < NG; i++) P->g[i].tag = 0;
    P->rng = 0x9E3779B9u * (uint32_t)(t + 1) + gb_blocks; P->nact = 1; P->cnt = 0; P->ux_k = -1;
    slot_trk[best] = (uint8_t)(t + 1); slot_last[best] = gb_blocks;
    return P;
}
static float rnd(pool *P)                 /* 0..1 */
{
    P->rng = P->rng * 1664525u + 1013904223u;
    return (float)(P->rng >> 8) * (1.0f / 16777216.0f);
}

static int zone_of(int set, int note) { return smp_bank_ok() ? smp_find(set, note) : -1; }
GB_SMALL static void grain_start(gb_voice *v, const gb_patch *p, int fresh)
{
    if (fresh || !GS(v).tag) { GS(v).tag = next_tag++; if (!next_tag) next_tag = 1; GS(v).lp = 0.0f; }
    GS(v).cd = 0.0f;                                           /* a grain at once */
    int zi = gb_flash_ok ? zone_of(gb_ev(p, 0), v->note) : -1;
    GS(v).zone = zi < 0 ? 0xFFFF : (uint16_t)zi;
    if (gb_flash_ok) pool_take(v->track);
}

/* one ADPCM sample: the k-th of the zone's data d */
GB_INLINE int dec1(const uint8_t *d, uint32_t k, int *pred, int *idx)
{
    uint32_t b = d[k >> 1], code = (k & 1u) ? b >> 4 : b & 15u;
    int step = smp_ima_step[*idx], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    int pr = *pred + ((code & 8u) ? -vd : vd);
    *pred = pr < -32768 ? -32768 : pr > 32767 ? 32767 : pr;
    int ix = *idx + smp_ima_idx[code & 7u]; *idx = ix < 0 ? 0 : ix > 88 ? 88 : ix;
    return *pred;
}
/* a user zone's seek point for sample k there yet (its slot's index made so far) */
static int ux_has(const pool *P, uint32_t zone, uint32_t k)
{
    uint32_t j = zone & 31u;
    if (P->ux_k != (int)((zone >> 5) & 3u)) return 0;
    if (P->ux_done || j < P->ux_z) return 1;
    return j == P->ux_z && P->ux_first[j] + k / UXS < P->ux_n;
}
/* the decoder's state before sample k (the seek point below, then the samples between) */
static void seek(const pool *P, uint32_t zone, const uint8_t *d, uint32_t k, int *pred, int *idx)
{
    uint32_t from;
    if (zone & SMP_USR) { uint32_t e = P->ux_first[zone & 31u] + k / UXS; *pred = P->ux_pred[e]; *idx = P->ux_idx[e]; from = k & ~(UXS - 1u); }
    else { uint32_t s = smp_seek((int)zone, k >> SMP_SEEK_SHIFT); *pred = (int16_t)(s & 0xFFFFu); *idx = (int)(s >> 16); from = k & ~((1u << SMP_SEEK_SHIFT) - 1u); }
    if (*idx > 88) *idx = 88;
    for (uint32_t i = from; i < k; i++) dec1(d, i, pred, idx);
}

static void spawn(pool *P, gb_voice *v, const gb_patch *p, const gb_mod *m, int share, float ivl, int delay)
{
    grain *f = 0; int own = 0; float load = 0.0f;
    for (int i = 0; i < NG; i++) {
        grain *g = &P->g[i];
        if (!g->tag) { if (!f) f = g; continue; }
        if (g->tag == GS(v).tag) own++;
        load += g->st * (g->rev ? REV_W : 1.0f);
    }
    if (!f || own >= share) return;
    const smp_zone *z = smp_zone_of(GS(v).zone);
    if (z->n < 64) return;
    float r1 = rnd(P), r2 = rnd(P), r3 = rnd(P), rr = (float)gb_ev(p, 6), sp = (float)gb_ev(p, 5) * (1.0f / 127.0f);
    int rev = r3 * 256.0f < rr;                                       /* RAND 127: half of them backwards */
    float amt = rr * rr * (12.0f / (127.0f * 127.0f));                /* the random detune, up to an octave */
    float semi = m->pitch + (float)gb_ev(p, 4) - (float)z->root16 * (1.0f / 16.0f) + (2.0f * r1 - 1.0f) * amt;
    float st = gb_exp2f(gb_clampf(semi, -96.0f, 96.0f) * (1.0f / 12.0f)) * (float)z->rate * (1.0f / 65536.0f);   /* (3.3.6: as SAMPLE reads it; was / 44100: 1.49 x too fast) */
    st = gb_minf(st, LOAD_MAX);
    if (load + st * (rev ? REV_W : 1.0f) > LOAD_MAX) return;
    float len = 441.0f * gb_exp2f((float)gb_ev(p, 2) * (1.0f / 24.0f));   /* 10..390 ms */
    float n = (float)(z->n - 2), span = len * st;
    if (span > n) { span = n; len = n / st; }                           /* a short zone: a shorter grain */
    if (len < 32.0f) return;
    float c = gb_clampf((float)gb_ev(p, 1) + m->shape - 64.0f, 0.0f, 127.0f) * (1.0f / 127.0f) + (r2 - 0.5f) * sp;
    float a = gb_clampf(c * n - 0.5f * span, 0.0f, n - span);
    if ((GS(v).zone & SMP_USR) && !ux_has(P, GS(v).zone, (uint32_t)(a + span))) return;   /* (its index not made that far yet) */
    f->tag = GS(v).tag; f->last = gb_blocks; f->zone = GS(v).zone; f->st = st; f->rev = (uint8_t)rev;
    f->ph = 0.0f; f->inc = 1.0f / len; f->delay = (int16_t)delay;
    float ov = gb_clampf(len / ivl, 1.0f, (float)share);              /* the grains it overlaps */
    f->g = 1.0f / gb_sqrtf(ov);
    if (rev) { f->fr = a + span; f->rbn = 0; f->rlo = 0; }
    else {
        const uint8_t *d = smp_zone_data(f->zone) + z->off;
        uint32_t k = (uint32_t)a; int pr, ix;
        seek(P, f->zone, d, k, &pr, &ix);
        f->s0 = (float)dec1(d, k, &pr, &ix); f->s1 = (float)dec1(d, k + 1, &pr, &ix);
        f->pos = k + 2; f->fr = a - (float)k; f->pred = (int16_t)pr; f->idx = (uint8_t)ix;
    }
}
/* a reverse grain's window: [i + 2 - RB, i + 1] (or from 0) */
static void refill(const pool *P, grain *g, const smp_zone *z, const uint8_t *d, uint32_t i)
{
    uint32_t lo = i + 2 >= RB ? i + 2 - RB : 0, nn = z->n - lo < RB ? z->n - lo : RB;
    int pr, ix;
    seek(P, g->zone, d, lo, &pr, &ix);
    for (uint32_t k = 0; k < nn; k++) g->rb[k] = (int16_t)dec1(d, lo + k, &pr, &ix);
    g->rlo = lo; g->rbn = (uint8_t)nn;
}

GB_HOT static int grain_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f;
    if (GS(v).zone == 0xFFFF || !smp_bank_ok()) return 0;
    if (!gb_flash_ok) return 1;                                /* (a save: the flash is not to be read; silent, waiting) */
    pool *P = pool_take(v->track);
    if (!P) return 1;
    P->cnt++;
    int share = NG / (P->nact ? P->nact : 1); if (share < 2) share = 2;
    float sp = (float)gb_ev(p, 5) * (1.0f / 127.0f);
    float ivl = GB_SR * gb_exp2f(-(float)gb_ev(p, 3) * (10.0f / 192.0f));  /* DENS: 1..98 a second */
    if (GS(v).cd < (float)GB_BLOCK) {
        spawn(P, v, p, m, share, ivl, (int)GS(v).cd);
        GS(v).cd += ivl * (1.0f + (rnd(P) - 0.5f) * sp);
    }
    GS(v).cd = gb_maxf(GS(v).cd - (float)GB_BLOCK, 0.0f);
    for (int j = 0; j < NG; j++) {
        grain *g = &P->g[j];
        if (g->tag != GS(v).tag) continue;
        g->last = gb_blocks;
        const smp_zone *z = smp_zone_of(g->zone);
        const uint8_t *d = smp_zone_data(g->zone) + z->off;
        float ph = g->ph, inc = g->inc, st = g->st, gl = g->g, fr = g->fr;
        int i = g->delay, done = 0; g->delay = 0;
        if (!g->rev) {
            float s0 = g->s0, s1 = g->s1; uint32_t pos = g->pos; int pr = g->pred, ix = g->idx;
            for (; i < GB_BLOCK; i++) {
                float w = gb_sin_turn(ph * 0.5f);
                out[i] += (s0 + (s1 - s0) * fr) * w * w * gl;
                fr += st;
                while (fr >= 1.0f) { fr -= 1.0f; s0 = s1; s1 = pos < z->n ? (float)dec1(d, pos, &pr, &ix) : 0.0f; pos++; }
                ph += inc;
                if (ph >= 1.0f) { done = 1; break; }
            }
            g->s0 = s0; g->s1 = s1; g->pos = pos; g->pred = (int16_t)pr; g->idx = (uint8_t)ix;
        } else {
            for (; i < GB_BLOCK; i++) {
                uint32_t k = (uint32_t)fr;
                if (!g->rbn || k < g->rlo || k + 1 >= g->rlo + g->rbn) refill(P, g, z, d, k);
                float a = (float)g->rb[k - g->rlo], b = k + 1 < g->rlo + g->rbn ? (float)g->rb[k + 1 - g->rlo] : a;
                float w = gb_sin_turn(ph * 0.5f);
                out[i] += (a + (b - a) * (fr - (float)k)) * w * w * gl;
                fr = gb_maxf(fr - st, 0.0f);
                ph += inc;
                if (ph >= 1.0f) { done = 1; break; }
            }
        }
        g->fr = fr; g->ph = ph;
        if (done) g->tag = 0;
    }
    /* TONE (+ the track's FLT modulation), at SLOOP's unit (0.5) */
    float k = (4000.0f + gb_clampf((float)gb_ev(p, 7) + m->cut, 0.0f, 127.0f) * (28767.0f / 127.0f)) * (1.0f / 32768.0f);
    float lp = GS(v).lp;
    for (int i = 0; i < GB_BLOCK; i++) { lp += (out[i] * (1.0f / 32768.0f) + GB_BIAS - lp) * k; out[i] = lp * 0.5f; }
    GS(v).lp = lp;
    return 1;
}
/* once a block: the voices that sounded, and the grains whose voice is gone */
static void grain_block(const gb_patch *p, int t, float beat)
{
    (void)beat;
    pool *P = pool_find(t);
    if (!P) return;
    P->nact = P->cnt; P->cnt = 0;
    for (int i = 0; i < NG; i++) if (P->g[i].tag && P->g[i].last != gb_blocks) P->g[i].tag = 0;
    /* a user slot: its seek index, 4096 samples a block (a full slot: ~0.1 s) */
    if (!gb_flash_ok) return;                                  /* (3.1.8: a save has the window off, and smp_nsets reads the bank's
                                                                  header through it: until 3.1.7 every save with a GRAIN track read it) */
    int k = gb_ev(p, 0) - smp_nsets();
    if (k < 0 || k >= SMP_NUSR || !smp_user_nz(k)) return;
    if (P->ux_k != k) { P->ux_k = (int8_t)k; P->ux_done = 0; P->ux_z = 0; P->ux_pos = 0; P->ux_n = 0; }
    int nz = smp_user_nz(k), pr = P->ux_p, ix = P->ux_i;
    for (int budget = 4096; !P->ux_done && budget > 0; ) {
        if (P->ux_z >= nz) { P->ux_done = 1; break; }
        uint32_t id = SMP_USR | (uint32_t)k << 5 | P->ux_z;
        const smp_zone *z = smp_zone_of(id);
        if (P->ux_pos == 0) { P->ux_first[P->ux_z] = P->ux_n; pr = 0; ix = 0; }
        if (P->ux_pos >= z->n) { P->ux_z++; P->ux_pos = 0; continue; }
        if (!(P->ux_pos % UXS)) { if (P->ux_n >= UXN) { P->ux_done = 1; break; } P->ux_pred[P->ux_n] = (int16_t)pr; P->ux_idx[P->ux_n] = (uint8_t)ix; P->ux_n++; }
        const uint8_t *d = smp_zone_data(id) + z->off;
        uint32_t e = P->ux_pos + UXS - P->ux_pos % UXS; if (e > z->n) e = z->n;
        for (; P->ux_pos < e; P->ux_pos++, budget--) dec1(d, P->ux_pos, &pr, &ix);
    }
    P->ux_p = (int16_t)pr; P->ux_i = (uint8_t)ix;
}
const gb_engine gb_eng_grain = { "GRAIN", ED, 3, 0, grain_start, grain_render, 0, 0, grain_block };
