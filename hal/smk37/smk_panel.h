/* SPDX-License-Identifier: GPL-3.0-only
 * What the SMK-37's panel has beyond the FM-1's (hal/smk37/panel.c): hal/fm1/panel.h's API keeps the FM-1's ids (14
 * buttons, 27 note keys F3..G5, 7 encoders), so fw/main.c reads the SMK as an FM-1; these give the rest: the keys'
 * velocity, the outer ten keys, the four extra buttons and their lights, the pads' colours, the faders, the wheels,
 * the pedal, the battery. */
#pragma once
#include <stdint.h>

/* the panel's FM-1 face on the SMK:
 *   note keys 0..26     = keybed keys 5..31 (F3..G5), and the 16 pads as the 16 white keys (reading order: the top
 *                         row white keys 1-8, the bottom row 9-16), each with its velocity
 *   buttons (fm1.h ids) = the printed ones: OCT-/+ OCTAVE-/+, FX FX, SEL SCALE, ENV CHORD, LFO NOTE REPEAT,
 *                         EDIT PARA, GLO GLOBE, HOME PATCH, SAVE BT, ARP ARP, SEQ SEQ, PLAY SEQ PLAY, REC SEQ REC
 *   encoders            = SELECT K2, ALGORITHM K3, PRESETS K4, KNOB 1..4 K5..K8; K1 is the MASTER pot (adc.c) */
int smk_key_vel(int k);                    /* note key 0..26: the velocity of its last press, 1..127 */
void smk_vel_mode(int mode);               /* KEY VELOCITY (smk_ctl.h KV_*), for the keys and the pads */
int smk_ext_take(void);                    /* the outer keys (keybed 0..4 = C3..E3, 32..36 = G#5..C6): key << 8 | velocity
                                            * (0: let go), -1 when none is waiting */
uint32_t smk_xbuttons(void);               /* the four extra buttons held, bit SB_* (KNOB BANK, FADER BANK, <, >) */
uint32_t smk_xbutton_edges(uint32_t *rel);
void smk_xled(int sb, int on);             /* their lights */
int smk_fader(int i);                      /* fader 1..4 as 0..3: 0..127, -1 before its first reading */
float smk_bend(void);                      /* the pitch wheel, -1..1 (0 at rest) */
float smk_mod(void);                       /* the mod wheel, 0..1 */
int smk_pedal_take(void);                  /* the sustain pedal: 1 went down, 0 went up, -1 no change */
int smk_batt_raw(void);                    /* mux C 7: as the FM-1's ADC channel 3 (the same divider) */
/* the pads' lights: the FM-1's white-key lights (panel_led / panel_glow / panel_level on ids 14 + white key) light
 * the pads in this colour (0xRRGGBB, the selected track's); smk_pad_color sets one pad's own colour (0: the tint) */
void smk_pad_tint(uint32_t rgb);
void smk_pad_color(int pad, uint32_t rgb);
void smk_pad_dim(int lvl);                 /* PAD LIGHTS: 0 full, 1 half, 2 a quarter */
void smk_pads_flush(void);                 /* (main loop) the pads' frame sent when it changed and the screen's bus is free */
void smk_pad_mode(int drums);              /* 1: DRUM PADS, a hit goes to smk_ext_take as (0x40 + pad) << 8 | velocity */
int smk_pad_held(int pad);                 /* pad 0..15 held now */
/* the factory calibration, read once the flash is open (never written): 1 the unit's curves, 0 the defaults */
int smk_trim_read(void);
/* the panel's diagnostics: frames scanned, conversions read / missed */
void smk_stats(uint32_t *frames, uint32_t *adc_ok, uint32_t *adc_miss);
int smk_enc_take(int k);                   /* knob K1..K8 as 0..7, raw (K1 is MASTER: adc.c) */
/* the TRS jack, MIDI OUT (uart.c): bytes queued, sent from the main loop */
void trs_midi_send(const uint8_t *b, int n);
void trs_midi_pump(void);
/* the timer's rate the panel wants (irq.c): 10 kHz, a column a tick, a frame of 16 in 1.6 ms */
#define SMK_TICK_HZ 10000u
