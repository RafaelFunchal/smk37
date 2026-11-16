/* SPDX-License-Identifier: GPL-3.0-only
 * The arpeggiator's patterns (arp.c): which of the notes held it plays at each step. The firmware keeps the notes
 * and the clock; the PC tests check every mode against SLOOP's examples (C E G held). */
#pragma once
#include <stdint.h>
enum { AM_OFF, AM_UP, AM_DN, AM_UPDN, AM_DNUP, AM_RND, AM_ORD, AM_REPEAT, AM_UP8, AM_CONV, AM_DIVG, AM_PINKY, AM_THUMB, AM_WALK, AM_CHORD, AM_N };
extern const char *const ARP_MODE[AM_N];
extern const uint16_t ARP_TICKS[7];
extern const char *const ARP_RATE[7];
typedef struct { uint8_t mode, rate, oct, gate, swing, prob, hold, ord; } arp_cfg;   /* ord: 0 the notes' order, 1 as played */
/* the notes of step k (a chord: up to 4) of what is held (as pressed, n of them); 0: none. rnd and walk: its state */
int arp_pick(const arp_cfg *c, const uint8_t *held, int n, uint32_t k, uint32_t *rnd, int *walk, uint8_t *out);
