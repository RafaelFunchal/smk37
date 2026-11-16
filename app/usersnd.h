/* SPDX-License-Identifier: GPL-3.0-only
 * Your sounds (usersnd.c): 32 places for a synth track's sound, kept in flash beside the projects (store.h, two
 * copies of a sector, saved as a project is). At the first start they are SLOOP 2.8's user presets (its two banks
 * of 16, read from its flash, each made a SLOOP 3 sound value for value); then yours. PRESETS goes through them
 * after the factory sounds; EDIT 5/5 (SON USER) loads, saves and erases them. Written for this project. */
#pragma once
#include <stdint.h>
#include "../engine/engine.h"
#include "sloopimport.h"

#define US_N 32
#define US_BASE 1000                     /* a track's preset_of: US_BASE + place (edited: -(US_BASE + place) - 2) */
#define US_REC 80
#define US_LEN (8 + US_N * US_REC)
#define US_NFAV 16
#define US_FILE_FV (US_LEN + 2 + 2 * US_NFAV)   /* the bank with your favourites after it ("FV", 16 x u16: a factory sound, US_BASE + a place; 0xFFFF none) */
#define US_FILE (US_FILE_FV + 40)               /* and your colour theme (40 bytes as SLOOP 2.7's: "THM2", a name, 5 + 4 colours, bg, accent, CRC) */
#define US_LEN_OK(n) ((n) == US_LEN || (n) == US_FILE_FV || (n) == US_FILE)
/* a place: used (0xA5), engine, kind, transpose, name (12), DIST CHO DLY REV, the arpeggiator (mode + 1, rate,
 * octaves, gate; 0: none), the gain (x 1000), poly, unison, the 48 values (0..255), 4 spare */
typedef struct { uint8_t used, engine, kind; int8_t trans; char name[12]; uint8_t fx[4], arp[4]; uint16_t gain; uint8_t poly, unison, p[GB_NPARAM]; } us_sound;

void us_from_bytes(const uint8_t *bank, int n);             /* (power-on) the names and which are used, from the bank's bytes */
int us_used(int k);
extern uint16_t us_fav[US_NFAV];                            /* your favourites (SLOOP 2.7's): first in PRESETS */
extern uint8_t us_theme[40];                                /* your theme's bytes (checked where it is used: fw/main.c) */
void us_fav_put(uint8_t *bank);                             /* the favourites and the theme into a bank's bytes of US_FILE, before a save */
const char *us_name(int k);                                 /* (a place's name, kept in RAM) */
int us_get(const uint8_t *bank, int k, us_sound *s);        /* a place of a bank's bytes: 1 if used */
void us_put(uint8_t *bank, int k, const us_sound *s);       /* (s 0: erased); the bank's header too */
void us_apply(gb_sound *snd, int t, const us_sound *s);     /* onto a track (its engine, values, gain, sends) */
void us_take(const gb_sound *snd, int t, const float *own, const char *name, us_sound *s);   /* a track's sound as a place */
/* SLOOP's user presets (its objects 5 and 6: "UPB1", 16 records of 192 bytes) as places, into bank (US_LEN): how many */
int us_from_sloop(sloop_rd rd, uint8_t *scratch, uint8_t *bank);
