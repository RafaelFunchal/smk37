/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37's controls as numbers, no hardware (the panel driver hal/smk37/panel.c runs them; tests/smk_test.c on the
 * PC): the factory calibration record and its velocity curves, the two-contact key velocity, the pads' trigger, the
 * faders' filter, the wheels (self-calibrating) and the PEDAL jack.
 * The algorithms are SLOOP Pro's (isod89/sloop-pro, GPL-3.0: hal/smk_input.h, src/smk_wheel.h, src/smk_pedal.h, after
 * the SMK-37 layer by M. Geisler, genkigenki/fm1-to-smk37), rewritten for SLOOP 3. */
#pragma once
#include <stdint.h>

/* ---- the calibration record (flash 0xC1000, 0x22C bytes): +0x08 16 pads x 5 u16, +0xA8 37 keys x 5 u16, the marker
 * "trimdat\0" at +0x224. Taken only if the marker is there and every curve rises; else the stock defaults */
typedef struct { uint16_t pad[16][5], key[37][5]; uint8_t from_unit; } smk_trim;
static inline void smk_trim_defaults(smk_trim *t)
{
    static const uint16_t P[5] = { 50, 550, 720, 850, 950 }, K[5] = { 5, 640, 800, 920, 980 };
    for (int i = 0; i < 16; i++) for (int j = 0; j < 5; j++) t->pad[i][j] = P[j];
    for (int i = 0; i < 37; i++) for (int j = 0; j < 5; j++) t->key[i][j] = K[j];
    t->from_unit = 0;
}
static inline int smk_trim_load(smk_trim *t, const uint8_t *rec)
{
    static const char M[8] = "trimdat";
    for (int i = 0; i < 8; i++) if (rec[0x224 + i] != (uint8_t)M[i]) return 0;
    for (int i = 0; i < 16 + 37; i++) {
        const uint8_t *p = rec + 8 + i * 10;
        for (int j = 1; j < 5; j++) if ((p[2 * j] | p[2 * j + 1] << 8) <= (p[2 * j - 2] | p[2 * j - 1] << 8)) return 0;
    }
    for (int i = 0; i < 16 + 37; i++) for (int j = 0; j < 5; j++) {
        uint16_t v = (uint16_t)(rec[8 + i * 10 + 2 * j] | rec[9 + i * 10 + 2 * j] << 8);
        if (i < 16) t->pad[i][j] = v; else t->key[i - 16][j] = v;
    }
    t->from_unit = 1;
    return 1;
}
/* the stock lookup: x through a 5-point rising curve -> 1..1023 */
static inline uint32_t smk_curve(const uint16_t *t, uint32_t x)
{
    uint32_t i = 0;
    while (i < 5u && x > t[i]) i++;
    if (i == 0) return 1;
    if (i == 5u) return 1023;
    uint32_t v = (i - 1u) * 256u + ((x - t[i - 1u]) << 8) / (uint32_t)(t[i] - t[i - 1u]);
    return v ? v : 1u;
}
static inline uint8_t smk_vel7(uint32_t v10) { return (uint8_t)(v10 >> 3 ? v10 >> 3 : 1u); }
/* a key: contact 1 then contact 2 after `us` microseconds -> MIDI velocity 1..127 (stock: T = 10 + 0.1 ms units) */
static inline uint8_t smk_key_velocity(const uint16_t curve[5], uint32_t us)
{
    uint32_t T = 10u + us / 100u, x = T <= 0x3FEu ? 0x3FFu - T : 1u;
    return smk_vel7(smk_curve(curve, x));
}

/* ---- KEY VELOCITY: what the player chose (HOME settings): the unit's curves, softer, harder, or fixed */
enum { KV_DYNAMIC, KV_SOFT, KV_HARD, KV_FIX64, KV_FIX100, KV_FIX127, KV_N };
static inline int smk_vel_apply(int mode, int v)
{
    switch (mode) {
    case KV_SOFT: { int s = v * v / 127; return s < 1 ? 1 : s; }              /* more travel for the same loudness */
    case KV_HARD: { int h = 127 - (127 - v) * (127 - v) / 127; return h > 127 ? 127 : h; }
    case KV_FIX64: return 64;
    case KV_FIX100: return 100;
    case KV_FIX127: return 127;
    default: return v < 1 ? 1 : v > 127 ? 127 : v;
    }
}

