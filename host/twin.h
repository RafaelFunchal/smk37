/* SPDX-License-Identifier: GPL-3.0-only
 * The PC twin: SLOOP 3's own firmware (fw/main.c, unchanged) on a simulated FM-1 (host/hal.c), driven by scenarios
 * (host/twin.c) that press its buttons, play its keys, talk to it as the web editor does, cut its power. */
#pragma once
#include <stdint.h>
#define TW_FLASH 0x100000u
extern uint64_t tw_ns;                      /* simulated time */
extern uint8_t tw_flash[TW_FLASH];
extern uint16_t tw_lcd[240 * 240];
extern uint32_t tw_btn, tw_keys; extern int tw_enc[7]; extern uint8_t tw_led[41]; extern int tw_pot;
extern int tw_window_reads_while_off, tw_stale_reads;
extern uint64_t tw_half_cpu_ns;             /* the host's time for the last half: a sudden cost shows (3.1.4) */
void tw_flash_init(void);
void tw_draw_cost(void);                    /* a tile drawn: TWIN_DRAW_US of the CPU time (the FM-1: ~1-2 ms; 0: none) */
void tw_flash_cut(void);
int tw_flash_busy_now(void);
int tw_usb_put_in(const uint8_t pk[4]);
int tw_usb_in_empty(void);
uint32_t tw_usb_take_out(uint8_t (*out)[4], uint32_t cap);
/* the twin's side (twin.c) */
void twin_tick(void);                       /* time moved on */
void twin_audio(const int32_t *lr, uint32_t frames);   /* a half of the audio, done */
void twin_flash_op(int kind, uint32_t a, uint32_t n);  /* 1 erase, 2 write */
void twin_flash_unerased(uint32_t a);       /* a write over bits not erased */
void twin_fail_hook(void);
void twin_end(int code);
void tw_denormals(void);                     /* TWIN_DENORM=1: the places a denormal was met (hal.c) */
