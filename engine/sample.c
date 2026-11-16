/* SPDX-License-Identifier: GPL-3.0-only
 * SAMPLE: SLOOP's sample sets (docs/sloop/SPEC-MOTEURS.md), written for this project. The note's zone of the set,
 * read at its pitch (linear between the decoded samples), looped where the set loops (LOOP), fewer bits (BITS), a
 * drive, a gentle low-pass (CUT). A note played again starts the sample over (the old one fades in a tail).
 *   E0 SET  the factory sets, then USR1..USR4   E1 TUNE +-24   E2 BITS   E3 LOOP OFF ON   E4 CUT   E6 DRV */
#include "engine.h"
#include "sample.h"
#include "slice.h"

const uint8_t *gb_bank;
volatile uint8_t gb_flash_ok = 1;
static int bank_ok;
const int16_t smp_ima_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
    1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 };
const int8_t smp_ima_idx[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const smp_head *H(void) { return (const smp_head *)(const void *)gb_bank; }
GB_SMALL void smp_bank_set(const uint8_t *img)
{
    const smp_head *h = (const smp_head *)(const void *)img;
    gb_bank = img; bank_ok = 0;
    for (int k = 0; k < SLC_SRCS; k++) slice_drop(k);          /* (SLICE's tables: in RAM written before read) */
    if (!img || h->magic != SMP_MAGIC || h->ver != 4 || !h->nsets || h->nsets > 32 || h->nzones > 512) return;
    if (h->data_off < 32u + 16u * h->nsets + 32u * h->nzones + 4u * h->nseek) return;
    const smp_zone *z = (const smp_zone *)(const void *)(img + 32 + 16 * h->nsets);
    for (int i = 0; i < h->nzones; i++)
        if (z[i].off + (z[i].n + 1u) / 2u > h->data_len || z[i].idx > 88 || !z[i].rate || z[i].ls > z[i].le) return;
    bank_ok = 1;
    slice_scan(0);                                             /* (SLICE's BREAK: its hits) */
#ifndef __PI32V2__
    slice_scan_all();                                          /* (the PC: at once) */
#endif
}
int smp_bank_ok(void) { return bank_ok; }
#ifndef __PI32V2__
#include <stdio.h>
int smp_bank_load(const char *path)
{
    static uint8_t img[262144];
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t n = fread(img, 1, sizeof img, f); fclose(f);
    (void)n; smp_bank_set(img);
    return bank_ok;
}
#endif
int smp_nsets(void)
{
    if (!bank_ok) return 0;
    int n = H()->nsets;
    while (n > 0 && smp_set_at(n - 1)->kind == SK_SLICE) n--;     /* (BREAK, last: SLICE's only) */
    return n;
}
int smp_break_zone(void)
{
    if (!bank_ok) return -1;
    for (int k = H()->nsets - 1; k >= 0 && smp_set_at(k)->kind == SK_SLICE; k--) if (smp_set_at(k)->nz) return smp_set_at(k)->z0;
    return -1;
}
const smp_set *smp_set_at(int k) { return (const smp_set *)(const void *)(gb_bank + 32 + 16 * k); }
const smp_zone *smp_zone_at(int k) { return (const smp_zone *)(const void *)(gb_bank + 32 + 16 * H()->nsets + 28 * k); }
const uint8_t *smp_data(void) { return gb_bank + H()->data_off; }
/* ---- the user slots */
static const uint8_t *usr[SMP_NUSR];
static uint8_t usr_n[SMP_NUSR];
int smp_user_head_ok(const uint8_t *s)
{
    uint32_t magic = s[0] | s[1] << 8 | s[2] << 16 | (uint32_t)s[3] << 24, len = s[16] | s[17] << 8 | s[18] << 16 | (uint32_t)s[19] << 24;
    int nz = s[6];
    if (magic != 0x504D5346u || (s[4] | s[5] << 8) != 1 || !nz || nz > 16 || len > SMP_USR_SIZE - 512u) return 0;
    for (int i = 0; i < nz; i++) {
        const uint8_t *q = s + 32 + 28 * i; smp_zone z;
        for (uint32_t j = 0; j < sizeof z; j++) ((uint8_t *)&z)[j] = q[j];
        if (z.off > len || z.n > 2u * SMP_USR_SIZE || z.off + (z.n + 1u) / 2u > len || z.idx > 88 || !z.rate ||
            z.rate > 4u << 16 || z.ls > z.le || z.le >= z.n || z.lo > z.hi) return 0;
    }
    return nz;
}
GB_SMALL void smp_user_scan(int k, const uint8_t *s)
{
    usr_n[k] = 0; usr[k] = s; slice_drop(k + 1);
    if (!s) return;
    uint32_t magic = s[0] | s[1] << 8 | s[2] << 16 | (uint32_t)s[3] << 24, len = s[16] | s[17] << 8 | s[18] << 16 | (uint32_t)s[19] << 24;   /* (magic, version, zones, a byte, a name of 8, the data's length) */
    int nz = s[6];
    if (magic != 0x504D5346u || (s[4] | s[5] << 8) != 1 || !nz || nz > 16 || len > SMP_USR_SIZE - 512u) return;   /* "FSMP" 1 */
    for (int i = 0; i < nz; i++) {
        const smp_zone *z = (const smp_zone *)(const void *)(s + 32 + 28 * i);
        if (z->off > len || z->n > 2u * SMP_USR_SIZE || z->off + (z->n + 1u) / 2u > len || z->idx > 88 || !z->rate ||
            z->rate > 4u << 16 || z->ls > z->le || z->le >= z->n || z->lo > z->hi) return;
    }
    usr_n[k] = (uint8_t)nz;
    slice_scan(k + 1);                                         /* (SLICE on this slot: its states and hits; the main loop scans) */
#ifndef __PI32V2__
    slice_scan_all();
#endif
}
int smp_user_nz(int k) { return k >= 0 && k < SMP_NUSR ? usr_n[k] : 0; }
const char *smp_user_name(int k, char *b)
{
    int i = 0;
    if (usr_n[k]) for (; i < 8 && usr[k][8 + i] >= 32 && usr[k][8 + i] < 127; i++) b[i] = (char)(usr[k][8 + i] >= 'a' && usr[k][8 + i] <= 'z' ? usr[k][8 + i] - 32 : usr[k][8 + i]);
    b[i] = 0;
    return b;
}
void smp_user_drop(int k) { if (k >= 0 && k < SMP_NUSR) { usr_n[k] = 0; slice_drop(k + 1); } }
const smp_zone *smp_zone_of(uint32_t zi)
{
    static const smp_zone none = { 0, 0, 0, 0, 1u << 16, 960, 0, 0, 0, 127, 0 };   /* (a slot being written: nothing) */
    if (zi & SMP_USR) return (zi & 31u) < usr_n[(zi >> 5) & 3u] ? (const smp_zone *)(const void *)(usr[(zi >> 5) & 3u] + 32 + 28 * (zi & 31u)) : &none;
    return smp_zone_at((int)zi);
}
const uint8_t *smp_zone_data(uint32_t zi) { return zi & SMP_USR ? usr[(zi >> 5) & 3u] + 512 : smp_data(); }
int smp_find(int set, int note)
{
    int zi = -1;
    if (set >= 0 && set < smp_nsets()) {
        const smp_set *s = smp_set_at(set);
        for (int i = 0; i < s->nz; i++) { const smp_zone *z = smp_zone_at(s->z0 + i); if (note >= z->lo && note <= z->hi) zi = s->z0 + i; }
        if (zi < 0 && s->kind != SK_KIT) zi = s->z0;                 /* (a kit's key with no sound stays silent) */
        return zi;
    }
    int k = set - smp_nsets();
    if (k < 0 || k >= SMP_NUSR || !usr_n[k]) return -1;
    for (int i = 0; i < usr_n[k]; i++) { const smp_zone *z = smp_zone_of(SMP_USR | (uint32_t)k << 5 | (uint32_t)i); if (note >= z->lo && note <= z->hi) zi = (int)(SMP_USR | (uint32_t)k << 5 | (uint32_t)i); }
    return zi >= 0 ? zi : (int)(SMP_USR | (uint32_t)k << 5);
}
uint32_t smp_seek(int zone, uint32_t k)
{
    const smp_head *h = H();
    const uint32_t *first = (const uint32_t *)(const void *)(gb_bank + 32 + 16 * h->nsets + 28 * h->nzones);
    return first[h->nzones + first[zone] + k];
}

static const gb_pd ED[8] = { { 0, 11, 0 }, { -24, 24, 0 }, { 0, 127, 0 }, { 0, 1, 1 }, { 0, 127, 127 }, { 0, 0, 0 },
                             { 0, 127, 0 }, { 0, 0, 0 } };
#define SS(v) (v)->s.smp
static void smp_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)fresh;
    int set = gb_ev(p, 0), zi;
    SS(v).zone = 0xFFFF; SS(v).ended = 1;
    if (!bank_ok || !gb_flash_ok) return;                      /* (a save: silent) */
    if ((zi = smp_find(set, v->note)) < 0) return;
    SS(v).zone = (uint16_t)zi; SS(v).ended = 0;
    SS(v).pos = 0; SS(v).frac = 0; SS(v).pred = 0; SS(v).idx = 0; SS(v).s0 = SS(v).s1 = 0.0f; SS(v).lp = 0.0f; SS(v).primed = 0;
}
/* the next decoded sample (and the loop) */
GB_INLINE float next(const smp_zone *z, const uint8_t *d, gb_voice *v, int loop)
{
    uint32_t pos = SS(v).pos, b = d[z->off + (pos >> 1)], code = (pos & 1u) ? b >> 4 : b & 15u;
    int step = smp_ima_step[SS(v).idx], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    int pr = SS(v).pred + ((code & 8u) ? -vd : vd);
    SS(v).pred = (int16_t)(pr < -32768 ? -32768 : pr > 32767 ? 32767 : pr);
    int ix = SS(v).idx + smp_ima_idx[code & 7u]; SS(v).idx = (uint8_t)(ix < 0 ? 0 : ix > 88 ? 88 : ix);
    pos++;
    if (pos > z->le && z->looped && loop) { pos = z->ls; SS(v).pred = z->pred; SS(v).idx = z->idx; }
    SS(v).pos = pos;
    return (float)SS(v).pred * (1.0f / 32768.0f);
}
GB_HOT static int smp_render(gb_voice *v, const gb_patch *p, const gb_mod *m, float *out)
{
    for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f;
    if (SS(v).ended || !bank_ok) return 0;
    if (!gb_flash_ok) return 1;                                /* (a save: the flash is not to be read; silent, waiting) */
    const smp_zone *z = smp_zone_of(SS(v).zone);
    const uint8_t *d = smp_zone_data(SS(v).zone);
    int loop = gb_ev(p, 3) && (z->looped != 2 || v->state == V_ON), bits = gb_ev(p, 2) / 10, drv = gb_ev(p, 6);   /* (a user slot's LOOP+TAIL: loops while held, then on to its end) */
    float st = gb_exp2f(gb_clampf(m->pitch + (float)gb_ev(p, 1) - (float)z->root16 * (1.0f / 16.0f), -96.0f, 96.0f) * (1.0f / 12.0f))
             * (float)z->rate * (1.0f / 65536.0f);
    st = gb_minf(st, 8.0f);                                    /* (the decoding bounded: 8 a sample) */
    float k = (4000.0f + gb_clampf((float)gb_ev(p, 4) + m->cut, 0.0f, 127.0f) * (28767.0f / 127.0f)) * (1.0f / 32768.0f);
    float q = bits ? (float)(1 << bits) * (1.0f / 32768.0f) : 0.0f, dg = (float)drv * (600.0f / 32768.0f);
    if (!SS(v).primed) { SS(v).s1 = next(z, d, v, loop); SS(v).primed = 1; }
    float fr = SS(v).frac, s0 = SS(v).s0, s1 = SS(v).s1, lp = SS(v).lp;
    int ended = 0;
    for (int i = 0; i < GB_BLOCK; i++) {
        fr += st;
        while (fr >= 1.0f) {
            fr -= 1.0f; s0 = s1;
            if (SS(v).pos >= z->n) { ended = 1; s1 = 0.0f; break; }
            s1 = next(z, d, v, loop);
        }
        float s = s0 + (s1 - s0) * fr;
        if (q > 0.0f) s = (float)(int)(s / q) * q;
        if (dg > 0.0f) s = gb_softclip(s + s * dg);
        lp += (s + GB_BIAS - lp) * k;
        out[i] = lp * 0.5f;
        if (ended) break;
    }
    SS(v).frac = fr; SS(v).s0 = s0; SS(v).s1 = s1; SS(v).lp = lp; SS(v).ended = (uint8_t)ended;
    return !ended;
}
const gb_engine gb_eng_sample = { "SAMPLE", ED, 0, 2, smp_start, smp_render, 0, 0, 0 };
