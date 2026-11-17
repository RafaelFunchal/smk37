/* SPDX-License-Identifier: GPL-3.0-only
 * The engines, in the order of the ENG_* indices (engine.h). */
#include "engine.h"
extern const gb_engine gb_eng_va, gb_eng_drum, gb_eng_fm, gb_eng_lofi, gb_eng_pd, gb_eng_wheel, gb_eng_noise, gb_eng_voice, gb_eng_trio, gb_eng_phys, gb_eng_fm6, gb_eng_sample, gb_eng_grain, gb_eng_slice;
#if BOARD_SMK37
extern const gb_engine gb_eng_piano;
const gb_engine *const gb_engines[] = { &gb_eng_va, &gb_eng_drum, &gb_eng_fm, &gb_eng_lofi, &gb_eng_pd, &gb_eng_wheel, &gb_eng_noise, &gb_eng_voice, &gb_eng_trio, &gb_eng_phys, &gb_eng_fm6, &gb_eng_sample, &gb_eng_grain, &gb_eng_slice, &gb_eng_piano };
#else
const gb_engine *const gb_engines[] = { &gb_eng_va, &gb_eng_drum, &gb_eng_fm, &gb_eng_lofi, &gb_eng_pd, &gb_eng_wheel, &gb_eng_noise, &gb_eng_voice, &gb_eng_trio, &gb_eng_phys, &gb_eng_fm6, &gb_eng_sample, &gb_eng_grain, &gb_eng_slice };
#endif
_Static_assert(sizeof gb_engines / sizeof gb_engines[0] == ENG_N, "the engines, in ENG_* order");
const int gb_nengines = (int)(sizeof gb_engines / sizeof gb_engines[0]);
