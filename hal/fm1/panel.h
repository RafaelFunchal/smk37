/* SPDX-License-Identifier: GPL-3.0-only
 * The FM-1's panel (panel.c): the 14 buttons, the 27 note keys, the 7 encoders and the LEDs, polled one column at a
 * time. Call panel_step() often (each call: one column read, its LEDs lit until the next call); panel_dark() before a
 * long job, so no column's LEDs stay lit through it. Debounced: a press after 2 frames closed, a release after 8
 * open (a frame: the 11 columns). */
#pragma once
#include <stdint.h>

void panel_init(void);
int panel_step(void);                    /* 1 when a frame has just completed (the state below is new) */
void panel_dark(void);
/* 3.3: the timer's interrupt scans the panel (the lights steady whatever the main loop does): panel_ticking() once
 * the interrupt runs, panel_tick() from it 4000 times a second; panel_step() then only says a frame is new, and
 * panel_dark / panel_glow_off do nothing. panel_glow_us: how long a faint light is lit of each column's turn */
void panel_ticking(void);
void panel_tick(void);
void panel_glow_us(int us);
uint32_t panel_keys(void);               /* bit n: note key n held (0 = F3 .. 26 = G5) */
uint32_t panel_buttons(void);            /* bit i: button id i held (0..13, fm1.h BTN_*) */
uint32_t panel_key_edges(uint32_t *released);      /* presses since the last call; *released the releases */
uint32_t panel_button_edges(uint32_t *released);
int panel_enc_take(int e);               /* clicks since the last call, + clockwise */
void panel_led(int id, int on);          /* id as in fm1.h: buttons 0..13, note keys 14..40 */
/* the faint lights (3.2): panel_glow_clear, then panel_glow for each; panel_step lights them with the column's
 * lights, panel_glow_off a few microseconds later leaves the full ones: faint in every frame, never a flash */
void panel_glow(int id);
void panel_glow_clear(void);
void panel_level(int id, int lvl);       /* (3.3.6) 0 off, 1 faint, 2 a third, 3 two thirds, 4 full */
void panel_glow_off(void);
/* the calibration: btn[id] the matrix button of button id, enc[e] the matrix encoder of encoder e, dir[e] +1 or -1
 * (clockwise: +); -1 if it is not a map (each once). The calibration itself reads the matrix as it is: */
int panel_map(const uint8_t btn[14], const uint8_t enc[7], const int8_t dir[7]);
uint32_t panel_raw_buttons(void);
int panel_raw_enc_take(int e);
