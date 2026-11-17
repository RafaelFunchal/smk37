/* SPDX-License-Identifier: GPL-3.0-only
 * Your FM6 bank and your SYN kits, SLOOP 3's own copies (xbank.c), written from the web editor. SLOOP 2.8's are never
 * written: until you save these, SLOOP 3 plays SLOOP 2.8's (read in place), else its defaults.
 * In the flash, after the application area (one sector each; an install never reaches them, fw/build.sh checks it):
 *   0x78000  FM6: "S3F6", version 1 (u16), 27 (u16), the slots used (u32, bit k = B(k+1)), CRC-32 of what follows,
 *            then 27 packed voices of 128 bytes (the classic DX7 VMEM layout, as SLOOP's bank)
 *   0x79000  SYN: "S3SY", version 1, 4, 0, CRC-32, then 4 kits: crush, 3 zeros, 16 sounds of 22 bytes (our keys' order)
 * A sound's 22 bytes: wave, noise source, note, note's sixteenths, bend, bend time, hold, decay, tone level, T2, T2 level,
 * click, noise level, noise hold, noise decay, filter, cutoff, filter env (signed), high-pass, chip clock, drive,
 * level (quarter dB + 128). */
#pragma once
#include <stdint.h>
#include "../engine/drum.h"
#if BOARD_SMK37                                  /* (hal/smk37/smk37.h: the SMK-37's data, above the calibration) */
#define XB_FM6_AT 0xD6000u
#define XB_SYN_AT 0xD7000u
#else
#define XB_FM6_AT 0x78000u
#define XB_SYN_AT 0x79000u
#endif
#define XB_NFM6 27u
#define XB_FM6_LEN (16u + XB_NFM6 * 128u)
#define XB_KIT_LEN (4u + 16u * 22u)
#define XB_SYN_LEN (16u + 4u * XB_KIT_LEN)
void xb_dsnd_pack(const gb_dsnd *s, uint8_t o[22]);
void xb_dsnd_unpack(const uint8_t d[22], gb_dsnd *o);
void xb_kit_pack(const gb_dsnd s[16], uint8_t crush, uint8_t *o);       /* XB_KIT_LEN bytes */
void xb_kit_unpack(const uint8_t *d, gb_dsnd s[16], uint8_t *crush);
void xb_head(uint8_t *img, int fm6, uint32_t used, uint32_t len);        /* the header of an image whose body is there */
int xb_check(const uint8_t *img, uint32_t len, int fm6, uint32_t *used); /* 0: the image is whole */
