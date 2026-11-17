/* SPDX-License-Identifier: GPL-3.0-only
 * The key, scales and chords (harmony.h). Written for this project, after the behaviour SLOOP's guide describes. */
#include "harmony.h"

const char *const harm_scale_name[SCALE_N] = { "CHR", "MAJ", "MIN", "DOR", "MIX", "PEN", "MPEN", "HARM", "PHRY", "LYD",
                                               "LOC", "MEL", "BLUES", "WHOLE", "DIMHW", "DIMWH" };
const char *const harm_keys_name[KEYS_N] = { "OFF", "SNAP", "WHITE" };
const char *const harm_chord_name[CH_N] = { "OFF", "TRIAD", "7TH", "9TH", "SUS4", "POWER" };
const char *const harm_note_name[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

/* each scale's steps from its root, in semitones */
static const uint8_t STEPS[SCALE_N][12] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 },
    { 0, 2, 4, 5, 7, 9, 10 }, { 0, 2, 4, 7, 9 }, { 0, 3, 5, 7, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 1, 3, 5, 7, 8, 10 },
    { 0, 2, 4, 6, 7, 9, 11 }, { 0, 1, 3, 5, 6, 8, 10 }, { 0, 2, 3, 5, 7, 9, 11 }, { 0, 3, 5, 6, 7, 10 }, { 0, 2, 4, 6, 8, 10 },
    { 0, 1, 3, 4, 6, 7, 9, 10 }, { 0, 2, 3, 5, 6, 8, 9, 11 } };
static const uint8_t SIZE[SCALE_N] = { 12, 7, 7, 7, 7, 5, 5, 7, 7, 7, 7, 7, 6, 6, 8, 8 };

static int degree(int scale, int base, int d)            /* the scale's d-th note above base (d < 0: below) */
{
    int n = SIZE[scale], o = d >= 0 ? d / n : -((-d + n - 1) / n), i = d - o * n;
    return base + 12 * o + STEPS[scale][i];
}
static int white_index(int key)                           /* 0..15, or -1 for a black key */
{
#if BOARD_SMK37                                           /* (the SMK-37's keys below F3: -5..-1, white -3..-1) */
    if (key < 0) { int pc = (53 + key) % 12, w = 0; if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10) return -1;
                   for (int k = key; k < 0; k++) { int p = (53 + k + 120) % 12; if (!(p == 1 || p == 3 || p == 6 || p == 8 || p == 10)) w--; } return w; }
#endif
    int pc = (53 + key) % 12, w = 0;
    if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10) return -1;
    for (int k = 0; k < key; k++) { int p = (53 + k) % 12; if (!(p == 1 || p == 3 || p == 6 || p == 8 || p == 10)) w++; }
    return w;
}
int harm_plus_of_key(int key)
{
    switch ((53 + key) % 12) {
    case 6: return CP_MINMAJ;
    case 8: return CP_7TH;
    case 10: return CP_SUS4;
    case 1: return CP_9TH;
    case 3: return CP_INV;
    default: return 0;
    }
}
static void sort(uint8_t *a, int n) { for (int i = 1; i < n; i++) { uint8_t x = a[i]; int j = i; while (j > 0 && a[j - 1] > x) { a[j] = a[j - 1]; j--; } a[j] = x; } }
static int has(const int *a, int n, int v) { for (int i = 0; i < n; i++) if (a[i] == v) return 1; return 0; }