/* ---- a pad: a force sensor read every ~1.8 ms. A hit starts `on` above the baseline and above the guard (the pad's
 * own envelope, fading); its velocity is the peak through the pad's curve, sent once the force falls or after
 * PEAKMAX readings; released after RELN readings under base + on/2; no new hit for LOCKN readings after one. The
 * baseline follows a quiet pad slowly both ways (SLOOP Pro 2.8: the stock tracking made pads ever more sensitive) */
#define SMK_PAD_PEAKMAX 4u
#define SMK_PAD_RELN 4u
#define SMK_PAD_LOCKN 22u
#define SMK_PAD_BASEN 8u
typedef struct { uint16_t base, peak, rt, vly; uint8_t st, low, lock, bc; } smk_pad_t;
static inline void smk_pad_init(smk_pad_t *p) { p->base = 100; p->peak = p->rt = 0; p->vly = 0xFFFF; p->st = p->low = p->lock = p->bc = 0; }
/* one reading -> velocity of a new hit (1..127), -1 a release, 0 nothing */
static inline int smk_pad_run(smk_pad_t *p, const uint16_t curve[5], uint32_t raw, uint32_t on)
{
    uint32_t base = p->base, st = p->st, rt = p->rt, lk = p->lock, h = raw > base ? raw - base : 0u;
    if (lk) p->lock = (uint8_t)(lk - 1u);
    if (rt) rt -= rt / 48u ? rt / 48u : 1u;
    int hit = st == 0u && !lk && h > (rt + on / 2u > on ? rt + on / 2u : on) && raw >= (uint32_t)p->vly + on;
    p->rt = (uint16_t)(h > rt ? h : rt);
    if (st == 0u) {
        if (raw < p->vly) p->vly = (uint16_t)raw;
        if (hit) { p->st = 1; p->peak = (uint16_t)raw; return 0; }
        if (base > raw + 2u * on) p->base = (uint16_t)(base - 1u);                 /* far above the rest: down at once */
        else if (!lk && h <= on && rt < on && ++p->bc >= SMK_PAD_BASEN) {
            p->bc = 0;
            if (raw > base) p->base = (uint16_t)(base + 1u); else if (raw < base) p->base = (uint16_t)(base - 1u);
        }
        return 0;
    }
    if (st < 0x80u) {
        uint32_t pk = p->peak;
        if (raw >= pk && st + 1u < SMK_PAD_PEAKMAX) { p->peak = (uint16_t)raw; p->st = (uint8_t)(st + 1u); return 0; }
        if (raw > pk) p->peak = (uint16_t)(pk = raw);
        p->st = 0x80; p->low = 0; p->lock = SMK_PAD_LOCKN;
        return smk_vel7(smk_curve(curve, pk));
    }
    if (raw < base + on / 2u) {
        if (++p->low >= SMK_PAD_RELN) { p->st = 0; p->vly = (uint16_t)raw; return -1; }
    } else p->low = 0;
    return 0;
}

/* ---- a fader: a median of 3 (a lone spike dropped), a signed x16 IIR, 0..127 with a +-1 dead band that still
 * reaches both ends */
typedef struct { uint16_t h0, h1; int32_t acc; int16_t v; } smk_fad_t;
static inline void smk_fad_init(smk_fad_t *f) { f->h0 = f->h1 = 0; f->acc = 0; f->v = -1; }
static inline void smk_fad_run(smk_fad_t *f, uint32_t raw)
{
    uint32_t h0 = f->h0, h1 = f->h1, m;
    f->h1 = (uint16_t)h0; f->h0 = (uint16_t)raw;
    m = raw > h0 ? (h0 > h1 ? h0 : raw > h1 ? h1 : raw) : (raw > h1 ? raw : h0 > h1 ? h1 : h0);
    if (f->v < 0) { f->acc = (int32_t)raw * 16; }                                 /* the first reading: there at once */
    f->acc += ((int32_t)m * 16 - f->acc) / 2;
    int32_t v = f->acc / 16 / 8; v = v < 0 ? 0 : v > 127 ? 127 : v;
    if (f->v < 0 || v > f->v + 1 || v + 1 < f->v || (v != f->v && (v == 0 || v == 127))) f->v = (int16_t)v;
}

/* ---- the pitch wheel: the first reading is the centre (it follows a resting wheel slowly), the ends widen to what
 * the wheel reaches; -> -1..1 (0 inside the dead band). The mod wheel: up reads lower; -> 0..1 */
