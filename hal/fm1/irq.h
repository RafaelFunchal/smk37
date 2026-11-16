/* SPDX-License-Identifier: GPL-3.0-only
 * The firmware's one interrupt (irq.c): fn called 4000 times a second (3.3) (from RAM: it runs during a save too). */
#pragma once
#include <stdint.h>
void irq_start(void (*fn)(void));
uint32_t irq_ticks(void);
void irq_off(void), irq_on(void);
