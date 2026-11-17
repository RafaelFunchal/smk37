/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37's TRS jack is MIDI OUT (no MIDI IN: PH8 is a key-matrix row). hal/fm1/uart.h's receiver answers
 * nothing; trs_midi_send queues bytes that trs_midi_pump sends one at a time, polled: UART1's transmitter on PC7,
 * 31250 baud from the 48 MHz UART clock. Facts: fm1-to-smk37 docs/01-hardware.md 9 (the stock v016 set-up). */
#include "smk37.h"
#include "../fm1/uart.h"

#define UT1_CON0    REG32(0x12100u)          /* bit 0 on; bit 13 clears TPND; bit 15 TPND (the byte has gone) */
#define UT1_CON1    REG32(0x12104u)
#define UT1_BAUD    REG32(0x12108u)
#define UT1_BUF     REG32(0x1210Cu)
#define UM_CLK_CON1 REG32(0x10010u)          /* bits 10-11: the UARTs' clock, 1 = PLL 48 MHz */
#define UM_IOMAP3   REG32(0x51028u)          /* bit 7 clear; bits 23..20 = 1: UART1 TX on an output channel */

#define QN 256u
static uint8_t q[QN]; static uint32_t qr, qw; static uint8_t open_, busy;

void trs_midi_open(void)
{
    UT1_CON0 = 0x3400u;                                            /* off, the pendings cleared */
    UT1_CON1 = 0;
    UM_CLK_CON1 = (UM_CLK_CON1 & ~(3u << 10)) | (1u << 10);
    UM_IOMAP3 &= ~0x80u;
    UM_IOMAP3 = (UM_IOMAP3 & ~(0xFu << 20)) | (1u << 20);
    GPIO(PC, G_PU) |= 1u << 7; GPIO(PC, G_PD) |= 1u << 7;          /* (as the stock set-up) */
    GPIO(PC, G_DIE) |= 1u << 7; GPIO(PC, G_OUT) &= ~(1u << 7); GPIO(PC, G_DIR) &= ~(1u << 7);
    UT1_BAUD = 48000000u / 31250u / 4u - 1u;                       /* 383 */
    CSYNC();
    UT1_CON0 |= 1u;
    open_ = 1; busy = 0; qr = qw = 0;
}
int trs_midi_byte(void) { return -1; }
uint32_t trs_midi_count(void) { return 0; }
void trs_midi_send(const uint8_t *b, int n)                        /* dropped when the queue is full */
{
    if (!open_) return;
    if ((uint32_t)n > QN - 1u - ((qw - qr) & (QN - 1u))) return;   /* (a message whole, or not at all) */
    for (int i = 0; i < n; i++) { q[qw] = b[i]; qw = (qw + 1u) & (QN - 1u); }
}
void trs_midi_pump(void)                                           /* (main loop; a byte takes 320 us) */
{
    if (!open_) return;
    if (busy) { if (!(UT1_CON0 & (1u << 15))) return; busy = 0; }
    if (qr == qw) return;
    UT1_CON0 |= 1u << 13;
    UT1_BUF = q[qr]; qr = (qr + 1u) & (QN - 1u);
    busy = 1;
}
