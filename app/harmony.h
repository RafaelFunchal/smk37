/* SPDX-License-Identifier: GPL-3.0-only
 * The key, scales and chords (harmony.c), as SLOOP's SEL: what a key of the FM-1 plays on a synth track.
 *   KEYS  OFF: every key chromatic · SNAP: rounded down to the scale · WHITE: the white keys walk the scale from
 *         C4 (C4 = the root), the black keys silent
 *   CHORD OFF, TRIAD (1 3 5), 7TH (1 3 5 7), 9TH (1 3 7 9), SUS4 (1 4 5), POWER (1 5 8): the white keys play the
 *         chords of the scale, C4 the I chord, D4 the II...; with the chromatic scale, the chords come from the
 *         minor one
 *   CHORD+ a black key held changes the chord: F# major <-> minor, G# adds the 7th, A# sus4, C# adds the 9th,
 *         D# an inversion (both octaves of black keys; several at once)
 * Portable, no state: the PC tests it (tests/harmony_test.c). */
#pragma once
#include <stdint.h>

enum { KEYS_OFF, KEYS_SNAP, KEYS_WHITE, KEYS_N };
enum { CH_OFF, CH_TRIAD, CH_7TH, CH_9TH, CH_SUS4, CH_POWER, CH_N };
enum { CP_MINMAJ = 1, CP_7TH = 2, CP_SUS4 = 4, CP_9TH = 8, CP_INV = 16 };   /* CHORD+ */
#define SCALE_N 16

typedef struct { uint8_t root, scale, keys, chord; int8_t transpose; } harm_cfg;   /* root 0..11 (C..B) */

extern const char *const harm_scale_name[SCALE_N], *const harm_keys_name[KEYS_N], *const harm_chord_name[CH_N];
extern const char *const harm_note_name[12];

/* the notes a key plays (out: up to 5, lowest first); 0: none (a black key with KEYS WHITE or a chord mode).
 * key: 0..26 (F3..G5); oct: the octave shift; plus: the CHORD+ black keys held (CP_*) */
int harm_notes(const harm_cfg *c, int key, int oct, int plus, uint8_t *out);
int harm_plus_of_key(int key);                    /* the CHORD+ bit of a black key, 0 for a white one */
/* a white key's chord, named (C, Dm, Bdim, G7...): what the SEL screen shows on its tile */
void harm_chord_label(const harm_cfg *c, int white, char *out);
