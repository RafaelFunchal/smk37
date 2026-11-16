/* SPDX-License-Identifier: GPL-3.0-only
 * The drum kits (engine/drum.c): SLOOP's General MIDI sampled kit and its four treatments, then its 32 synthesised
 * kits (engine/drumkits.c, tools/mkdrumkits.py). A synthesised sound, in SLOOP's musical units:
 *   tone   SINE TRI SQUARE FM BELL at a pitch, falling BEND semitones over BT; HOLD at full, then DECAY; a second
 *          partial (a sine at T2 / 32 x the tone, its level the tone's squared x T2LEV)
 *   click  a burst of bright noise at the hit
 *   noise  WHITE METAL (six squares) CYM (metal + white) CHIP (an LFSR at its own clock) | CLAP (three bursts, then
 *          the tail); its level, hold and decay; a 12 dB/oct high-pass before it (HPF)
 *   filter a state-variable LP BP HP on the noise or (| 4) on everything, its resonance (bits 3..7: 0..31), its cutoff
 *          FCUT (+ FENV steps at the hit, falling with the pitch; the velocity: softer is darker)
 *   out    a drive, the kit's crush (bits dropped: low nibble; a sample-and-hold of high nibble + 1), a level
 * Times: an index 0..127 on 5 ms .. 4 s (to -60 dB); cutoffs: an index on the cutoff curve; HOLDs in 2 ms. */
#pragma once
#include <stdint.h>
typedef struct {
    uint8_t wave, src;
    uint16_t note16;            /* the tone's pitch (METAL, CYM: their base), MIDI note x 16 */
    uint8_t bend, bt, hold, decay, tlev, t2, t2lev, click, nlev, nhold, ndec, flt, fcut;
    int8_t fenv;
    uint8_t hpf, chip, drive;   /* hpf: 0 off; chip: the CHIP clock's MIDI note */
    int8_t qdb;                 /* its level, quarter dB */
} gb_dsnd;
typedef struct { const char *name, *style; uint8_t crush; uint16_t s[16]; } gb_dkit;   /* s: its sounds in gb_dsnds */
extern const gb_dsnd gb_dsnds[];
enum { DW_NONE, DW_SINE, DW_TRI, DW_SQUARE, DW_FM, DW_BELL };
enum { DN_NONE, DN_WHITE, DN_METAL, DN_CYM, DN_CHIP, DN_CLAP = 0x10 };
#define GB_NDKITS 32            /* synthesised */
#define GB_KIT_SAMPLED 5        /* ACOUSTIC DEEP TIGHT BRIGHT DUST: the GM set, as it is and treated */
#define GB_KIT_USR (GB_KIT_SAMPLED + GB_NDKITS)   /* USR1..USR4: a user sample slot as a kit, USR3+4: one kit in two */
#define GB_KIT_SYN (GB_KIT_USR + 5)               /* SYN1..SYN4: your synthesised kits (SLOOP's, from its settings) */
#define GB_NKITS (GB_KIT_SYN + 4)
extern const gb_dkit gb_dkits[GB_NDKITS];
extern const char *const gb_dkit_names[GB_NKITS];
extern gb_dsnd gb_dsyn[4][16];          /* SYN1..SYN4 (RAM), in our keys' order */
extern uint8_t gb_dsyn_crush[4];
void drum_syn_default(void);
void drum_syn_forget(int k);            /* SYN k changed: played as it is now from the next hit */            /* SYN1..4 as SLOOP starts them: copies of 808, 909, TRAP, TECHNO */
