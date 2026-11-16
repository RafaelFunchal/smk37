/* SPDX-License-Identifier: GPL-3.0-only
 * The FM-1's power block (P33), 3.3.3: its supplies raised as M-VAVE's own firmware raises them, the reason of the
 * last reset, and a watchdog: a FM-1 that stops answering restarts (and says where it was) instead of staying
 * frozen. Only the main loop touches P33 (no interrupt does). */
#pragma once
#include <stdint.h>

void sys_supplies_stock(void);           /* VDC14 1.60 V, then SYSVDD 1.38 V (up only, a level every 100 us) */
uint32_t sys_reset_bits(void);           /* P3_RST_SRC as found: bit 0 power-on, bit 2 the watchdog, bit 6 soft */
void sys_wdt_arm(void);                  /* reset mode, ~4 s */
void sys_wdt_feed(void);
void sys_wdt_stop(void);
uint32_t sys_p33_timeouts(void);
