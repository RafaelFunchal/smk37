/* SPDX-License-Identifier: GPL-3.0-only
 * The demo song (demo_song.c): drums, bass, chords, a lead and, from bar 9, a wide pad, played by a tiny step loop.
 * The PC demo (host/demo.c) and the FM-1's sound bench (bench/son.c) play the same notes. */
#pragma once
#include "../engine/engine.h"

typedef struct { uint32_t frame, step_frames, bars; int bass_on, lead_on, chord_on, wide_on; } demo_song;
void demo_song_init(demo_song *d, gb_sound *s, float rate, float bpm, uint32_t bars);   /* bars: 0 = loop for ever */
void demo_song_block(demo_song *d, gb_sound *s);       /* the notes of the next block (call before rendering it) */
