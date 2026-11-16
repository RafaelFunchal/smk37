/* SPDX-License-Identifier: GPL-3.0-only
 * Written by tools/sloopkinds.py: the kind of each SLOOP 2.8.5 factory sound, by engine (its order in
 * SLOOP) and its place in that engine's list. Facts only (names and kinds), for app/sloopimport.c. */
#include "sloopimport.h"
#include "presets.h"
static const uint8_t K0[] = { K_BASSE, K_BASSE, K_BASSE, K_BASSE, K_BASSE, K_BASSE, K_BASSE, K_BASSE, K_BASSE, K_LEAD, K_PLUCK, K_BRASS, K_LEAD, K_PAD, K_PAD, K_PAD, K_LEAD, K_LEAD, K_BASSE, K_LEAD, K_BASSE, K_PAD };
static const uint8_t K1[] = { K_KEYS, K_KEYS, K_KEYS, K_KEYS, K_KEYS, K_KEYS, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_PAD, K_BASSE, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_BASSE, K_PLUCK, K_PLUCK, K_KEYS };
static const uint8_t K2[] = { K_BASSE, K_KEYS, K_PAD, K_PLUCK, K_BRASS, K_LEAD, K_PAD, K_PLUCK };
static const uint8_t K3[] = { K_LEAD, K_BASSE, K_PLUCK, K_BASSE, K_BRASS, K_PLUCK, K_LEAD, K_FX };
static const uint8_t K4[] = { K_BASSE, K_BASSE, K_KEYS, K_KEYS, K_KEYS, K_LEAD, K_PLUCK, K_BRASS, K_BRASS, K_FX, K_FX };
static const uint8_t K5[] = { K_PAD, K_PAD, K_LEAD, K_BASSE, K_LEAD, K_LEAD, K_PAD };
static const uint8_t K6[] = { K_BASSE, K_BRASS, K_BRASS, K_BRASS, K_BRASS, K_PAD, K_LEAD, K_LEAD, K_ORGUE, K_PAD, K_BRASS, K_LEAD, K_BRASS, K_PAD };
static const uint8_t K7[] = { K_ORGUE, K_ORGUE, K_ORGUE, K_ORGUE, K_ORGUE, K_ORGUE, K_ORGUE, K_ORGUE };
static const uint8_t K8[] = { K_PAD, K_PAD, K_LEAD, K_PAD, K_PAD, K_PAD, K_PAD, K_PAD };
static const uint8_t K9[] = { K_KEYS, K_PLUCK, K_BASSE, K_BRASS, K_PAD, K_PLUCK, K_ORGUE, K_PLUCK };
static const uint8_t K10[] = { K_PLUCK, K_PLUCK, K_PLUCK, K_PAD, K_PLUCK, K_FX, K_FX, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_BASSE, K_PLUCK, K_PLUCK, K_PAD, K_PLUCK, K_PLUCK, K_PLUCK, K_PLUCK, K_PAD, K_PLUCK, K_PLUCK, K_FX, K_FX, K_FX };
static const uint8_t K11[] = { K_PAD, K_FX, K_LEAD, K_FX, K_PAD, K_FX, K_FX, K_FX, K_FX, K_FX, K_FX };
const uint8_t *const sloop_kind[12] = { K0, K1, K2, K3, K4, K5, K6, K7, K8, K9, K10, K11 };
const uint8_t sloop_nkind[12] = { 22, 22, 8, 8, 11, 7, 14, 8, 8, 8, 26, 11 };
const uint16_t sloop_bank[157] = { 0x0000, 0x0001, 0x0003, 0x0002, 0x0005, 0x0006, 0x0007, 0x0004, 0x010C, 0x0200, 0x0600, 0x0008, 0x0503, 0x0301, 0x0400, 0x0401, 0x0902, 0x0112, 0x0012, 0x0A0C, 0x0303, 0x0014, 0x0100, 0x0101, 0x0102, 0x0104, 0x0105, 0x0402, 0x0403, 0x0404, 0x0201, 0x0103, 0x0900, 0x0115, 0x0700, 0x0701, 0x0702, 0x0703, 0x0704, 0x0906, 0x0608, 0x0705, 0x0706, 0x0707, 0x000D, 0x0605, 0x010B, 0x000E, 0x0202, 0x000F, 0x0800, 0x0801, 0x0500, 0x0501, 0x0904, 0x0609, 0x0803, 0x0804, 0x0805, 0x0A03, 0x0B00, 0x0A0F, 0x0A14, 0x0B04, 0x0206, 0x0506, 0x0806, 0x0807, 0x0015, 0x060D, 0x000C, 0x0009, 0x0606, 0x0607, 0x0502, 0x0300, 0x0405, 0x0802, 0x0011, 0x0010, 0x0504, 0x0B02, 0x0306, 0x0205, 0x0505, 0x0013, 0x060B, 0x000A, 0x0203, 0x010A, 0x0106, 0x0107, 0x0109, 0x0108, 0x0406, 0x0302, 0x0901, 0x0905, 0x0907, 0x0111, 0x010D, 0x010E, 0x010F, 0x0110, 0x0A02, 0x0A07, 0x0A08, 0x0A00, 0x0A01, 0x0A04, 0x0A09, 0x0A0A, 0x0A0B, 0x0A0D, 0x0A0E, 0x0A10, 0x0A11, 0x0A12, 0x0A13, 0x0A15, 0x0A16, 0x0305, 0x0207, 0x0113, 0x0114, 0x0601, 0x0602, 0x0603, 0x0604, 0x000B, 0x0204, 0x0407, 0x0408, 0x0903, 0x060A, 0x0304, 0x060C, 0x0409, 0x040A, 0x0A05, 0x0A06, 0x0B01, 0x0B03, 0x0A17, 0x0A18, 0x0A19, 0x0B05, 0x0B06, 0x0B07, 0x0B08, 0x0B09, 0x0B0A, 0x0307, 0x0CFF, 0x0CFF, 0x0CFF, 0x0CFF };
const uint16_t sloop_nbank = 157;
