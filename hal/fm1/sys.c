/* SPDX-License-Identifier: GPL-3.0-only
 * The FM-1's power block (P33): supplies, reset reason, watchdog (sys.h). Written for this project after what the
 * FM-1 firmwares before it found and used on the hardware:
 *   - the P33 serial access and the watchdog (WDT_CON 0x80: timeout code, bit 6 clear, bit 4 enable; VLD_KEEP 0x17
 *     bit 6 off: a plain reset), as in Felucca (Leo Kuroshita, GPL-3.0, fm1_sys.h), SLOOP 2.x and siren, and as
 *     pi32v2-isa's hardware tests used it under m-bootloader (hw/fm1-2026-10-24-app2: code 0xB, reset mode; the
 *     app restarted by it, then started again by m-bootloader);
 *   - the supplies (SLOOP 2.7, from X0X 1.0.9 by Charles Vestal, GPL-3.0): VDC14 (P3_ANA_CON6 [2:0]) 1.25 V +
 *     50 mV a level, SYSVDD (P3_ANA_CON9 [3:0], the cores and the SRAM) 0.93 V + 30 mV a level. The boot loader
 *     leaves 3 (1.40 V) and 11 (1.26 V); M-VAVE's firmware raises them to 7 (1.60 V) and 15 (1.38 V) at the same
 *     360 MHz. At the boot loader's, with both cores busy, some units read the flash and the SRAM wrong. */
#include "fm1.h"
#include "board.h"
#include "sys.h"

#define P33_CON REG32(0x13E08u)
#define P33_DAT REG32(0x13E0Cu)
#define P3_ANA_CON6 0x06u
#define P3_ANA_CON9 0x09u
#define P3_RST_SRC 0x12u
#define P3_VLD_KEEP 0x17u
#define P3_WDT_CON 0x80u
#define WDT_TIME 0x0Cu                          /* 0xA ~1 s, 0xB ~2 s, 0xC ~4 s, 0xD ~8 s (nominal) */

static uint32_t timeouts, rst_bits = 0xFFFFFFFFu;
#define XT __attribute__((section(".xtext.sys")))         /* (at the start only, the window on: from the flash) */

static uint8_t xfer(uint8_t b)
{
    uint32_t n = 100000u;
    P33_DAT = b;
    P33_CON |= 1u << 4;
    (void)P33_CON;
    while (P33_CON & (1u << 1)) if (--n == 0) { timeouts++; break; }
    return (uint8_t)P33_DAT;
}
static void sel(uint32_t a)
{
    if (a & 0x8000u) P33_CON |= (1u << 0) | (1u << 8);
    else { P33_CON &= ~(1u << 8); P33_CON |= 1u << 0; }
}
static void op(uint32_t o, uint32_t a, uint8_t d)          /* 0 write, 1 or, 2 and */
{
    sel(a);
    xfer((uint8_t)((o << 5) | ((a >> 8) & 3u)));
    xfer((uint8_t)a);
    xfer(d);
    P33_CON &= ~((1u << 0) | (1u << 8));
}
static uint8_t rd(uint32_t a)
{
    sel(a);
    xfer((uint8_t)(0x80u | ((a >> 8) & 3u)));
    xfer((uint8_t)a);
    uint8_t v = xfer(0x5E);
    P33_CON &= ~((1u << 0) | (1u << 8));
    return v;
}

static void raise(uint32_t reg, uint32_t mask, uint32_t level)
{
    uint32_t v = rd(reg) & mask;
    while (v < level && v < mask) {
        v++;
        op(0, reg, (uint8_t)((rd(reg) & ~mask) | v));
        time_wait_us(100);                                   /* (the timer started first: time_init) */
    }
}
/* (from RAM, not the flash window: the flash is read while its supply moves) */
void sys_supplies_stock(void) { raise(P3_ANA_CON6, 7u, 7u); raise(P3_ANA_CON9, 15u, 15u); }

XT uint32_t sys_reset_bits(void) { if (rst_bits == 0xFFFFFFFFu) rst_bits = rd(P3_RST_SRC); return rst_bits; }

XT void sys_wdt_arm(void)
{
    (void)sys_reset_bits();                                  /* (read before anything resets it) */
    op(0, P3_WDT_CON, 0);
    op(2, P3_VLD_KEEP, (uint8_t)~0x40u);                     /* a plain reset, no exception first */
    op(0, P3_WDT_CON, (uint8_t)(WDT_TIME | 0x40u));          /* the time, reset mode, cleared */
    op(1, P3_WDT_CON, 0x10u);                                /* on */
}
void sys_wdt_feed(void) { op(1, P3_WDT_CON, 0x40u); }
void sys_wdt_stop(void) { op(0, P3_WDT_CON, 0); }
uint32_t sys_p33_timeouts(void) { return timeouts; }
