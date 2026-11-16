/* SPDX-License-Identifier: GPL-3.0-only
 * The arpeggiator's patterns (arp.h). Written for this project. */
#include "arp.h"
const char *const ARP_MODE[AM_N] = { "OFF", "UP", "DN", "UPDN", "DNUP", "RND", "ORD", "REPEAT", "UP+8", "CONV", "DIVG", "PINKY", "THUMB", "WALK", "CHORD" };
const uint16_t ARP_TICKS[7] = { 384, 192, 128, 96, 64, 48, 32 };
const char *const ARP_RATE[7] = { "1/4", "1/8", "8T", "1/16", "16T", "1/32", "32T" };
static int arp_list(const arp_cfg *c, const uint8_t *held, int n, uint8_t *L)   /* the notes over the octaves */
{
    uint8_t b[16]; int m = n < 16 ? n : 16;
    for (int i = 0; i < m; i++) b[i] = held[i];
    if (!c->ord && c->mode != AM_ORD) for (int i = 1; i < m; i++) { uint8_t x = b[i]; int j = i; while (j > 0 && b[j - 1] > x) { b[j] = b[j - 1]; j--; } b[j] = x; }
    int N = 0, oc = c->oct < 1 ? 1 : c->oct > 4 ? 4 : c->oct;
    for (int o = 0; o < oc; o++) for (int i = 0; i < m; i++) { int x = b[i] + 12 * o; if (x <= 127) L[N++] = (uint8_t)x; }
    return N;
}
/* the notes of step k (a chord: up to 4); 0: a rest */
int arp_pick(const arp_cfg *c, const uint8_t *held, int n, uint32_t k, uint32_t *rnd, int *walk, uint8_t *out)
{
    uint8_t L[64]; int N = arp_list(c, held, n, L);
    if (!N) return 0;
    uint32_t r = *rnd; r ^= r << 13; r ^= r >> 17; r ^= r << 5; *rnd = r;
    int i = 0;
    switch (c->mode) {
    case AM_UP: case AM_ORD: i = (int)(k % (uint32_t)N); break;
    case AM_DN: i = N - 1 - (int)(k % (uint32_t)N); break;
    case AM_UPDN: case AM_DNUP: { int P = N > 1 ? 2 * N - 2 : 1, j = (int)(k % (uint32_t)P); i = j < N ? j : 2 * N - 2 - j; if (c->mode == AM_DNUP) i = N - 1 - i; break; }
    case AM_RND: i = (int)(r % (uint32_t)N); break;
    case AM_REPEAT: out[0] = held[n - 1]; return 1;
    case AM_UP8: { int j = (int)(k % (uint32_t)(N + 1)); if (j == N) { int x = L[0] + 12 * (c->oct < 1 ? 1 : c->oct); out[0] = (uint8_t)(x > 127 ? 127 : x); return 1; } i = j; break; }
    case AM_CONV: case AM_DIVG: { int j = (int)(k % (uint32_t)N); if (c->mode == AM_DIVG) j = N - 1 - j; i = j & 1 ? N - 1 - j / 2 : j / 2; break; }
    case AM_PINKY: case AM_THUMB: {
        if (N == 1) break;
        int j = (int)(k % (uint32_t)(2 * N - 2));
        if (c->mode == AM_PINKY) i = j & 1 ? N - 1 : j / 2; else i = j & 1 ? 1 + j / 2 : 0;
        break; }
    case AM_WALK: { int w = *walk + ((r >> 8) & 1 ? 1 : -1); if (k == 0) w = 0; w = w < 0 ? 1 : w >= N ? N - 2 : w; *walk = w < 0 ? 0 : w; i = *walk; break; }
    case AM_CHORD: { int m = n < 4 ? n : 4, o = (int)(k % (uint32_t)(c->oct < 1 ? 1 : c->oct)); for (int j = 0; j < m; j++) { int x = held[j] + 12 * o; out[j] = (uint8_t)(x > 127 ? 127 : x); } return m; }
    default: break;
    }
    out[0] = L[i < 0 ? 0 : i >= N ? N - 1 : i];
    return 1;
}
