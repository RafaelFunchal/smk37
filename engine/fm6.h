/* SPDX-License-Identifier: GPL-3.0-only
 * FM6's patches (fm6.c): the classic six-operator voice, its 155 values as named fields (OP1 first). */
#pragma once
#include <stdint.h>
typedef struct {
    uint8_t r[4], l[4];                     /* the envelope: four rates, four levels (0..99) */
    uint8_t ol, fc, ff, det, mode, kvs, ams, rs;   /* output level, coarse, fine, detune (0..14, 7 none), fixed, velocity,
                                                    * amplitude modulation, rate scaling */
    uint8_t bp, ld, rd, lc, rc;             /* keyboard level scaling: break point, depths, curves */
} fm6_op;
typedef struct {
    const char *name;
    fm6_op op[6];
    uint8_t alg, fb, oks;                   /* algorithm 1..32, feedback 0..7, oscillator key sync */
    uint8_t pr[4], pl[4];                   /* the pitch envelope */
    uint8_t lfs, lfd, lpmd, lamd, lks, lfw, lpms;   /* the LFO: speed, delay, depths, key sync, wave, pitch sensitivity */
    uint8_t trnsp;                          /* transpose, 24 = none */
} fm6_patch;
#define FM6_NFACTORY 8
#define FM6_NBANK 27                        /* B1..B27: SLOOP's FM6 bank (its object 8), read in place */
extern const fm6_patch fm6_init_voice, fm6_factory[FM6_NFACTORY];
/* a bank voice for track t (the main loop: fm6_user_set; the sound: from its next block). PTCH 8.. plays it once
 * fm6_user_id[t] says it holds that slot, else the init voice */
extern fm6_patch fm6_user[];
extern volatile int8_t fm6_user_id[];
void fm6_user_set(int t, int ptch, const uint8_t *vmem);   /* a packed voice (128 bytes, the classic VMEM) */
