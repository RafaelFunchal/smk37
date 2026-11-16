/* SPDX-License-Identifier: GPL-3.0-only
 * Each pattern's own sound (psound.c). A track has 12 patterns; each keeps the track's sound as it was when
 * that pattern played last: change a sound while pattern 3 plays and only pattern 3 has it; pattern 1 comes
 * back with its own. The track's live sound (the engine's patch and the firmware's own values, before the
 * steps' locks) is the current pattern's; when the sequencer changes pattern, the live sound is put back in its
 * pattern and the next pattern's is taken. Kept small (the parameters in bytes, as the presets are); the drum
 * track's 16 sounds' tune, decay and mute too. The level, pan, sends, filter stay the track's (the mix). */
#pragma once
#include <stdint.h>
#include "../engine/engine.h"
#include "../seq/seq.h"
#define PS_DRUM 3                                   /* the drum track (its sounds' tune, decay, mute per pattern) */
typedef struct { uint8_t engine, poly, unison, protect; int16_t preset; uint16_t gain; uint8_t p[GB_NPARAM]; } psound;
typedef struct { int8_t dtune[16], ddec[16]; uint16_t dmute; } pdrum;
typedef struct {
    psound s[SEQ_NTRK][SEQ_NPAT];
    pdrum d[SEQ_NPAT];
    uint8_t cur[SEQ_NTRK];                          /* the pattern whose sound each track has now */
} psounds;
/* preset: the firmware's preset_of (-1 none, k, -k - 2: k edited); own: its values before locks (main.c base) */
void ps_take(psounds *ps, int t, int pat, const gb_patch *live, const float *own, int preset);   /* live -> pattern */
void ps_give(const psounds *ps, int t, int pat, gb_patch *live, float *own, int *preset);       /* pattern -> live */
void ps_fill(psounds *ps, int t, int cur, const gb_patch *live, const float *own, int preset);   /* every pattern: live */
int ps_same(const psounds *ps, int t, int a, int b);   /* two patterns of a track: the same sound */
void ps_copy(psounds *ps, int t, int from, int to);
/* bytes: a pattern's sound in a project file (PS_BYTES, then 34 more on the drum track); 0 on a short read */
int ps_write(const psounds *ps, int t, int pat, uint8_t *out);
int ps_read(psounds *ps, int t, int pat, const uint8_t *in, uint32_t len, int old);   /* old: a format-12 file's (an 8-bit preset) */
#define PS_BYTES (8 + GB_NPARAM)                    /* (format 12: 7 + GB_NPARAM, the preset in a byte) */
#define PS_BYTES12 (7 + GB_NPARAM)
#define PS_DRUM_BYTES 34
