/* SPDX-License-Identifier: GPL-3.0-only
 * WHEEL: a tonewheel organ with SLOOP's parameters (docs/sloop/SPEC-MOTEURS.md), written for this project. Nine
 * sines at the drawbar footages (16' 5 1/3' 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1'), their levels 0..8 (3 dB a step) from
 * one of 16 registrations plus SUB / BODY / TOP; a partial above ~6 kHz folds down an octave. PERC (single
 * trigger: only when no other key is held), a key CLICK, a DRIVE, a rotary speaker (two rotors, an amplitude and a
 * Doppler swing, easing between OFF SLOW FAST): the rotors are the track's, made once a block.
 *   E0 REG   E1 SUB  E2 BODY  E3 TOP (-8..8)   E4 PERC OFF 2ND 3RD 2SOFT 3SOFT 2SLOW 3SLOW   E5 CLICK  E6 DRV
 *   E7 ROTR OFF SLOW FAST */
#include "engine.h"

static const gb_pd ED[8] = { { 0, 15, 4 }, { -8, 8, 0 }, { -8, 8, 0 }, { -8, 8, 0 }, { 0, 6, 0 }, { 0, 127, 40 },
                             { 0, 127, 0 }, { 0, 2, 1 } };
#define NP 9
static const float RATIO[NP] = { 0.5f, 1.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
/* the registrations, a digit a bar (16' .. 1') */
static const char REG[16][NP + 1] = { "008000000", "008400000", "800800000", "838000000", "888000000", "888800000",
                                      "888600006", "888000888", "800000888", "008080800", "008765432", "006876540",
                                      "004432000", "008888000", "858000000", "888888888" };
static const float LV[9] = { 0.0f, 0.0891f, 0.1259f, 0.1778f, 0.2512f, 0.3548f, 0.5012f, 0.7079f, 1.0f };

typedef struct {                                 /* a track's, once a block (wheel_block) */
    float g[NP], plev, pdec, clk, dg, dcomp;
    int pk;
    float rph[2], rsp[2], rwet, am[2], fm[2];
} wtrk;
static wtrk W[GB_NTRACK];

static void wheel_block(const gb_patch *p, int t, float beat)
{
    (void)beat;
    wtrk *T = &W[t];
    int perc = gb_ev(p, 4), rot = gb_ev(p, 7), reg = gb_ev(p, 0);
    float s = 0.0f, lv[NP];
    for (int k = 0; k < NP; k++) {
        int d = REG[reg][k] - '0' + (k < 2 ? gb_ev(p, 1) : k < 4 ? gb_ev(p, 2) : gb_ev(p, 3));
        d = d < 0 ? 0 : d > 8 ? 8 : d;
        lv[k] = perc && k == NP - 1 ? 0.0f : LV[d];                   /* PERC on: the 1' bar is off */
        s += lv[k];
    }
    float norm = 2.4f / (s + 1.5f);                                   /* full bars louder than one, never clipping */
    if (perc == 1 || perc == 2 || perc == 5 || perc == 6) norm *= 0.7071f;   /* normal percussion: the bars -3 dB */
    for (int k = 0; k < NP; k++) T->g[k] = lv[k] * norm;
    T->pk = perc ? 3 + ((perc - 1) & 1) : NP;                         /* 2ND: 4', 3RD: 2 2/3' */
    T->plev = perc == 3 || perc == 4 ? 0.366f : 0.732f;
    T->pdec = gb_exp2f(-1.442695f * (float)GB_BLOCK / ((perc >= 5 ? 0.6f : 0.15f) * GB_SR));
    T->clk = (float)gb_ev(p, 5) * (180.0f / 32767.0f);
    int dr = gb_ev(p, 6);
    T->dg = dr ? 1.0f + (float)dr * (6.0f / 256.0f) : 0.0f;
    T->dcomp = 6.0f / (2.0f + T->dg);
    static const float SPD[2][3] = { { 0.0f, 0.8f, 6.7f }, { 0.0f, 0.7f, 5.8f } };   /* the high rotor, the low */
    for (int k = 0; k < 2; k++) {
        float ease = (float)GB_BLOCK / ((k ? 1.5f : 0.37f) * GB_SR);
        T->rsp[k] = gb_flush(T->rsp[k] + (SPD[k][rot] - T->rsp[k]) * gb_minf(ease, 1.0f));
        T->rph[k] += T->rsp[k] * (float)GB_BLOCK * GB_ISR; T->rph[k] -= (float)(int)T->rph[k];
        float sn = gb_sin_turn(T->rph[k]), c = gb_sin_turn(T->rph[k] + 0.25f), dep = k ? 0.21f : 0.4f;
        T->am[k] = 1.0f - dep * 0.5f * (1.0f - c);
        T->fm[k] = sn * (k ? 0.002f / 5.8f : 0.005f / 6.7f) * T->rsp[k];
    }
    T->rwet = gb_flush(T->rwet + ((rot ? 1.0f : 0.0f) - T->rwet) * gb_minf((float)GB_BLOCK / (0.025f * GB_SR), 1.0f));
    for (int k = 0; k < 2; k++) { T->am[k] = 1.0f - T->rwet * (1.0f - T->am[k]); T->fm[k] *= T->rwet; }
}

GB_SMALL static void wheel_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)p;
    if (fresh) {                                                      /* the wheels: anywhere (free running) */
        for (int k = 0; k < NP; k++) { v->s.wheel.ph[k] = (uint32_t)k * 0x9E3779B9u ^ v->seed; v->s.wheel.gp[k] = 0.0f; }
        v->s.wheel.nz = v->seed;
    }
    v->s.wheel.perc = v->others ? 0.0f : 1.0f;                        /* single trigger */
    v->s.wheel.clk = 1.0f; v->s.wheel.hp = 0.0f; v->s.wheel.gate = 1;
}
static void wheel_spread(gb_voice *v, uint32_t r) { for (int k = 0; k < NP; k++) v->s.wheel.ph[k] += r * (uint32_t)(k + 1); }

