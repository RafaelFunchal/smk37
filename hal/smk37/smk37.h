/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP 3 on the M-VAVE SMK-37 (Pro, Elite, MKE-P37; the Donner Starrykey 37 Play): the board's facts. The SoC, the
 * audio, the second core, the flash's SPI0 and the window are the FM-1's (hal/fm1/fm1.h); what differs is the wiring:
 * the screen on SPI1 group A, 16 RGB pads on SPI1 group B, a 16 x 8 key matrix behind four 74HC595 shifted by SPI2,
 * three analog muxes (pads, faders, wheels, pedal, battery) and the TRS jack as MIDI OUT.
 * Facts: github.com/genkigenki/fm1-to-smk37 docs/01-hardware.md (verified on units), m-bootloader docs/SPEC.md 8.
 * SMK-37 port of SLOOP 3: Copyright (C) 2026 the SLOOP 3 SMK-37 port's authors. */
#pragma once
#include <stdint.h>
#include "../fm1/fm1.h"

#ifndef BOARD_SMK37
#error "hal/smk37 is the SMK-37 build: BOARD_SMK37=1"
#endif

/* ---- the screen: ST7789V, 240 x 240, no window offset, no backlight pin (always lit) */
#define SMK_LCD_CS   PC, 8                    /* active low */
#define SMK_LCD_DC   PB, 5                    /* 0 command */
#define SMK_LCD_CLK  PB, 6                    /* SPI1 group A (IOMAP_CON1 bit 4 = 0) */
#define SMK_LCD_MOSI PB, 7
#define SMK_SPI1_GROUP_B (1u << 4)

/* ---- the pads' lights: 16 WS2812-class LEDs on PC10 = SPI1 MOSI in group B (PC9, group B's clock, stays an analog
 * input: ADC channel 8). A bit is one SPI byte (0xF0 one, 0xC0 zero) at BAUD 8; GRB; 36 zero bytes latch first */
#define SMK_PADLED_PIN  PC, 10
#define SMK_PADLED_BAUD 8u
#define SMK_PADLED_LATCH 36u
#define SMK_NPAD 16
static const uint8_t SMK_PAD_SLOT[SMK_NPAD] = { 0, 15, 14, 13, 12, 11, 10, 9, 1, 2, 3, 4, 5, 6, 7, 8 };   /* hardware pad -> chain */

/* ---- the matrix: four 74HC595 (32 bits) by SPI2 group C (PA3 clock, PA4 data), PA1 latch, PA0 /OE low; rows (low =
 * closed, pull-ups) PA2 PA5 PA6 PA7 PA9 PH6 PH8 PH9. NEVER outputs here: PA2, PA9, PH6, PH8, PH9 are rows (on the
 * FM-1 the backlight, LEDs and MIDI IN). Word bits 31..16 the column, active low; 15..0 the button LEDs (active low)
 * except 5..3, the muxes' address */
#define SPI2_CON  REG32(0x11E00u)
#define SPI2_BAUD REG32(0x11E04u)
#define SPI2_ADR  REG32(0x11E0Cu)
#define SPI2_CNT  REG32(0x11E10u)
#define SMK_USBIO_CON REG32(0x16A0Cu)          /* the second USB PHY's pads as GPIO: bits 17 / 18 the LED commons, low on */
#define SMK_NCOL 16
#define SMK_NKEY 37                            /* keybed key 0 = C3 .. 36 = C6 */
#define SMK_KEY_F3 5                           /* keys 5..31: the FM-1's 27 note keys F3..G5 */
#define SMK_NBTN 18
#define SMK_NENC 8
#define SMK_NLED 19

/* the printed buttons (hardware ids, docs/01-hardware.md 5.3) */
enum { SB_KNOB_BANK, SB_FADER_BANK, SB_LEFT, SB_RIGHT, SB_OCT_DOWN, SB_OCT_UP, SB_ARP, SB_NOTE_REPEAT, SB_SCALE,
       SB_CHORD, SB_GLOBE, SB_BT, SB_PATCH, SB_PARA, SB_FX, SB_SEQ, SB_SEQ_PLAY, SB_SEQ_REC };

/* ---- the analog side: ADC ch1 = PA8 (mux A: pads 0..7), ch8 = PC9 (mux B: pads 8..15), ch2 = PA10 (mux C: faders,
 * the wheels, the pedal, the battery); the three share the address (bits 5..3 of the 595 word) */
enum { SMK_ADC_A = 1, SMK_ADC_B = 8, SMK_ADC_C = 2 };

/* ---- MIDI OUT on the TRS jack: UART1 TX on PC7, 31250 baud (no MIDI IN: PH8 is a matrix row) */
#define SMK_MIDI_TX PC, 7

/* ---- the factory calibration (USRTRIM): read, NEVER written. "trimdat\0" at +0x224 */
#define SMK_TRIM_AT 0xC1000u
#define SMK_TRIM_LEN 0x22Cu

/* ---- the flash map (1 MiB). Protected, never erased or programmed by anything here: the boot area [0, 0x4000), the
 * Bluetooth pairing and the calibration [0xC0000, 0xC2000), the stock resources [0xF4000, 0xFE000) and the unit's key
 * area [0xFF000, 0x100000). The installer (m-bootloader's MIDI MODE) writes [0x4000, end of the piano bank): the
 * application area, as on the FM-1, then the piano bank, in clear. SLOOP 3's own data lies above the guarded hole,
 * out of an install's reach: an update keeps the projects, the sounds and the sample slot.
 *   0x04000  the application area (the firmware, the sample bank at 0x48000): enciphered, as on the FM-1
 *   0x78000  the piano bank (engine/piano.c, tools/mkpiano.py), in clear, up to 0xC0000
 *   0xC2000  the user sample slot USR1 (80 KB)
 *   0xD6000  the FM6 bank, 0xD7000 the SYN kits
 *   0xD8000  the project: two banks of 13 sectors
 *   0xF2000  the user sounds, two copies
 */
#define SMK_PIANO_AT     0x78000u
#define SMK_PIANO_END    0xC0000u
#define SMK_SLOT_AT      0xC2000u
#define SMK_DATA_AT      0xD6000u
#define SMK_DATA_END     0xF4000u
#define SMK_WRITE_LO     SMK_SLOT_AT          /* the only range the firmware may erase or program */
#define SMK_WRITE_HI     SMK_DATA_END
#define SMK_PLAIN_FROM   SMK_PIANO_AT         /* the window reads in clear from here: the piano, the slot, the banks */
_Static_assert(SMK_PIANO_END <= 0xC0000u, "the piano bank stays below the Bluetooth pairing and the calibration");
_Static_assert(SMK_WRITE_LO >= 0xC2000u && SMK_WRITE_HI <= 0xF4000u, "writes stay between the calibration and the stock resources");
_Static_assert(SMK_SLOT_AT + 0x14000u == SMK_DATA_AT, "the slot, then the data");

static inline int smk_flash_writable(uint32_t a, uint32_t n)   /* [a, a + n) inside the one range we write */
{
    return a >= SMK_WRITE_LO && a < SMK_WRITE_HI && n <= SMK_WRITE_HI - a;
}
