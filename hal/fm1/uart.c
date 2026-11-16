/* SPDX-License-Identifier: GPL-3.0-only
 * MIDI IN on the TRS jack: UART1's receiver on PH8, 31250 baud, its DMA filling a ring. Written for this project
 * from facts (the registers, their bits and the pin's routing, as the FM-1's public firmwares record them; the
 * baud divider: 48 MHz / 31250 / 4 - 1).
 * The ring is read by what it holds, not by the DMA's count: a slot not yet read holds 0xFD (a MIDI byte no one
 * sends), the reader takes bytes until it meets one and puts 0xFD back behind it. The count can miss a byte
 * that is already in the ring (found on an FM-1 by players of the public firmwares: a note-off then waited for
 * the next note); the content cannot. */
#include "uart.h"
#include "fm1.h"

#define UT1_CON0    REG32(0x12100u)          /* bit 0 on; 0x40 RX DMA, 0x80 latch the count, 0x400 / 0x1000 clear the
                                                pendings */
#define UT1_CON1    REG32(0x12104u)
#define UT1_BAUD    REG32(0x12108u)
#define UT1_OTCNT   REG32(0x12110u)          /* the receive timeout */
#define UT1_RXSADR  REG32(0x1211Cu)
#define UT1_RXEADR  REG32(0x12120u)
#define UT1_RXCNT   REG32(0x12124u)
#define UM_CLK_CON1 REG32(0x10010u)          /* bits 10-11: the UARTs' clock, 1 = PLL 48 MHz */
#define UM_IOMAP2   REG32(0x51024u)          /* bits 8-13: the source of input channel 1 (49 = PH8) */
#define UM_IOMAP3   REG32(0x51028u)          /* bit 7: UART1 on its fixed pins; bits 4-6 its RX (5 = input channel 1) */

#define RING 128u
#define EMPTY 0xFDu
static volatile uint8_t ring[RING] __attribute__((aligned(16)));
static uint32_t rd, count, open_;

void trs_midi_open(void)
{
    for (uint32_t i = 0; i < RING; i++) ring[i] = EMPTY;
    rd = 0;
    UT1_CON0 = 0x3400u;                                            /* off, the pendings cleared */
    UT1_CON1 = 0;
    UM_CLK_CON1 = (UM_CLK_CON1 & ~(3u << 10)) | (1u << 10);
    UM_IOMAP3 &= ~0x80u;
    UM_IOMAP3 = (UM_IOMAP3 & ~0x70u) | (5u << 4);
    UM_IOMAP2 = (UM_IOMAP2 & ~0x3F00u) | (49u << 8);
    pin_in_pullup(PH, 8);
    UT1_RXSADR = (uint32_t)(uintptr_t)ring;
    UT1_RXEADR = (uint32_t)(uintptr_t)ring + RING;
    UT1_RXCNT = RING;
    UT1_BAUD = 48000000u / 31250u / 4u - 1u;
    UT1_OTCNT = 60000u;
    CSYNC();
    UT1_CON0 = 0x40u | 0x80u | 0x1000u | 0x400u;                   /* RX by DMA; no interrupt */
    CSYNC();
    UT1_CON0 |= 1u;                                                /* on, last */
    open_ = 1;
}
int trs_midi_byte(void)
{
    if (!open_) return -1;
    for (;;) {
        uint32_t b = ring[rd];
        if (b == EMPTY && ring[(rd + 1u) % RING] == EMPTY) {       /* nothing new: the pendings acknowledged */
            UT1_CON0 |= 0x80u; CSYNC(); UT1_CON0 |= 0x1400u;
            return -1;
        }
        ring[rd] = EMPTY;                                           /* (a 0xFD with a byte after it: noise, passed) */
        rd = (rd + 1u) % RING;
        count++;
        if (b != EMPTY) return (int)b;
    }
}
uint32_t trs_midi_count(void) { return count; }
