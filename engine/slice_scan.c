/* SPDX-License-Identifier: GPL-3.0-only
 * SLICE's tables (slice.h): run in the main loop, from the flash window (fw/ram.ld .xtext), when a material comes
 * (the start, a user slot written by the editor). Two passes over its first sample:
 *   1. the largest step between two samples (what a hat, a snare, a click make) and the largest sample;
 *   2. hops of 32 samples: a user slot's decoder states at 32 points; a hit where a hop's step peak (or its plain
 *      peak: low hits) rises past 1.75 x its follower (decaying a sixteenth / a sixty-fourth a hop: a low tone's
 *      cycles are not rises), above 1/12 (1/8) of the largest, at least 50 ms after the last hit. The slice starts
 *      a hop before (a little pre-roll, never after the hit).
 * Written for this project from SLOOP's description of its own slicer (its SLICE, facts only). */
#include "engine.h"
#include "sample.h"
#include "slice.h"

static int dec(const uint8_t *d, uint32_t off, uint32_t pos, int *pred, int *idx)
{
    uint32_t b = d[off + (pos >> 1)], code = (pos & 1u) ? b >> 4 : b & 15u;
    int step = smp_ima_step[*idx], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    int pr = *pred + ((code & 8u) ? -vd : vd);
    *pred = pr < -32768 ? -32768 : pr > 32767 ? 32767 : pr;
    int ix = *idx + smp_ima_idx[code & 7u]; *idx = ix < 0 ? 0 : ix > 88 ? 88 : ix;
    return *pred;
}

/* the scan, a slice of it at a time: the main loop calls slice_scan_poll (a user slot written by the editor: no
 * pause in the sound); slice_scan_all finishes them at once (the start, before the sound) */
static struct { int8_t src; uint8_t pass, pending; uint32_t pos, len, k, last, h0, gap; int pred, idx, prev, pkd, pkr, thd, thr, envd, envr; } SC = { .src = -1 };
void slice_scan(int src)
{
    if (src < 0 || src >= SLC_SRCS) return;
    slc_tabs[src].len = 0; slc_tabs[src].non = 0;
    SC.pending |= (uint8_t)(1u << src);
    if (SC.src == src) SC.src = -1;                        /* (begun again) */
}
#define CHUNK 8192u
int slice_scan_poll(void)                                  /* 1 while there is scanning left */
{
    if (SC.src < 0) {
        if (!SC.pending) return 0;
        int s = 0; while (!((SC.pending >> s) & 1u)) s++;
        SC.pending &= (uint8_t)~(1u << s);
        int zi = slice_zone(s);
        if (zi < 0) return SC.pending != 0;
        const smp_zone *z = smp_zone_of((uint32_t)zi);
        if (s >= 1 && smp_user_nz(s - 1) >= 2) {           /* (3.3.6) zones in order: they are the slices, nothing to scan */
            int nz = smp_user_nz(s - 1); uint32_t end = 0, ok = 1;
            for (int k = 0; k < nz && ok; k++) { const smp_zone *q = smp_zone_of(SMP_USR | (uint32_t)(s - 1) << 5 | (uint32_t)k);
                if (q->n < 64u || (k && q->off < end)) ok = 0;
                end = q->off + (q->n + 1u) / 2u;
            }
            if (ok) { slc_tab *t = &slc_tabs[s]; t->non = (uint8_t)nz; t->brk = (uint8_t)nz; t->len = 2u * (end - z->off); return SC.pending != 0; }
        }
        slc_tabs[s].brk = 0;
        if (z->n < 64u || z->n > 4u * 65535u) return SC.pending != 0;
        SC.src = (int8_t)s; SC.pass = 1; SC.pos = 0; SC.len = z->n; SC.pred = SC.idx = SC.prev = SC.pkd = SC.pkr = 0;
    }
    slc_tab *t = &slc_tabs[SC.src];
    if (t->non == 0xFF) { SC.src = -1; return SC.pending != 0; }   /* (its material dropped: slice_drop) */
    int zi = slice_zone(SC.src);
    if (zi < 0) { SC.src = -1; return 1; }
    const smp_zone *z = smp_zone_of((uint32_t)zi);
    const uint8_t *d = smp_zone_data((uint32_t)zi);
    if (z->n != SC.len) { SC.src = -1; return 1; }         /* (changed under us) */
    uint32_t len = SC.len, end = len - SC.pos > CHUNK ? SC.pos + CHUNK : len;
    if (SC.pass == 1) {                                    /* 1: the largest step, the largest sample */
        for (; SC.pos < end; SC.pos++) {
            int x = dec(d, z->off, SC.pos, &SC.pred, &SC.idx), a = x - SC.prev; a = a < 0 ? -a : a;
            if (a > SC.pkd) SC.pkd = a;
            a = x < 0 ? -x : x; if (a > SC.pkr) SC.pkr = a;
            SC.prev = x;
        }
        if (SC.pos < len) return 1;
        SC.pass = 2; SC.pos = 0; SC.pred = SC.idx = SC.prev = 0; SC.envd = SC.envr = 0; SC.k = 0; SC.last = 0; SC.h0 = 0;
        SC.thd = SC.pkd / 12; SC.thr = SC.pkr / 8;
        SC.gap = (uint32_t)((uint64_t)z->rate * 44100u >> 16) / 20u;
        t->non = 1; t->on[0] = 0;
        return 1;
    }
    end = (end + 31u) & ~31u; if (end > len) end = len;   /* (whole hops) */
    while (SC.pos < end) {                                 /* 2: the states, the hits, a hop of 32 at a time */
        uint32_t hs = SC.pos, e = len - SC.pos > 32u ? SC.pos + 32u : len;
        int hd = 0, hr = 0;
        for (; SC.pos < e; SC.pos++) {
            while (SC.k < SLC_GRID && (uint32_t)((uint64_t)SC.k * len / SLC_GRID) == SC.pos) t->grid[SC.k++] = (uint32_t)(uint16_t)SC.pred | (uint32_t)SC.idx << 16;
            int x = dec(d, z->off, SC.pos, &SC.pred, &SC.idx), a = x - SC.prev; a = a < 0 ? -a : a;
            if (a > hd) hd = a;
            a = x < 0 ? -x : x; if (a > hr) hr = a;
            SC.prev = x;
        }
        if (hs && ((hd > SC.thd && hd * 4 > SC.envd * 7) || (hr > SC.thr && hr * 4 > SC.envr * 7)) && SC.h0 >= SC.last + SC.gap && t->non < SLC_AUTO) {
            t->on[t->non++] = (uint16_t)(SC.h0 / 4u);    /* (from the hop before; on a multiple of 4) */
            SC.last = SC.h0;
        }
        SC.envd = hd > SC.envd ? hd : SC.envd - SC.envd / 16;
        SC.envr = hr > SC.envr ? hr : SC.envr - SC.envr / 64;
        SC.h0 = hs;
    }
    if (SC.pos < len) return 1;
    t->len = len; SC.src = -1;
    return SC.pending != 0;
}
void slice_scan_all(void) { while (slice_scan_poll()) { } }
