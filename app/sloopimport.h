/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP 2.8's projects opened by SLOOP 3 (sloopimport.c). SLOOP 2.8 keeps them in flash from 0x97000 (its song
 * sections A..D and the working project), which SLOOP 3 never writes: at the first start (no SLOOP 3 project
 * yet) they become SLOOP 3's project 1, sections A..D as patterns 1..4 (each with its own sounds, its locks), the
 * working project's tempo, swing, mix, effects, TEXTURE, arpeggiators, the drum kit; its song as ours. Written for this project
 * from the facts of SLOOP 2.8.5's formats (its storage objects, its project format 6), none of its code. */
#pragma once
#include <stdint.h>
#include "../seq/seq.h"
#include "../engine/engine.h"
#include "project.h"

typedef int (*sloop_rd)(uint32_t addr, void *dst, uint32_t n);   /* 0 = read */
#define SLOOP_PROJ_LEN 3840u                       /* (the larger: format 5; format 6 is 3816) */
/* the payload of SLOOP's object obj (1..4: sections A..D, 7: the working project) into buf (SLOOP_PROJ_LEN):
 * 1 if it is a whole project of format 6 (SLOOP 2.6 .. 2.8) or 5 (SLOOP 2.4, 2.5) (its CRCs, its hash), else 0 */
int sloop_read(sloop_rd rd, int obj, uint8_t *buf);
/* the payload of any of SLOOP's objects (0 settings, 1..4 sections, 5 6 user presets, 7 working project, 8 FM6 bank)
 * into buf (cap bytes): its length, 0 if none whole (its header's and its payload's CRCs) */
int sloop_obj(sloop_rd rd, int obj, uint8_t *buf, uint32_t cap);
extern uint32_t sloop_obj_at;                       /* (the payload's flash address, after a sloop_obj found it) */
/* SLOOP's settings of the device (its menu, its calibration): 1 if found */
typedef struct { uint8_t lowcut, lights, keys, notes, usb_full, usb_rate, core2, accel, cal, btn[14], enc[7]; int8_t dir[7]; uint8_t palette, theme_on; } sloop_set;
int sloop_settings(sloop_rd rd, uint8_t *buf, sloop_set *o);
/* SYN1..SYN4 in the settings' payload pl (n bytes, sloop_obj 0), in our keys' order: 1 if there */
int sloop_synkits(const uint8_t *pl, int n, gb_dsnd out[4][16], uint8_t crush[4]);
/* its MIDI LEARN map (in ours: cc | track << 7 | code << 10, fw/main.c ML_*) and its MIDI settings: the entries kept */
typedef struct { uint8_t have, dprev, bend, fxch, dmap, clk, reso, rmch; } sloop_mset;
int sloop_midi(const uint8_t *pl, int n, uint16_t ml[16], sloop_mset *o);
/* one of SLOOP's user presets (a record of its banks, objects 5 and 6): 1 if a sound */
int sloop_upreset(const uint8_t *rec, gb_patch *p, char name[13], uint8_t fx[4], uint8_t arp[4], int *trans);
/* sections[0..3] (0: none) and the working project (0: none): one SLOOP 3 project. 0 if there was nothing */
int sloop_import(const uint8_t *const sections[4], const uint8_t *work, seq_t *q, gb_sound *s, project_extra *x, int *preset_of);
/* the sound of track t (0..2 a synth, 3 the drums: its kit, each sound's TUNE and DECAY) of the project src onto
 * s->trk[t]: the factory sound it is (-1: its own) */
int sloop_sound(const uint8_t *src, int t, gb_sound *s);
/* SLOOP's song (in its settings' payload pl, n bytes) as ours over the sections as patterns 1..4: its entries */
int sloop_song(const uint8_t *pl, int n, const uint8_t *const sections[4], seq_t *q);
/* SLOOP 2.7's favourites (its settings' side record) as ours (a factory sound, 1000 + a place of yours): how many */
int sloop_favs(sloop_rd rd, uint8_t *scratch, uint16_t out[16]);
/* SLOOP 2.7's colour theme (the side record's first 40 bytes, "THM2" and its CRC): 1 if there, into out */
int sloop_theme(sloop_rd rd, uint8_t *scratch, uint8_t out[40]);
extern const uint16_t sloop_bank[], sloop_nbank;       /* (sloop_kinds.c) SLOOP's PRESETS list: engine << 8 | place */
extern const uint8_t sloop_amode[15], sloop_arate[6];   /* SLOOP's arpeggiator's MODE and RATE in ours (arp.h) */
extern const uint8_t *const sloop_kind[12];         /* (sloop_kinds.c, from tools/sloopkinds.py) */
extern const uint8_t sloop_nkind[12];
