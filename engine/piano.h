/* SPDX-License-Identifier: MPL-2.0
 * Erik37: Copyright (C) 2026 M. Geisler (genkigenki/makkuro); adapted for SLOOP 3.
 * PIANO: Erik37's sampled grand as a SLOOP engine (engine/PIANO.md). Its bank (tools/mkpiano.py, Erik37's ERK1
 * format cut down: engine/erk_bank.h) is read in place from the flash window, as SAMPLE's (gb_flash_ok). */
#pragma once
#include <stdint.h>
#include "engine.h"

extern const gb_engine gb_eng_piano;
/* the bank's image (main loop, the flash window on): checked (ERK1, its tables, every sample inside its data); NULL
 * or a bad image leaves the engine silent. Voices started on an earlier image end at their next block. */
void piano_bank_set(const uint8_t *img);
int piano_bank_ok(void);
