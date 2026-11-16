/* SPDX-License-Identifier: GPL-3.0-only
 * The sample bank (tools/mksamples.py): sets of zones of IMA ADPCM, read in place from the flash window (the PC tests:
 * from RAM). A zone: where its data starts, its length, its loop, its rate, the note it was recorded at, the keys it
 * covers. The SAMPLE and GRAIN engines and the sampled drum kit read it; while the flash window is off (a save), they
 * stay silent (gb_flash_ok). Written for this project. */
#pragma once
#include <stdint.h>
typedef struct { uint32_t off, n, ls, le, rate; int16_t root16, pred; uint8_t idx, lo, hi, looped; } smp_zone;   /* 28 bytes */
typedef struct { char name[8]; uint16_t z0, nz; uint8_t kind, rsv[3]; } smp_set;                              /* 16 bytes */
typedef struct { uint32_t magic; uint16_t ver, nsets, nzones, rsv; uint32_t data_off, data_len, crc, nseek, rsv2; } smp_head;   /* 32 */
/* after the sets and the zones: each zone's first seek point (u32), then the seek points: the decoder's state every 512
 * samples of a zone from its start (predictor in bits 0..15, step index above) */
#define SMP_MAGIC 0x42533353u               /* "S3SB" */
enum { SK_PIANO, SK_ONESHOT, SK_SUS, SK_KIT, SK_SLICE };   /* SK_SLICE: SLICE's BREAK, after the others: not a set of SAMPLE / GRAIN */
extern const uint8_t *gb_bank;              /* the bank's image (0: none) */
extern volatile uint8_t gb_flash_ok;        /* 0 while the flash window is off: nothing may read the bank */
int smp_bank_ok(void);                      /* the bank is there and well formed (checked once: smp_bank_set) */
void smp_bank_set(const uint8_t *img);      /* (main loop, the window on) */
int smp_nsets(void);                        /* SAMPLE's and GRAIN's sets (BREAK is not one) */
int smp_break_zone(void);                   /* SLICE's BREAK: its zone, -1 if the bank has none */
const smp_set *smp_set_at(int k);
const smp_zone *smp_zone_at(int k);
const uint8_t *smp_data(void);
#define SMP_SEEK_SHIFT 10                    /* the seek index: every 1024 samples (format 4, 3.1.6; 512 in format 3, 256 in 2) */
uint32_t smp_seek(int zone, uint32_t k);    /* the decoder's state at sample 512 k of a zone */
/* the user slots: SLOOP's USR1..USR4 ("FSMP", in its flash at 0xA0000, 0xB4000, 0xC8000, 0xE7000, 80 KB each: a
 * header of up to 16 zones as ours, the data at +512), read in place through the window (fw/main.c opens it in
 * clear above 0x93000). A zone's id: the bank's index, or SMP_USR | slot << 5 | zone */
#define SMP_USR 0x8000u
#define SMP_NUSR 4
#define SMP_USR_SIZE 0x14000u
void smp_user_scan(int k, const uint8_t *slot);
void smp_user_drop(int k);
int smp_user_head_ok(const uint8_t *head);        /* a slot's header (its first 512 bytes) well formed: its zones, 0 if not */                         /* slot k out (the editor writes it): its zones are empty */    /* (window on) slot k's header there: usable, or not (0) */
int smp_user_nz(int k);
const char *smp_user_name(int k, char *b9);
const smp_zone *smp_zone_of(uint32_t zi);
const uint8_t *smp_zone_data(uint32_t zi);          /* its data (+ z->off) */
int smp_find(int set, int note);                    /* set: the bank's, then USR1..4; note's zone (-1: none) */
extern const int16_t smp_ima_step[89];
extern const int8_t smp_ima_idx[8];
#ifndef __PI32V2__
int smp_bank_load(const char *path);        /* (the PC: the bank from a file, assets/samples.bin) */
#endif
