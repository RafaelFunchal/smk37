/* SPDX-License-Identifier: GPL-3.0-only
 * SLICE (slice.h): the voices. Note n plays slice (n - 60 + START) mod the slice count: the lowest key is slice 0.
 *   E0 SRC   BREAK USR1..USR4 (an empty slot: BREAK)     E1 DIV   4 8 16 32 AUTO      E2 START  0..31: the first slice
 *   E3 PITCH -24..+24 semitones (varispeed: faster is higher; the note does not change it)
 *   E4 MODE  ONE (to the slice's end, the key's release ignored), GATE (until released: the ADSR), LOOP (while held)
 *   E5 REV   backwards          E6 DECAY  a fade over the slice (127: none)          E7 TONE   a gentle low-pass
 * A slice starts after its decoder state is found: the nearest known state before it (512 samples away at most in
 * the bank, a 32nd of a user slot), decoded forwards, 4096 samples a block at most (silent meanwhile: a few ms at
 * worst). Backwards, that first pass runs on through the slice and keeps its state at 20 points: a window of 40
 * samples is decoded forwards from the nearest one and read from its top down. A new slice fades in over a block;
 * the last 1.5 ms of one fade out (no click at the cut). */
#include "engine.h"
#include "sample.h"
#include "slice.h"

slc_tab slc_tabs[SLC_SRCS] GB_LOWRAM;                      /* (written by slice_scan, len 0 until then: cleared at the start, fw/main.c) */
static const gb_pd ED[8] = { { 0, 4, 0 }, { 0, 4, 2 }, { 0, 31, 0 }, { -24, 24, 0 }, { 0, 2, 0 }, { 0, 1, 0 }, { 0, 127, 127 }, { 0, 127, 127 } };
enum { PH_DEAD, PH_SEEK, PH_PLAY };
enum { M_ONE, M_GATE, M_LOOP };
#define SV(v) (v)->s.slc
#define SEEK_BUDGET 4096
#define RB 40
#define NCK 20
GB_INLINE uint32_t ck_pos(const gb_voice *v, int k) { return SV(v).a + (uint32_t)((uint64_t)(SV(v).b - SV(v).a) * (uint32_t)k / NCK); }
_Static_assert(sizeof(((gb_voice *)0)->s.slc) <= 232, "a SLICE voice in the voice's state");

int slice_zone(int src)
{
    if (src <= 0) return smp_break_zone();
    if (src > SMP_NUSR || !smp_user_nz(src - 1)) return -1;
    return (int)(SMP_USR | (uint32_t)(src - 1) << 5);
}
void slice_drop(int src) { if (src >= 0 && src < SLC_SRCS) { slc_tabs[src].len = 0; slc_tabs[src].non = 0xFF; slc_tabs[src].brk = 0; } }   /* (0xFF: a scan under way stops) */

/* the decoder, one sample (the bank's IMA ADPCM, sample.c's tables) */
__attribute__((noinline)) static int dec1(const uint8_t *d, uint32_t off, uint32_t pos, int16_t *pred, uint8_t *idx)
{
    uint32_t b = d[off + (pos >> 1)], code = (pos & 1u) ? b >> 4 : b & 15u;
    int step = smp_ima_step[*idx], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    int pr = *pred + ((code & 8u) ? -vd : vd);
    *pred = (int16_t)(pr < -32768 ? -32768 : pr > 32767 ? 32767 : pr);
    int ix = *idx + smp_ima_idx[code & 7u]; *idx = (uint8_t)(ix < 0 ? 0 : ix > 88 ? 88 : ix);
    return *pred;
}
/* the nearest known decoder state at or before pos: its position, its state */
static uint32_t known_state(int src, uint32_t zi, uint32_t len, uint32_t pos, uint32_t *st)
{
    if (!(zi & SMP_USR)) { uint32_t k = pos >> SMP_SEEK_SHIFT; *st = smp_seek((int)zi, k); return k << SMP_SEEK_SHIFT; }
    uint32_t k = (uint32_t)((uint64_t)pos * SLC_GRID / len), at;
    if (k >= SLC_GRID) k = SLC_GRID - 1;
    while (k && (at = (uint32_t)((uint64_t)k * len / SLC_GRID)) > pos) k--;
    *st = slc_tabs[src].grid[k];
    return (uint32_t)((uint64_t)k * len / SLC_GRID);
}