/* the chord of degree d (before the transposition), in v; its count */
static int chord(const harm_cfg *c, int d, int oct, int plus, int *v)
{
    int sc = c->scale == 0 ? 2 : c->scale;                /* chromatic: the chords come from the minor scale */
    int base = 60 + c->root + 12 * oct, n = 0, r = degree(sc, base, d);
    switch (c->chord) {
    case CH_TRIAD: v[n++] = r; v[n++] = degree(sc, base, d + 2); v[n++] = degree(sc, base, d + 4); break;
    case CH_7TH: v[n++] = r; v[n++] = degree(sc, base, d + 2); v[n++] = degree(sc, base, d + 4); v[n++] = degree(sc, base, d + 6); break;
    case CH_9TH: v[n++] = r; v[n++] = degree(sc, base, d + 2); v[n++] = degree(sc, base, d + 6); v[n++] = degree(sc, base, d + 8); break;
    case CH_SUS4: v[n++] = r; v[n++] = degree(sc, base, d + 3); v[n++] = degree(sc, base, d + 4); break;
    default: v[n++] = r; v[n++] = r + 7; v[n++] = r + 12; break;   /* POWER */
    }
    int third = c->chord == CH_POWER || c->chord == CH_SUS4 ? -1 : 1;   /* where the third is */
    if ((plus & CP_MINMAJ) && third > 0) v[third] = v[third] - r == 4 ? r + 3 : v[third] - r == 3 ? r + 4 : v[third];
    if ((plus & CP_SUS4) && third > 0) v[third] = r + 5;
    if ((plus & CP_7TH) && n < 5) { int x = degree(sc, base, d + 6); if (!has(v, n, x)) v[n++] = x; }
    if ((plus & CP_9TH) && n < 5) { int x = degree(sc, base, d + 8); if (!has(v, n, x)) v[n++] = x; }
    if (plus & CP_INV) { int lo = 0; for (int i = 1; i < n; i++) if (v[i] < v[lo]) lo = i; v[lo] += 12; }
    return n;
}
int harm_notes(const harm_cfg *c, int key, int oct, int plus, uint8_t *out)
{
    int w = white_index(key), v[5], n = 0;
    if (c->chord != CH_OFF) {
        if (w < 0) return 0;                              /* (a black key changes the chord: CHORD+) */
        n = chord(c, w - 4, oct, plus, v);
    } else if (c->keys == KEYS_WHITE) {
        if (w < 0) return 0;
        v[n++] = degree(c->scale, 60 + c->root + 12 * oct, w - 4);
    } else {
        int x = 53 + key + 12 * oct;
        if (c->keys == KEYS_SNAP) {                       /* down to the nearest note of the scale */
            int rel = ((x - c->root) % 12 + 12) % 12, best = 0;
            for (int i = 0; i < SIZE[c->scale]; i++) if (STEPS[c->scale][i] <= rel) best = STEPS[c->scale][i];
            x -= rel - best;
        }
        v[n++] = x;
    }
    int m = 0;
    for (int i = 0; i < n; i++) { int x = v[i] + c->transpose; if (x >= 0 && x <= 127) out[m++] = (uint8_t)x; }
    sort(out, m);
    return m;
}
void harm_chord_label(const harm_cfg *c, int white, char *out)
{
    static const int8_t WKEY[16] = { 0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26 };   /* white index -> key */
    uint8_t n[5]; int k = harm_notes(c, WKEY[white & 15], 0, 0, n);
    char *p = out;
    if (!k) { *p = 0; return; }
    if (c->chord == CH_OFF) { const char *s = harm_note_name[n[0] % 12]; while (*s) *p++ = *s++; *p = 0; return; }
    int v[5], m = chord(c, white - 4, 0, 0, v), r = v[0];
    const char *s = harm_note_name[(r + c->transpose + 1200) % 12];
    while (*s) *p++ = *s++;
    if (c->chord == CH_POWER) { *p++ = '5'; *p = 0; return; }
    if (c->chord == CH_SUS4) { *p++ = 's'; *p++ = 'u'; *p++ = 's'; *p = 0; return; }
    int t3 = v[1] - r, t5 = m > 2 && c->chord != CH_9TH ? v[2] - r : 7;
    if (t3 == 3 && t5 == 6) { *p++ = 'd'; *p++ = 'i'; *p++ = 'm'; }
    else if (t3 == 3) *p++ = 'm';
    else if (t5 == 8) *p++ = '+';
    if (c->chord == CH_7TH) *p++ = '7';
    if (c->chord == CH_9TH) *p++ = '9';
    *p = 0;
}
