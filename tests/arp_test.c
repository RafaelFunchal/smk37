/* SPDX-License-Identifier: GPL-3.0-only
 * The arpeggiator's modes, against SLOOP's own examples: C, E and G held. */
#include "../app/arp.h"
#include <stdio.h>
#include <string.h>
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void run(int mode, int oct, int ord, const uint8_t *held, int n, int steps, char *got)
{
    arp_cfg c = { (uint8_t)mode, 3, (uint8_t)oct, 50, 0, 100, 0, (uint8_t)ord };
    uint32_t r = 1; int w = 0; got[0] = 0;
    static const char *const NM[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int k = 0; k < steps; k++) {
        uint8_t o[4]; int m = arp_pick(&c, held, n, (uint32_t)k, &r, &w, o);
        for (int i = 0; i < m; i++) { strcat(got, NM[o[i] % 12]); if (o[i] >= 72) strcat(got, "'"); if (i + 1 < m) strcat(got, "+"); }
        strcat(got, " ");
    }
}
int main(void)
{
    printf("arp: the modes, C E G held\n");
    const uint8_t ceg[3] = { 60, 64, 67 }, gce[3] = { 67, 60, 64 };
    static const struct { int mode; const char *want; } T[] = {
        { AM_UP, "C E G C " }, { AM_DN, "G E C G " }, { AM_UPDN, "C E G E C " }, { AM_DNUP, "G E C E G " },
        { AM_UP8, "C E G C' C " }, { AM_CONV, "C G E C " }, { AM_DIVG, "E G C E " }, { AM_PINKY, "C G E G C " },
        { AM_THUMB, "C E C G C " }, { AM_REPEAT, "G G G " } };
    char got[256];
    for (unsigned i = 0; i < sizeof T / sizeof T[0]; i++) {
        int steps = 0; for (const char *p = T[i].want; *p; p++) steps += *p == ' ';
        run(T[i].mode, 1, 0, ceg, 3, steps, got);
        CHECK(!strcmp(got, T[i].want), "%s: '%s' (want '%s')", ARP_MODE[T[i].mode], got, T[i].want);
    }
    run(AM_ORD, 1, 0, gce, 3, 4, got);  CHECK(!strcmp(got, "G C E G "), "ORD: '%s'", got);
    run(AM_UP, 1, 1, gce, 3, 3, got);   CHECK(!strcmp(got, "G C E "), "UP, ORD PLAY: '%s'", got);
    run(AM_UP, 2, 0, ceg, 3, 7, got);   CHECK(!strcmp(got, "C E G C' E' G' C "), "UP, 2 octaves: '%s'", got);
    run(AM_CHORD, 2, 0, ceg, 3, 3, got); CHECK(!strcmp(got, "C+E+G C'+E'+G' C+E+G "), "CHORD: '%s'", got);
    /* WALK: one step at a time, never out of the notes held */
    arp_cfg c = { AM_WALK, 3, 2, 50, 0, 100, 0, 0 }; uint32_t r = 7; int w = 0, prev = -1, ok = 1;
    for (int k = 0; k < 200; k++) { uint8_t o[4]; arp_pick(&c, ceg, 3, (uint32_t)k, &r, &w, o); if (w < 0 || w > 5 || (prev >= 0 && (w - prev > 1 || prev - w > 1))) ok = 0; prev = w; }
    CHECK(ok, "WALK left its path");
    uint8_t one = 60, o[4]; uint32_t rr = 1; int ww = 0;
    for (int m = 1; m < AM_N; m++) { if (m == AM_UP8) continue;   /* (its C' is meant) */
        arp_cfg cc = { (uint8_t)m, 3, 1, 50, 0, 100, 0, 0 }; for (int k = 0; k < 8; k++) if (arp_pick(&cc, &one, 1, (uint32_t)k, &rr, &ww, o) < 1 || o[0] != 60) { CHECK(0, "%s with one note held", ARP_MODE[m]); break; } }
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