GB_SMALL static void slc_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)fresh;
    SV(v).phase = PH_DEAD;
    if (!smp_bank_ok() || !gb_flash_ok) return;
    int src = gb_ev(p, 0), zi = slice_zone(src);
    if (zi < 0 || !slc_tabs[src].len) { src = 0; zi = slice_zone(0); }
    if (zi < 0 || !slc_tabs[src].len) return;
    const slc_tab *t = &slc_tabs[src];
    uint32_t len = t->len, div = (uint32_t)gb_ev(p, 1), n = div < 4u ? 4u << div : (t->non ? t->non : 1u), a, b;
    if (t->brk) n = t->brk;                               /* (3.3.6) its zones: slice j is zone j, from its start */
    int j = ((int)v->note - 60 + gb_ev(p, 2)) % (int)n; if (j < 0) j += (int)n;
    if (t->brk) {
        uint32_t zj = (uint32_t)zi | (uint32_t)j; const smp_zone *q = smp_zone_of(zj);
        if (q->n < 16u) return;
        SV(v).zone = (uint16_t)zj; SV(v).src = (uint8_t)src; a = 0; b = q->n; SV(v).a = a; SV(v).b = b; SV(v).pos = 0; SV(v).pred = 0; SV(v).idx = 0;
        goto go;
    }
    if (div < 4u) { a = (uint32_t)((uint64_t)len * (uint32_t)j / n); b = (uint32_t)((uint64_t)len * (uint32_t)(j + 1) / n); }
    else { a = (uint32_t)t->on[j] * 4u; b = (uint32_t)j + 1u < t->non ? (uint32_t)t->on[j + 1] * 4u : len; }
    if (b > len) b = len;
    if (b <= a + 16u) return;
    uint32_t st, at = known_state(src, (uint32_t)zi, len, a, &st);
    SV(v).zone = (uint16_t)zi; SV(v).src = (uint8_t)src; SV(v).a = a; SV(v).b = b;
    SV(v).pos = at; SV(v).pred = (int16_t)(st & 0xFFFFu); SV(v).idx = (uint8_t)((st >> 16) & 127u);
go:
    SV(v).rev = (uint8_t)gb_ev(p, 5); SV(v).mode = (uint8_t)gb_ev(p, 4);
    SV(v).tgt = SV(v).rev ? b : a;                       /* backwards: on through the slice, keeping its states */
    SV(v).frac = 0; SV(v).s0 = SV(v).s1 = 0; SV(v).lp = 0; SV(v).dec = 1; SV(v).ramp = 0;
    for (int k = 0; k < NCK; k++) SV(v).ck[k] = 0xFFFFFFFFu;
    SV(v).ws = 0xFFFFFFFFu;
    SV(v).phase = PH_SEEK;
}
static void slc_release(gb_voice *v, const gb_patch *p)
{
    if (gb_ev(p, 4) == M_ONE && SV(v).phase != PH_DEAD) v->eg = EG_DEC;   /* ONE: to the slice's end */
}

/* backwards: sample q (a <= q < b), through the window */
static float back_at(gb_voice *v, const smp_zone *z, const uint8_t *d, uint32_t q)
{
    if (q < SV(v).ws || q >= SV(v).ws + RB) {
        uint32_t a = SV(v).a, ws = q + 1u >= a + RB ? q + 1u - RB : a;
        int k = NCK - 1; while (k > 0 && (SV(v).ck[k] == 0xFFFFFFFFu || ck_pos(v, k) > ws)) k--;
        uint32_t pos = ck_pos(v, k), st = SV(v).ck[k]; int16_t pr = (int16_t)(st & 0xFFFFu); uint8_t ix = (uint8_t)(st >> 16);
        while (pos < ws) dec1(d, z->off, pos++, &pr, &ix);
        for (int i = 0; i < RB; i++) SV(v).rb[i] = (int16_t)(pos < SV(v).b ? dec1(d, z->off, pos++, &pr, &ix) : 0);
        SV(v).ws = ws;
    }
    return (float)SV(v).rb[q - SV(v).ws] * (1.0f / 32768.0f);
}

