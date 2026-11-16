/* SPDX-License-Identifier: GPL-3.0-only
 * The demo song: four tracks from bar 1 to 8, a fifth (a wide unison pad, the load test) from bar 9 to 16, then
 * again. A step starts in the first block at or after its frame (the real sequencer will place events to the
 * sample). */
#include "demo_song.h"

/* a sound in SLOOP's units: E0..E7, ATK DEC SUS REL, ENV > FILTER, the voice mode */
static void patch(gb_sound *s, int t, int eng, const int8_t *e, const uint8_t *env, int fenv, int voice, int poly, int protect)
{
    gb_patch *q = &s->trk[t].patch;
    gb_patch_default(q, eng); q->poly = (uint8_t)poly; q->protect = (uint8_t)protect;
    for (int i = 0; i < 8; i++) gb_patch_set(q, SP_E0 + i, e[i]);
    if (eng == ENG_DRUM) return;
    for (int i = 0; i < 4; i++) gb_patch_set(q, SP_ATK + i, env[i]);
    gb_patch_set(q, SP_EFLT, fenv); gb_patch_set(q, SP_VOICE, voice);
}

void demo_song_init(demo_song *d, gb_sound *s, float rate, float bpm, uint32_t bars)
{
    static const int8_t DRUMS[8] = { 5, 0, 0, 0 };            /* SLOOP's 808 */
    static const int8_t BASS[8] = { 0, 0, 0, 0, 40, 60, 30, 64 }, PAD[8] = { 0, 18, 64, 4, 62, 10, 0, 32 };
    static const int8_t LEAD[8] = { 1, 8, 64, 0, 80, 20, 10, 64 }, WIDE[8] = { 4, 14, 64, 0, 70, 15, 20, 64 };
    static const uint8_t EB[4] = { 0, 60, 40, 20 }, EP[4] = { 75, 90, 115, 90 }, EL[4] = { 2, 70, 110, 30 }, EW[4] = { 40, 90, 118, 85 };
    patch(s, 0, ENG_DRUM, DRUMS, 0, 0, 0, 8, 1);
    patch(s, 1, ENG_VA, BASS, EB, 40, VM_MONO, 1, 1);
    patch(s, 2, ENG_VA, PAD, EP, 0, VM_POLY, 8, 0);
    patch(s, 3, ENG_VA, LEAD, EL, 20, VM_POLY, 2, 0);
    patch(s, 4, ENG_VA, WIDE, EW, 0, VM_POLY, 8, 0);
    sm_jump(&s->trk[0].level, 0.85f); sm_jump(&s->trk[1].level, 0.7f); sm_jump(&s->trk[2].level, 0.45f);
    sm_jump(&s->trk[3].level, 0.4f); sm_jump(&s->trk[4].level, 0.3f);
    sm_jump(&s->trk[2].send_rev, 0.5f); sm_jump(&s->trk[3].send_dly, 0.35f); sm_jump(&s->trk[3].send_rev, 0.3f);
    sm_jump(&s->trk[0].send_rev, 0.08f); sm_jump(&s->trk[3].pan, 0.62f); sm_jump(&s->trk[4].send_rev, 0.4f);
    sm_jump(&s->dly_time, rate * 60.0f / bpm * 0.75f);       /* a dotted eighth */
    d->frame = 0; d->step_frames = (uint32_t)(rate * 60.0f / bpm / 4.0f + 0.5f); d->bars = bars;
    d->bass_on = d->lead_on = d->chord_on = d->wide_on = -1;
}

void demo_song_block(demo_song *d, gb_sound *s)
{
    static const char *KICK = "x...x...x...x...", *SNR = "....x.......x...", *CH = "..x...x...x...x.", *OH = "......x.......x.";
    static const int BASSN[16] = { 33, -1, 33, 45, -1, 33, -1, 43, 33, -1, 45, -1, 31, -1, 43, -1 };
    static const int CHORDS[4][3] = { { 57, 60, 64 }, { 55, 59, 62 }, { 53, 57, 60 }, { 55, 59, 64 } };
    static const int WIDEC[4][4] = { { 45, 52, 57, 60 }, { 43, 50, 55, 59 }, { 41, 48, 53, 57 }, { 43, 50, 55, 59 } };
    static const int LEADN[32] = { 69, -1, 72, -1, 76, -1, 74, 72, -1, 69, -1, -1, 67, -1, 69, -1,
                                   72, -1, 74, -1, 76, 79, -1, 76, 74, -1, 72, -1, 69, -1, -1, -1 };
    uint32_t fr = d->frame, sf = d->step_frames;
    d->frame += GB_BLOCK;
    if (fr % sf >= GB_BLOCK) return;
    gb_owner seq = GB_OWNER(OWN_SEQ, 0);
    uint32_t st = fr / sf;
    int s16 = (int)(st % 16), bar = (int)(st / 16 % 16);
    if (KICK[s16] == 'x') snd_note_on(s, seq, 0, 0, 1.0f);
    if (SNR[s16] == 'x' && bar >= 1) snd_note_on(s, seq, 0, 1, 0.9f);
    if (SNR[s16] == 'x' && bar >= 4) snd_note_on(s, seq, 0, 2, 0.6f);
    if (CH[s16] == 'x') snd_note_on(s, seq, 0, 3, s16 % 4 == 2 ? 0.55f : 0.35f);
    if (OH[s16] == 'x' && bar >= 2) snd_note_on(s, seq, 0, 4, 0.4f);
    if (bar >= 1 && BASSN[s16] >= 0) {
        if (d->bass_on >= 0) snd_note_off(s, seq, 1, d->bass_on);
        d->bass_on = BASSN[s16]; snd_note_on(s, seq, 1, d->bass_on, s16 % 4 == 0 ? 1.0f : 0.75f);
    }
    if (bar >= 1 && BASSN[s16] < 0 && d->bass_on >= 0) { snd_note_off(s, seq, 1, d->bass_on); d->bass_on = -1; }
    if (s16 == 0 && bar >= 2) {
        if (d->chord_on >= 0) for (int k = 0; k < 3; k++) snd_note_off(s, seq, 2, CHORDS[d->chord_on][k]);
        d->chord_on = bar % 4;
        for (int k = 0; k < 3; k++) snd_note_on(s, seq, 2, CHORDS[d->chord_on][k], 0.8f);
    }
    if (s16 == 0) {                                          /* the wide pad: bars 9..16 */
        if (d->wide_on >= 0) { for (int k = 0; k < 4; k++) snd_note_off(s, seq, 4, WIDEC[d->wide_on][k]); d->wide_on = -1; }
        if (bar >= 8) { d->wide_on = bar % 4; for (int k = 0; k < 4; k++) snd_note_on(s, seq, 4, WIDEC[d->wide_on][k], 0.7f); }
    }
    if (bar >= 4) {
        int ln = LEADN[st % 32];
        if (ln >= 0) { if (d->lead_on >= 0) snd_note_off(s, seq, 3, d->lead_on); d->lead_on = ln; snd_note_on(s, seq, 3, ln, 0.8f); }
        else if (d->lead_on >= 0 && st % 2 == 1) { snd_note_off(s, seq, 3, d->lead_on); d->lead_on = -1; }
    }
}
