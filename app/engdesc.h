/* SPDX-License-Identifier: GPL-3.0-only
 * Each engine as the player sees it (engdesc.c, written by tools/mkengdesc.py from SLOOP's facts): its name, its two
 * pages' titles, its eight values (label, how a value reads, its choices), which of them CC 74 and CC 71 move (the
 * cutoff or brightness, the resonance or Q: -1 none). The track's other values (envelope, LFO, voice) are the
 * same for every engine (sp_ui in fw/main.c). */
#pragma once
#include <stdint.h>
#include "../engine/engine.h"
enum { PF_NUM, PF_PCT, PF_BI, PF_TIME, PF_HZ, PF_CUT, PF_SEMI, PF_CENT, PF_LIST, PF_DB,
       PF_LVL,      /* a level: (v - 112) / 2 dB, 0 off */
       PF_LIST8 };  /* a choice of 16 over 0..127 (v / 8) */
typedef struct { const char *name; uint8_t fmt, n; const char *const *ch; } pdesc;   /* n: choices (PF_LIST) */
typedef struct { const char *name; const char *page[2]; pdesc p[8]; int8_t cut, res; } edesc;
extern const edesc gb_edesc[ENG_N];
