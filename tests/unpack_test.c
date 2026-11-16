/* SPDX-License-Identifier: GPL-3.0-only
 * 3.2: fw/unpack.c itself (the code that unpacks the firmware into RAM at the start) on the PC: the packed image
 * the build made gives back the RAM image, byte for byte; an image damaged anywhere is refused, or unpacks without
 * ever writing outside the RAM it was given (the sanitizers watch).   unpack_test RAM.PK RAM.BIN */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *ram;
#define RAM_AT ((uintptr_t)ram)
#include "../fw/unpack.c"

static uint8_t *load(const char *fn, long *n)
{
    FILE *f = fopen(fn, "rb"); if (!f) { printf("unpack: no %s\n", fn); exit(1); }
    fseek(f, 0, SEEK_END); *n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc((size_t)*n + 4096); memset(b, 0, (size_t)*n + 4096);
    if (fread(b, 1, (size_t)*n, f) != (size_t)*n) exit(1);
    fclose(f); return b;
}
int main(int argc, char **argv)
{
    if (argc < 3) return 1;
    long np, nb; uint8_t *pk = load(argv[1], &np), *bin = load(argv[2], &nb);
    long len = (nb + 3) & ~3L; int fails = 0;
    ram = malloc((size_t)len); memset(ram, 0xA5, (size_t)len);
    int r = xip_unpack(pk, pk + np);
    if (r || memcmp(ram, bin, (size_t)nb)) { printf("unpack: the image does not come back (%d)\n", r); fails++; }
    /* damaged: one bit changed at each of N places (the length and the sums too; one in four a byte's top bit, which
     * plain sums of the words let through when a match copied it); then cut short */
    int N = argc > 3 ? atoi(argv[3]) : 20000;
    uint32_t rs = 12345; int refused = 0, same = 0;
    for (int i = 0; i < N; i++) {
        rs = rs * 1664525u + 1013904223u; long at = i < 12 ? i : (long)((rs >> 3) % (uint32_t)np);
        uint8_t keep = pk[at]; pk[at] ^= (uint8_t)((i & 3) == 3 ? 0x80u : 1u << (rs >> 29));
        memset(ram, 0xA5, (size_t)len);
        r = xip_unpack(pk, pk + np);
        if (r) refused++; else if (!memcmp(ram, bin, (size_t)nb)) same++; else { printf("unpack: a damaged image (byte %ld) was accepted\n", at); fails++; }
        pk[at] = keep;
    }
    for (long cut = 13; cut < np; cut += np / 37) { memset(ram, 0xA5, (size_t)len); if (!xip_unpack(pk, pk + cut)) { printf("unpack: an image cut at %ld was accepted\n", cut); fails++; } }
    printf("unpack: %ld bytes back from %ld; %d damaged images: %d refused, %d harmless; %d failed\n", nb, np, N, refused, same, fails);
    return fails != 0;
}