#define SMK_PW_DEAD 12
#define SMK_PW_SPAN0 80
#define SMK_PW_DRIFT_MS 250u
typedef struct { int32_t pc, plo, phi; uint32_t drift_ms; } smk_pw_t;
static inline float smk_pw_bend(smk_pw_t *w, int32_t p, uint32_t now_ms)
{
    if (!w->pc) { w->pc = p ? p : 1; w->plo = p - SMK_PW_SPAN0; w->phi = p + SMK_PW_SPAN0; w->drift_ms = now_ms; }
    if (p < w->plo) w->plo = p;
    if (p > w->phi) w->phi = p;
    int32_t d = p - w->pc;
    if (d > -SMK_PW_DEAD && d < SMK_PW_DEAD) {
        if (now_ms - w->drift_ms >= SMK_PW_DRIFT_MS) { w->pc += d > 0 ? 1 : d < 0 ? -1 : 0; w->drift_ms = now_ms; }
        return 0.0f;
    }
    w->drift_ms = now_ms;
    int32_t up = w->phi - w->pc - SMK_PW_DEAD, dn = w->pc - w->plo - SMK_PW_DEAD;
    float b = d > 0 ? (float)(d - SMK_PW_DEAD) / (float)(up > 1 ? up : 1) : (float)(d + SMK_PW_DEAD) / (float)(dn > 1 ? dn : 1);
    return b < -1.0f ? -1.0f : b > 1.0f ? 1.0f : b;
}
#define SMK_MW_LO 485
#define SMK_MW_HI 615
#define SMK_MW_DEAD 12
typedef struct { int32_t lo, hi; } smk_mw_t;
static inline float smk_mw_val(smk_mw_t *w, int32_t m)
{
    if (!w->hi) { w->lo = m < SMK_MW_LO ? m : SMK_MW_LO; w->hi = m > SMK_MW_HI ? m : SMK_MW_HI; }
    if (m < w->lo) w->lo = m;
    if (m > w->hi) w->hi = m;
    int32_t span = w->hi - w->lo - SMK_MW_DEAD;
    float v = w->hi - m <= SMK_MW_DEAD ? 0.0f : (float)(w->hi - m - SMK_MW_DEAD) / (float)(span > 1 ? span : 1);
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

/* ---- the PEDAL jack, a sustain switch of either polarity: the reading at power-on is UP; DOWN SMK_PED_ON away from
 * it, UP again at SMK_PED_OFF or less, after SMK_PED_N readings in a row. Down for SMK_PED_STUCK_MS without having
 * proven its polarity (down, then up for SMK_PED_PROVEN_MS): that reading is the new rest (a pedal plugged in late,
 * the other way round) */
#define SMK_PED_ON 450u
#define SMK_PED_OFF 250u
#define SMK_PED_N 3u
#define SMK_PED_STUCK_MS 120000u
#define SMK_PED_PROVEN_MS 30000u
typedef struct { uint16_t rest; uint8_t learnt, down, cand, cnt, was_down, proven; uint32_t since; } smk_ped_t;
/* one reading -> 1 went down, 0 went up, -1 no change */
static inline int smk_ped_sample(smk_ped_t *s, uint32_t raw, uint32_t ms)
{
    if (raw > 1023u) return -1;
    if (!s->learnt) { s->rest = (uint16_t)raw; s->learnt = 1; s->down = s->cand = 0; s->cnt = 0; return -1; }
    uint32_t d = raw > s->rest ? raw - s->rest : s->rest - raw, side;
    if (d >= SMK_PED_ON) side = 1u; else if (d <= SMK_PED_OFF) side = 0u; else { s->cnt = 0; return -1; }
    if (side != s->cand) { s->cand = (uint8_t)side; s->cnt = 0; }
    if (s->cnt < SMK_PED_N) s->cnt++;
    if (s->cnt < SMK_PED_N) return -1;
    if (side == s->down) {
        if (!side && s->was_down && (uint32_t)(ms - s->since) >= SMK_PED_PROVEN_MS) s->proven = 1;
        if (side && !s->proven && (uint32_t)(ms - s->since) >= SMK_PED_STUCK_MS) { s->rest = (uint16_t)raw; s->down = s->cand = 0; s->cnt = 0; return 0; }
        return -1;
    }
    s->down = (uint8_t)side; s->was_down |= (uint8_t)side; s->since = ms;
    return (int)side;
}