static int slc_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f;
    if (SV(v).phase == PH_DEAD) { v->eg = EG_IDLE; return 0; }
    if (!gb_flash_ok) return 1;                            /* (a save: the flash is not to be read; silent, waiting) */
    const smp_zone *z = smp_zone_of(SV(v).zone);
    const uint8_t *d = smp_zone_data(SV(v).zone);
    if (!slc_tabs[SV(v).src].len || z->n < SV(v).b) { SV(v).phase = PH_DEAD; v->eg = EG_IDLE; return 0; }   /* (its slot rewritten) */
    if (SV(v).phase == PH_SEEK) {
        uint32_t pos = SV(v).pos, tgt = SV(v).tgt, a = SV(v).a, len = SV(v).b - a; int left = SEEK_BUDGET;
        int16_t pr = SV(v).pred; uint8_t ix = SV(v).idx;
        while (pos < tgt && left--) {
            if (pos >= a && SV(v).rev) {                   /* backwards: the states along the slice */
                uint32_t k = (uint32_t)((uint64_t)(pos - a) * NCK / len);
                if (k < NCK && SV(v).ck[k] == 0xFFFFFFFFu && ck_pos(v, (int)k) == pos) SV(v).ck[k] = (uint32_t)(uint16_t)pr | (uint32_t)ix << 16;
                else if (k + 1u < NCK && SV(v).ck[k + 1u] == 0xFFFFFFFFu && ck_pos(v, (int)k + 1) == pos) SV(v).ck[k + 1u] = (uint32_t)(uint16_t)pr | (uint32_t)ix << 16;
            }
            dec1(d, z->off, pos++, &pr, &ix);
        }
        SV(v).pos = pos; SV(v).pred = pr; SV(v).idx = ix;
        if (pos < tgt) return 1;                           /* (on next block) */
        if (!SV(v).rev) { SV(v).apred = pr; SV(v).aidx = ix; SV(v).s1 = 0; }
        else { if (SV(v).ck[0] == 0xFFFFFFFFu) { SV(v).phase = PH_DEAD; v->eg = EG_IDLE; return 0; } SV(v).pos = SV(v).b; }
        SV(v).phase = PH_PLAY;
    }
    /* playing: the material's rate x PITCH (the note does not change it: its bends, LFO, glide do) */
    float semis = m->pitch - (float)v->note + (float)gb_ev(p, 3);
    float st = gb_exp2f(gb_clampf(semis, -48.0f, 36.0f) * (1.0f / 12.0f)) * (float)z->rate * (1.0f / 65536.0f);
    st = gb_minf(st, 8.0f);
    float k = (4000.0f + gb_clampf((float)gb_ev(p, 7) + m->cut, 0.0f, 127.0f) * (28767.0f / 127.0f)) * (1.0f / 32768.0f);
    int dv = gb_ev(p, 6);
    float dk = dv >= 127 ? 1.0f : gb_exp2f(-1.0f / (gb_time_ms((float)dv) * 0.001f * GB_SR) * 1.442695f);
    int loop = SV(v).mode == M_LOOP && v->state == V_ON;
    float fr = SV(v).frac, s0 = SV(v).s0, s1 = SV(v).s1, lp = SV(v).lp, dc = SV(v).dec, rp = SV(v).ramp;
    uint32_t a = SV(v).a, b = SV(v).b, pos = SV(v).pos;
    int16_t pr = SV(v).pred; uint8_t ix = SV(v).idx;
    int ended = 0;
    float fade_n = 0.0015f * GB_SR * st;                   /* the last 1.5 ms (in material samples) */
    for (int i = 0; i < GB_BLOCK; i++) {
        fr += st;
        while (fr >= 1.0f) {
            fr -= 1.0f; s0 = s1;
            if (!SV(v).rev) {
                if (pos >= b) {
                    if (!loop) { ended = 1; break; }
                    pos = a; pr = SV(v).apred; ix = SV(v).aidx; dc = 1.0f;
                }
                s1 = (float)dec1(d, z->off, pos++, &pr, &ix) * (1.0f / 32768.0f);
            } else {
                if (pos <= a) {
                    if (!loop) { ended = 1; break; }
                    pos = b; dc = 1.0f;
                }
                pos--;
                s1 = back_at(v, z, d, pos);
            }
        }
        if (ended) break;
        float left = SV(v).rev ? (float)(pos - a) : (float)(b - pos), g = rp < 1.0f ? rp : 1.0f;
        if (!loop && left < fade_n) g *= left / fade_n;
        rp += 1.0f / GB_BLOCK;
        float s = (s0 + (s1 - s0) * fr) * dc * g;
        dc *= dk;
        lp += (s + GB_BIAS - lp) * k;
        out[i] = lp * 0.5f;
    }
    SV(v).frac = fr; SV(v).s0 = s0; SV(v).s1 = s1; SV(v).lp = lp; SV(v).dec = dc; SV(v).ramp = rp;
    SV(v).pos = pos; SV(v).pred = pr; SV(v).idx = ix;
    if (ended || dc < 1e-4f) { SV(v).phase = PH_DEAD; v->eg = EG_IDLE; return 0; }
    return 1;
}
const gb_engine gb_eng_slice = { "SLICE", ED, 0, 2, slc_start, slc_render, slc_release, 0, 0 };
