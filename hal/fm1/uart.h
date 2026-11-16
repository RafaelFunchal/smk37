/* SPDX-License-Identifier: GPL-3.0-only
 * MIDI IN on the FM-1's TRS jack (uart.c): PH8 to UART1's receiver, 31250 baud, received by DMA into a small
 * ring, polled. A TRS-to-DIN adapter of the right type (A or B) is needed: with the other, nothing comes. */
#pragma once
#include <stdint.h>
void trs_midi_open(void);
int trs_midi_byte(void);                   /* the next byte received, or -1 */
uint32_t trs_midi_count(void);             /* bytes received since the start */
