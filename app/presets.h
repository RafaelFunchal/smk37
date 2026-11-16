/* SPDX-License-Identifier: GPL-3.0-only
 * The factory sounds (preset_data.c, written by tools/mkpresets.py; presets.c). PRESETS browses them by kind
 * (basses, keys, organs, pads, leads, plucks and bells, brass and stabs, voices, FX; the kits on the drum track),
 * whatever their engine. A preset sets the track's patch, its gain (all as loud) and its two sends; the level and
 * pan stay the player's. */
#pragma once
#include "../engine/engine.h"

enum { K_BASSE, K_KEYS, K_ORGUE, K_PAD, K_LEAD, K_PLUCK, K_BRASS, K_VOIX, K_FX, K_KIT, K_N };
/* a sound in SLOOP's units (docs/sloop/facts.json: its 153 sounds as they are); the drum kits too (E0..E7) */
enum { PX_TRANS = 100 };                                 /* x: a value that is not the sound's (the track's TRANS) */
typedef struct {
    const char *name;
    uint8_t engine, kind;
    int8_t e[8];                                         /* E0..E7 */
    uint8_t env[4];                                      /* ATK DEC SUS REL */
    int8_t fenv;                                         /* ENV > FILTER */
    uint8_t mono;                                        /* 1: MONO */
    uint8_t fx[4];                                       /* DIST, CHORUS, DELAY, REVERB (0..127) */
    uint8_t arp[4];                                      /* MODE RATE OCT GATE + 1 (0: none) */
    int8_t x[8];                                         /* up to 4 (SP_* or PX_*, value) pairs; 0xFF ends */
    uint16_t gain;                                       /* 1/1000: all as loud (tools/levels.txt) */
} gb_preset;
extern const gb_preset gb_presets[];
extern const int gb_npresets;
extern const char *const gb_kind_name[K_N];
extern const uint8_t gb_sloop_map[12][32], gb_sloop_n[12];   /* (preset_data.c: SLOOP's sounds, ours) */
extern const uint8_t gb_sloop_eng[12];                       /* SLOOP's engines (its order), ours (0xFF: none yet) */

int preset_step(int cur, int drum, int d);               /* the next (d > 0) or previous sound, kind by kind */
int preset_rank(int i, int *count);                      /* its place in that order (from 1), and the count */
void preset_apply(gb_sound *s, int trk, int i);
float preset_gain(int i);          /* (a synth track takes any synth sound, any engine) */
int preset_trans(int i);                                 /* its TRANS (semitones), 0 none */
const uint8_t *preset_arp(int i);                        /* its arpeggio (MODE RATE OCT GATE, SLOOP's), 0 none */
/* SLOOP 2.8's sound k of engine e (its order): ours, -1 none (a project of SLOOP's, its sounds) */
int preset_of_sloop(int e, int k);
