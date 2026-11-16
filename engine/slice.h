/* SPDX-License-Identifier: GPL-3.0-only
 * SLICE (slice.c, slice_scan.c): a sample cut into slices, played from the keys and the sequencer. Its material:
 * the bank's BREAK (tools/mkbreak.py) or a user slot USR1..USR4 (its first sample). Equal slices (4 8 16 32), or
 * AUTO: one per hit, found by slice_scan. Written for this project from the facts of SLOOP's design (its SLICE,
 * not in SLOOP 2.8.5's release: docs/PARITE-SLOOP3.md).
 * The material is IMA ADPCM, entered only at a known decoder state: the bank's zones have theirs every 256 samples;
 * a user slot's are found by slice_scan (32 points along it) and kept here with its hits.
 * 3.3.6: a user slot of several zones in order (a break cut in the editor, or a kit): its zones are its slices,
 * whatever DIV, each played from its own start (where its decoder starts: nothing to seek, no scan). */
#pragma once
#include <stdint.h>
#define SLC_SRCS 5                    /* BREAK, USR1..USR4 */
#define SLC_GRID 32                   /* a user slot's decoder states: at k * len / 32 */
#define SLC_AUTO 16                   /* AUTO: hits at most */
typedef struct {
    uint32_t len;                     /* samples scanned (0: none, or being rescanned) */
    uint32_t grid[SLC_GRID];          /* (user slots) predictor | step index << 16 */
    uint16_t on[SLC_AUTO];            /* the hits' starts / 4 (on[0] = 0) */
    uint8_t non;
    uint8_t brk;                      /* (3.3.6) its slices are its zones: how many (0: cut as above) */
} slc_tab;
extern slc_tab slc_tabs[SLC_SRCS];
void slice_scan(int src);             /* the material's table to be made (smp_bank_set and smp_user_scan ask for it) */
int slice_scan_poll(void);            /* (main loop, the flash window on) a part of the scanning: 1 while some is left */
void slice_scan_all(void);            /* (the start, the PC) all of it at once */
void slice_drop(int src);             /* the material changes: its voices fall silent */
int slice_zone(int src);              /* its zone (sample.h's id), -1 if none */