GB_HOT static int wheel_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    const wtrk *T = &W[v->track];
    float acc[GB_BLOCK];
    for (int n = 0; n < GB_BLOCK; n++) acc[n] = 0.0f;
    float perc1 = v->s.wheel.perc * T->pdec;
    for (int k = 0; k < NP; k++) {
        float dt = m->dt;
        while (dt * RATIO[k] > 0.136f) dt *= 0.5f;                   /* fold back above ~6 kHz */
        float pdt = dt * RATIO[k];
        int r = pdt < 800.0f * GB_ISR;                                /* 1: the low rotor */
        float g1 = (T->g[k] + (k == T->pk ? T->plev * perc1 : 0.0f)) * T->am[r], g0 = v->s.wheel.gp[k];
        pdt *= 1.0f + T->fm[r];
        uint32_t ph = v->s.wheel.ph[k], inc = (uint32_t)(pdt * 4294967296.0f);
        v->s.wheel.gp[k] = g1;
        if (g0 == 0.0f && g1 == 0.0f) { v->s.wheel.ph[k] = ph + inc * GB_BLOCK; continue; }
        float g = g0, dg = (g1 - g0) * (1.0f / GB_BLOCK);
        GB_UNROLL4
        for (int n = 0; n < GB_BLOCK; n++) { ph += inc; g += dg; acc[n] += gb_sin_u32(ph) * g; }
        v->s.wheel.ph[k] = ph;
    }
    v->s.wheel.perc = perc1;
    if (v->s.wheel.gate && v->state != V_ON) { v->s.wheel.gate = 0; v->s.wheel.clk = 0.5f; }   /* the release's click */
    float c0 = v->s.wheel.clk * T->clk;
    if (c0 > 0.0005f) {                                               /* the key click: high-passed noise, ~2.5 ms */
        v->s.wheel.clk *= 0.75f * 0.75f;
        float c1 = v->s.wheel.clk * T->clk, dc = (c1 - c0) * (1.0f / GB_BLOCK), hp = v->s.wheel.hp;
        uint32_t r = v->s.wheel.nz;
        for (int n = 0; n < GB_BLOCK; n++) {
            r ^= r << 13; r ^= r >> 17; r ^= r << 5;
            float z = (float)(int32_t)r * (0.5f / 2147483648.0f), y = z - hp;
            hp += y * 0.125f; c0 += dc; acc[n] += y * c0;
        }
        v->s.wheel.nz = r; v->s.wheel.hp = hp;
    }
    if (T->dg > 0.0f) { float g = T->dg * 0.5f, c = T->dcomp; for (int n = 0; n < GB_BLOCK; n++) acc[n] = gb_softclip(acc[n] * g) * c; }
    for (int n = 0; n < GB_BLOCK; n++) out[n] = acc[n] * 0.5f;
    return 1;
}

const gb_engine gb_eng_wheel = { "WHEEL", ED, 0, 0, wheel_start, wheel_render, 0, wheel_spread, wheel_block };
