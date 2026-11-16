/* SPDX-License-Identifier: GPL-3.0-only
 * One interrupt: TIMER5 at 4 kHz (3.3; 2 kHz before), for the panel (a column a tick: hal/fm1/panel.c) and the USB
 * port (its audio input wants a packet every millisecond, whatever the main loop is doing: the caller serves it every
 * other tick). Every other source stays off. Written for this project from the facts of the FM-1's
 * interrupt controller (the vectors at 0x01C7FE00, a configuration word per 8 sources from 0x1EEF100: 4 bits each,
 * bit 0 on, bits 1-3 the priority; the global enable: icfg bit 8 and sti) and of TIMER5 (0x10900: CON, CNT, PRD;
 * from OSC / 4 = 6 MHz, source 63; CON bit 14 acknowledges). */
#include "irq.h"
#include "fm1.h"

#define VEC ((volatile uint32_t *)0x01C7FE00u)
#define ICFG(n) REG32(0x1EEF100u + 4u * ((uint32_t)(n) >> 3))
#define ILAT_CLR REG32(0x1EEF1A4u)
#define T5_CON REG32(0x10900u)
#define T5_CNT REG32(0x10904u)
#define T5_PRD REG32(0x10908u)
#define IRQ_TIMER5 63u

extern void isr_tick(void);                    /* irq.S */
static void (*tick_fn)(void);
static volatile uint32_t ticks;
void irq_tick(void)                            /* (from isr_tick) */
{
    T5_CON |= 0x4000u;
    ticks++;
    if (tick_fn) tick_fn();
}
uint32_t irq_ticks(void) { return ticks; }
void irq_start(void (*fn)(void))
{
    __asm__ volatile("cli" ::: "memory");
    for (uint32_t i = 0; i < 32u; i++) REG32(0x1EEF100u + 4u * i) = 0;   /* every source off */
    ILAT_CLR = 0xFFu;
    tick_fn = fn;
    T5_CON = 0x4000u; T5_CNT = 0; T5_PRD = 1500u;                       /* 6 MHz / 1500: 4 kHz */
    VEC[IRQ_TIMER5] = (uint32_t)(uintptr_t)isr_tick;
    uint32_t sh = (IRQ_TIMER5 & 7u) * 4u;
    ICFG(IRQ_TIMER5) = (ICFG(IRQ_TIMER5) & ~(0xFu << sh)) | (((1u << 1) | 1u) << sh);   /* on, priority 1 */
    T5_CON = 0x4019u;
    uint32_t c; __asm__ volatile("%0 = icfg" : "=r"(c)); c |= 0x100u; __asm__ volatile("icfg = %0" ::"r"(c) : "memory");
    __asm__ volatile("csync\n\tsti" ::: "memory");
}
void irq_off(void) { __asm__ volatile("cli" ::: "memory"); }
void irq_on(void) { __asm__ volatile("csync\n\tsti" ::: "memory"); }
