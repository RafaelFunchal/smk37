/* SPDX-License-Identifier: GPL-3.0-only
 * The key, scales and chords (app/harmony.c): what the keys play, as SLOOP's guide describes it. */
#include "../app/harmony.h"
#include <stdio.h>
#include <string.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL: " __VA_ARGS__); printf("\n"); } } while (0)
/* the keys: F3 = 0 ... the white ones */
enum { F3 = 0, Fs3 = 1, G3 = 2, Gs3 = 3, A3 = 4, As3 = 5, B3 = 6, C4 = 7, Cs4 = 8, D4 = 9, Ds4 = 10, E4 = 11, G4 = 14, A4 = 16 };
static int notes(const harm_cfg *c, int key, int plus, uint8_t *o) { return harm_notes(c, key, 0, plus, o); }

int main(void)
{
    uint8_t o[5]; int n; char lab[12];
    printf("harmony: keys, scales, chords, CHORD+\n");
    harm_cfg c = { 0, 0, KEYS_OFF, CH_OFF, 0 };
    n = notes(&c, C4, 0, o); CHECK(n == 1 && o[0] == 60, "C4 chromatic: %d", o[0]);
    n = notes(&c, Fs3, 0, o); CHECK(n == 1 && o[0] == 54, "F#3: %d", o[0]);
    /* SNAP in D minor: C#4 rounds down to C4 */
    c = (harm_cfg){ 2, 2, KEYS_SNAP, CH_OFF, 0 };
    n = notes(&c, Cs4, 0, o); CHECK(n == 1 && o[0] == 60, "snap C#4 in D minor: %d", o[0]);
    n = notes(&c, D4, 0, o); CHECK(o[0] == 62, "snap D4: %d", o[0]);
    /* WHITE in D minor: C4 is D4, D4 is E4, E4 is F4; B3 is C4; the black keys silent */
    c.keys = KEYS_WHITE;
    n = notes(&c, C4, 0, o); CHECK(n == 1 && o[0] == 62, "white C4 = root D: %d", o[0]);
    n = notes(&c, E4, 0, o); CHECK(o[0] == 65, "white E4 = F: %d", o[0]);
    n = notes(&c, B3, 0, o); CHECK(o[0] == 60, "white B3 = C below: %d", o[0]);
    CHECK(notes(&c, Cs4, 0, o) == 0, "a black key with WHITE");
    /* chords in C major: C4 = C E G, D4 = D F A, G4 = G B D */
    c = (harm_cfg){ 0, 1, KEYS_OFF, CH_TRIAD, 0 };
    n = notes(&c, C4, 0, o); CHECK(n == 3 && o[0] == 60 && o[1] == 64 && o[2] == 67, "C: %d %d %d", o[0], o[1], o[2]);
    n = notes(&c, D4, 0, o); CHECK(n == 3 && o[0] == 62 && o[1] == 65 && o[2] == 69, "Dm: %d %d %d", o[0], o[1], o[2]);
    n = notes(&c, G4, 0, o); CHECK(n == 3 && o[0] == 67 && o[1] == 71 && o[2] == 74, "G: %d %d %d", o[0], o[1], o[2]);
    /* CHORD+: G4 + G# = G7; F# + G# = Gm7; on C4, F# + G#: C Eb G B (the 7th of the scale) */
    n = notes(&c, G4, harm_plus_of_key(Gs3), o); CHECK(n == 4 && o[3] == 77, "G7: top %d", o[3]);
    n = notes(&c, G4, harm_plus_of_key(Fs3) | harm_plus_of_key(Gs3), o); CHECK(n == 4 && o[1] == 70 && o[3] == 77, "Gm7: %d %d %d %d", o[0], o[1], o[2], o[3]);
    n = notes(&c, C4, CP_MINMAJ | CP_7TH, o); CHECK(n == 4 && o[0] == 60 && o[1] == 63 && o[2] == 67 && o[3] == 71, "C-Eb-G-B: %d %d %d %d", o[0], o[1], o[2], o[3]);
    n = notes(&c, C4, CP_SUS4, o); CHECK(o[1] == 65, "Csus4: %d", o[1]);
    n = notes(&c, C4, CP_INV, o); CHECK(n == 3 && o[0] == 64 && o[2] == 72, "C first inversion: %d %d %d", o[0], o[1], o[2]);
    CHECK(notes(&c, Cs4, 0, o) == 0, "a black key in chord mode plays nothing (it changes the chord)");
    /* 9TH: 1 3 7 9; POWER: 1 5 8; the chromatic scale gives minor chords */
    c.chord = CH_9TH; n = notes(&c, C4, 0, o); CHECK(n == 4 && o[1] == 64 && o[2] == 71 && o[3] == 74, "C9 (1 3 7 9): %d %d %d %d", o[0], o[1], o[2], o[3]);
    c.chord = CH_POWER; n = notes(&c, C4, 0, o); CHECK(n == 3 && o[1] == 67 && o[2] == 72, "C5: %d %d %d", o[0], o[1], o[2]);
    c = (harm_cfg){ 9, 0, KEYS_OFF, CH_TRIAD, 0 };
    n = notes(&c, C4, 0, o); CHECK(o[0] == 69 && o[1] == 72 && o[2] == 76, "A minor from the chromatic scale: %d %d %d", o[0], o[1], o[2]);
    /* transpose, the octave */
    c = (harm_cfg){ 0, 1, KEYS_OFF, CH_OFF, -12 };
    n = harm_notes(&c, C4, 1, 0, o); CHECK(o[0] == 60, "transpose -12, octave +1: %d", o[0]);
    /* the labels SEL shows */
    c = (harm_cfg){ 0, 1, KEYS_OFF, CH_TRIAD, 0 };
    harm_chord_label(&c, 4, lab); CHECK(!strcmp(lab, "C"), "label I: %s", lab);
    harm_chord_label(&c, 5, lab); CHECK(!strcmp(lab, "Dm"), "label II: %s", lab);
    harm_chord_label(&c, 10, lab); CHECK(!strcmp(lab, "Bdim"), "label VII: %s", lab);
    c.chord = CH_7TH; harm_chord_label(&c, 8, lab); CHECK(!strcmp(lab, "G7"), "label V7: %s", lab);
    harm_chord_label(&c, 5, lab); CHECK(!strcmp(lab, "Dm7"), "label II7: %s", lab);
    /* every key, every setting: notes in range, sorted, no more than 5 */
    int bad = 0;
    for (int r = 0; r < 12; r++) for (int s = 0; s < SCALE_N; s++) for (int k = 0; k < KEYS_N; k++) for (int ch = 0; ch < CH_N; ch++)
        for (int key = 0; key < 27; key++) for (int pl = 0; pl < 32; pl += 3) {
            harm_cfg x = { (uint8_t)r, (uint8_t)s, (uint8_t)k, (uint8_t)ch, 0 };
            n = harm_notes(&x, key, 3, pl, o);
            if (n > 5) bad++;
            for (int i = 1; i < n; i++) if (o[i] < o[i - 1] || o[i] > 127) bad++;
        }
    CHECK(!bad, "%d bad note lists", bad);
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
