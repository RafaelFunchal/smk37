/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37's panel for SLOOP 3: the 16 x 8 matrix (37 velocity keys, 18 buttons, 8 encoders, 19 button lights) and
 * the three analog muxes (16 pads, 4 faders, 2 wheels, the pedal, the battery), one column a tick of the timer's
 * interrupt (10 kHz: a frame of 16 columns in 1.6 ms), with one conversion a tick. Its face to fw/main.c is the
 * FM-1's (hal/fm1/panel.h: the same ids); the SMK's own extras are in smk_panel.h.
 * Scan order (the stock one, so the matrix codes col << 4 | row of docs/01-hardware.md apply unchanged): each tick
 * reads the rows of the column latched a tick ago, latches the word shifted during the last tick, then starts the
 * DMA of the next column's word. Facts: fm1-to-smk37 docs/01-hardware.md; the methods: SLOOP Pro's smk_input.h. */
#include "smk37.h"
#include "smk_ctl.h"
#include "smk_panel.h"
#include "../fm1/board.h"
#include "../fm1/panel.h"
#include "../fm1/irq.h"
#include "../fm1/flash.h"
#define XT __attribute__((section(".xtext.smkpanel")))   /* (main loop only, never while a save has the window off) */
#define BOOTBSS __attribute__((section(".lowbss.smk")))  /* (not cleared at the start: panel_init and the flush write them) */

/* ---- the tables (docs/01-hardware.md 5.2 .. 5.5, 6.2): matrix codes col << 4 | row */
static const uint8_t KEYC[SMK_NKEY][2] = {
    {0x10, 0x52}, {0x20, 0x72}, {0x30, 0x92}, {0x40, 0xb2}, {0x50, 0xd2}, {0x60, 0xf2}, {0x70, 0xe2},
    {0x80, 0xc2}, {0x90, 0xa2}, {0xa0, 0x82}, {0xc0, 0x62}, {0xd0, 0x42}, {0xe0, 0x22}, {0xf0, 0x12},
    {0xb0, 0x32}, {0x11, 0x33}, {0x21, 0x43}, {0x31, 0x53}, {0x41, 0x63}, {0x51, 0x73}, {0x61, 0x83},
    {0x71, 0x93}, {0x81, 0xa3}, {0x91, 0xb3}, {0xa1, 0xc3}, {0xb1, 0xd3}, {0xc1, 0xe3}, {0xd1, 0x23},
    {0xe1, 0x13}, {0xf1, 0x14}, {0x64, 0x24}, {0x74, 0x34}, {0x84, 0x44}, {0x94, 0x54}, {0xa4, 0xd4},
    {0xb4, 0xf4}, {0xc4, 0xe4},
};
static const uint8_t BTNC[SMK_NBTN] = { 0x26, 0x76, 0x96, 0x56, 0x66, 0x36, 0x75, 0x65, 0x25, 0x45, 0x15, 0x86, 0x95, 0x55, 0x85, 0x35, 0x16, 0x46 };
static const uint8_t ENCC[SMK_NENC][2] = { {0x97, 0xa7}, {0x37, 0x47}, {0x17, 0x27}, {0xb7, 0xc7}, {0x57, 0x67}, {0x77, 0x87}, {0xd7, 0xe7}, {0xf7, 0x07} };
/* LED i = button i: group << 4 | bit of the word's low half (SEQ PLAY green 16; its red 18) */
static const uint8_t LEDC[SMK_NLED] = { 0x0c, 0x12, 0x00, 0x10, 0x0d, 0x1d, 0x02, 0x01, 0x09, 0x08, 0x0b, 0x0a, 0x11, 0x19, 0x18, 0x1a, 0x1c, 0x1b, 0x1e };
static const uint8_t PADMUX[8] = { 1, 2, 3, 0, 4, 7, 5, 6 };      /* mux address -> pad (mux A; mux B: + 8) */
static const uint8_t FADMUX[4] = { 1, 2, 3, 0 };                  /* mux C address 0..3 -> fader */
/* the FM-1's ids on the SMK's printed buttons (fm1.h BTN_*: OCT- OCT+ FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC) */
static const uint8_t FM1_BTN[14] = { SB_OCT_DOWN, SB_OCT_UP, SB_FX, SB_SCALE, SB_CHORD, SB_NOTE_REPEAT, SB_PARA,
                                     SB_GLOBE, SB_PATCH, SB_BT, SB_ARP, SB_SEQ, SB_SEQ_PLAY, SB_SEQ_REC };
/* the FM-1's encoders (fm1.h ENC_*: SELECT ALGORITHM KNOB1..4 PRESETS) on the SMK's K1..K8 (0..7); K1 = MASTER */
static const uint8_t FM1_ENC[7] = { 1, 2, 4, 5, 6, 7, 3 };
#define XBTN_MASK ((1u << SB_KNOB_BANK) | (1u << SB_FADER_BANK) | (1u << SB_LEFT) | (1u << SB_RIGHT))
/* the 16 white keys among the 27 (F3 G3 A3 B3 C4 D4 E4 F4 G4 A4 B4 C5 D5 E5 F5 G5): pad p is white key p */
static const uint8_t WHITE_NOTE[16] = { 0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26 };
static int8_t white_of[27];

/* ---- the state (the interrupt writes; the main loop reads with it held off) */
static smk_trim trim BOOTBSS;
static volatile uint8_t ticking;
static uint8_t col, grp, raw[SMK_NCOL];
static volatile uint32_t frames; static uint32_t frames_seen;
static uint32_t btn_held, btn_pr, btn_rel; static uint8_t btn_cnt[SMK_NBTN];
static uint32_t knotes, pnotes, notes, note_pr, note_rel;
static uint8_t key_vel[27], ks_stab[SMK_NKEY], ks_rel[SMK_NKEY]; static uint32_t ks_t1[SMK_NKEY];
static uint8_t pad_note[16];                                       /* the white key a pad pressed (+1) */
static uint8_t enc_last[SMK_NENC], enc_hist[SMK_NENC]; static int16_t enc_cnt[SMK_NENC], enc_steps[SMK_NENC];
static smk_pad_t pads[16] BOOTBSS; static smk_fad_t fads[4] BOOTBSS;
static volatile uint16_t wheel_raw[2], pedal_raw, batt_raw; static volatile uint32_t pedal_n;
static uint32_t adc_ok, adc_miss;
static volatile uint8_t vel_mode;
static volatile uint8_t pad_sens = 1;               /* PAD SENS: 0 LOW .. 3 MAX (a hit this far above the rest) */
static const uint8_t PAD_ON[4] = { 20, 32, 48, 72 };  /* (SLOOP Pro's PAD THRESHOLD) */
static volatile int8_t enc_dir = 1;                 /* KNOBS: 1 as built, -1 the other way (a unit wired so) */
static volatile uint16_t pad_rawv[16];              /* (PANEL TEST) each pad's last reading */
static volatile uint8_t hits;                      /* (smk_hits) presses counted, keys and pads: read while a save works */
static volatile uint16_t last_hit;                  /* (PANEL TEST) the last key (0..36) or pad (0x40 + p) << 8 | velocity */
static volatile uint16_t extq[32]; static volatile uint8_t extq_w, extq_r;
/* the lights: buttons and white keys, a level each 0..4 (4 full); `full` from panel_led, `soft` from glow / level */
static uint8_t led_full[14 + 27], led_soft[14 + 27], xled[SMK_NLED];
static volatile uint32_t ledw[2] = { 0xFFFFFFFFu, 0xFFFFFFFFu };
static volatile uint32_t muxw = 0xFFFFFFC7u;
static uint32_t shift_word;                                        /* (the DMA reads it after the call: static) */
/* the analog schedule: (channel << 4 | mux address) a tick: each sweep the 16 pads, the pitch wheel, and one of the
 * other seven mux C inputs in turn (18 ticks a sweep: pads every 1.8 ms; faders, mod wheel, pedal, battery 12.6 ms) */
#define SWEEP 18u
#define NSLOT (7u * SWEEP)
static uint8_t sched[NSLOT], slot, adc_slot = 0xFF;

XT static void sched_init(void)
{
    static const uint8_t OTHER[7] = { 0, 1, 2, 3, 5, 6, 7 };
    uint32_t n = 0;
    for (uint32_t sw = 0; sw < 7u; sw++) for (uint32_t a = 0; a < 8u; a++) {
        sched[n++] = (uint8_t)(0u << 4 | a);
        sched[n++] = (uint8_t)(1u << 4 | a);
        if (a == 4u || a == OTHER[sw]) sched[n++] = (uint8_t)(2u << 4 | a);
    }
}

/* ---- pins and buses */
static void pin_in_pu(int p, uint32_t m) { GPIO(p, G_DIE) |= m; GPIO(p, G_DIR) |= m; GPIO(p, G_PD) &= ~m; GPIO(p, G_PU) |= m; }
static void pin_analog(int p, uint32_t m) { GPIO(p, G_DIE) &= ~m; GPIO(p, G_DIR) |= m; GPIO(p, G_PD) &= ~m; GPIO(p, G_PU) &= ~m; }
static void pins_out_low(int p, uint32_t m) { GPIO(p, G_DIE) |= m; GPIO(p, G_PU) &= ~m; GPIO(p, G_PD) &= ~m; GPIO(p, G_OUT) &= ~m; GPIO(p, G_DIR) &= ~m; }
static void common(uint32_t g)                     /* that group's LED common on (low), the other off; 2: both off */
{
    uint32_t v = SMK_USBIO_CON | 0x1000Fu | (1u << 17) | (1u << 18);
    if (g < 2u) v &= ~(1u << (17u + g));
    SMK_USBIO_CON = v;
}
static uint32_t rows(void)                         /* 1 = closed: PA2 PA5 PA6 PA7 PA9 PH6 PH8 PH9 */
{
    uint32_t a = GPIO(PA, G_IN), h = GPIO(PH, G_IN);
    uint32_t r = ((a >> 2) & 1u) | ((a >> 4) & 0x0Eu) | ((a >> 5) & 0x10u) | ((h >> 1) & 0x20u) | ((h >> 2) & 0xC0u);
    return ~r & 0xFFu;
}
static void shift(uint32_t c)
{
    shift_word = ~(c < 8u ? 0x01000000u << c : 0x00010000u << (c - 8u)) & ledw[grp] & muxw;
    SPI2_CON |= 0x4000u;
    SPI2_ADR = (uint32_t)(uintptr_t)&shift_word;
    SPI2_CNT = 4u;
}
static void latch(void) { GPIO(PA, G_OUT) |= 1u << 1; GPIO(PA, G_OUT) &= ~(1u << 1); }
static void adc_start(uint32_t ch)
{
    ADC_CON = 0;
    if (WLA_CON0 & (1u << 14)) WLA_CON0 &= ~(1u << 14);
    ADC_CON = 0x400Du | ((ch & 0xFu) << 8);        /* the SMK's timing: done in ~40 us (the FM-1's takes ~200) */
    ADC_CON |= 0x10u;
    ADC_CON |= 0x40u;
}

/* ---- events */
static void extq_put(uint32_t k, uint32_t vel)
{
    uint8_t w = extq_w;
    if ((uint8_t)((w + 1u) & 31u) == extq_r) return;
    extq[w] = (uint16_t)(k << 8 | vel); extq_w = (uint8_t)((w + 1u) & 31u);
}
static void notes_now(void)
{
    uint32_t n = knotes | pnotes, ch = n ^ notes;
    note_pr |= ch & n; note_rel |= ch & ~n; notes = n;
}
static void key_event(uint32_t k, uint32_t vel)    /* keybed key 0..36, vel 0 = let go */
{
    if (vel) { last_hit = (uint16_t)(k << 8 | vel); hits++; }
    if (k >= SMK_KEY_F3 && k < SMK_KEY_F3 + 27u) {
        uint32_t b = 1u << (k - SMK_KEY_F3);
        if (vel) { key_vel[k - SMK_KEY_F3] = (uint8_t)vel; knotes |= b; } else knotes &= ~b;
        notes_now();
    } else extq_put(k, vel);
}
static volatile uint8_t pad_drums;                 /* (smk_pad_mode) the pads to the queue as 0x40 + pad */
static uint16_t pad_down;                           /* pads held (any mode) */
static void pad_event(uint32_t p, int vel)         /* hardware pad p: a hit (1..127) or let go (-1) */
{
    if (vel > 0) { pad_down |= (uint16_t)(1u << p); last_hit = (uint16_t)((0x40u + p) << 8 | (uint32_t)vel); hits++; }
    else pad_down &= (uint16_t)~(1u << p);
    if (vel > 0 && pad_drums) { pad_note[p] = 0x80u; extq_put(0x40u + p, (uint32_t)smk_vel_apply(vel_mode, vel)); return; }
    if (vel < 0 && pad_note[p] == 0x80u) { pad_note[p] = 0; extq_put(0x40u + p, 0); return; }   /* (its release goes where its hit went) */
    if (vel > 0) {
        uint32_t n = WHITE_NOTE[p];
        key_vel[n] = (uint8_t)smk_vel_apply(vel_mode, vel);
        pad_note[p] = (uint8_t)(n + 1u); pnotes |= 1u << n;
    } else {
        if (!pad_note[p]) return;
        pnotes &= ~(1u << (pad_note[p] - 1u)); pad_note[p] = 0;
    }
    notes_now();
}

/* ---- a frame: keys, buttons, encoders, the next frame's lights */
static void keys_frame(uint32_t now)
{
    for (uint32_t k = 0; k < SMK_NKEY; k++) {
        uint32_t c1 = (raw[KEYC[k][0] >> 4] >> (KEYC[k][0] & 15u)) & 1u, c2 = (raw[KEYC[k][1] >> 4] >> (KEYC[k][1] & 15u)) & 1u;
        uint32_t s = c1 | c2 << 1, st = ks_stab[k];
        if (s == 2u) s = 3u;                                   /* contact 2 alone: as both */
        if (s == st) { ks_rel[k] = 0; continue; }
        if (s == 0u) {                                         /* contact 1 open: let go after a few frames */
            if (++ks_rel[k] < 3u) continue;
            ks_rel[k] = 0; ks_stab[k] = 0;
            if (st == 3u) key_event(k, 0);
            continue;
        }
        ks_rel[k] = 0;
        if (st == 0u) ks_t1[k] = now;
        if (s == 3u && st != 3u) {
            uint32_t us = (now - ks_t1[k]) / time_mhz();
            key_event(k, (uint32_t)smk_vel_apply(vel_mode, smk_key_velocity(trim.key[k], us)));
        }
        if (s == 1u && st == 3u) continue;                     /* 3 -> 1: still down */
        ks_stab[k] = (uint8_t)s;
    }
}
static void buttons_frame(void)
{
    for (uint32_t i = 0; i < SMK_NBTN; i++) {
        uint32_t c = (raw[BTNC[i] >> 4] >> (BTNC[i] & 15u)) & 1u, b = 1u << i, on = (btn_held & b) != 0u;
        if (c == on) { btn_cnt[i] = 0; continue; }
        if (++btn_cnt[i] < (on ? 4u : 2u)) continue;
        btn_cnt[i] = 0; btn_held ^= b;
        if (!on) btn_pr |= b; else btn_rel |= b;
    }
}
static void enc_frame(void)                         /* the stock decoder: 4 counts a quadrature cycle, a step a detent */
{
    for (uint32_t e = 0; e < SMK_NENC; e++) {
        uint32_t cur = ((raw[ENCC[e][0] >> 4] >> (ENCC[e][0] & 15u)) & 1u) | ((raw[ENCC[e][1] >> 4] >> (ENCC[e][1] & 15u)) & 1u) << 1, h;
        if (enc_last[e] == 0xFF) { enc_last[e] = (uint8_t)cur; enc_hist[e] = (uint8_t)(cur * 0x15u); continue; }
        if (cur != enc_last[e]) { enc_last[e] = (uint8_t)cur; continue; }
        if (cur == (enc_hist[e] & 3u)) continue;
        h = ((uint32_t)enc_hist[e] << 2 | cur) & 0x3Fu; enc_hist[e] = (uint8_t)h;
        if (h == 0x07u || h == 0x1Eu || h == 0x21u || h == 0x38u) enc_cnt[e]--;
        else if (h == 0x0Bu || h == 0x12u || h == 0x2Du || h == 0x34u) enc_cnt[e]++;
        if (enc_cnt[e] >= 4 || enc_cnt[e] <= -4) { int s = enc_cnt[e] / 4; enc_steps[e] = (int16_t)(enc_steps[e] + s); enc_cnt[e] = (int16_t)(enc_cnt[e] - s * 4); }
    }
}
static int lit(uint32_t lvl, uint32_t f)            /* a level 0..4 on frame f: 4 always, 3 three in 4, 2 one in 2, 1 one in 4 */
{
    return lvl >= 4u || (lvl == 3u && (f & 3u)) || (lvl == 2u && !(f & 1u)) || (lvl == 1u && !(f & 3u));
}
static void leds_frame(void)
{
    uint32_t w[2] = { 0xFFFFu, 0xFFFFu }, f = frames >> 1;
    for (uint32_t i = 0; i < 14u; i++) {
        uint32_t l = led_full[i] ? 4u : led_soft[i], L = FM1_BTN[i];
        if (lit(l, f)) w[LEDC[L] >> 4] &= ~(1u << (LEDC[L] & 15u));
    }
    for (uint32_t L = 0; L < SMK_NLED; L++) if (xled[L]) w[LEDC[L] >> 4] &= ~(1u << (LEDC[L] & 15u));
    ledw[0] = 0xFFFF0000u | w[0] | 0x38u;
    ledw[1] = 0xFFFF0000u | w[1] | 0x38u;
    grp ^= 1u;
}

/* ---- the analog side */
static void analog(uint32_t s, uint32_t v)
{
    uint32_t a = sched[s] & 7u, ch = sched[s] >> 4;
    if (ch < 2u) {
        uint32_t p = PADMUX[a] + 8u * ch;
        pad_rawv[p] = (uint16_t)v;
        int r = smk_pad_run(&pads[p], trim.pad[p], v, PAD_ON[pad_sens & 3u]);
        if (r) pad_event(p, r);
    } else if (a < 4u) smk_fad_run(&fads[FADMUX[a]], v);
    else if (a == 4u) wheel_raw[0] = (uint16_t)(wheel_raw[0] ? wheel_raw[0] + ((int32_t)v - wheel_raw[0]) / 4 : v);
    else if (a == 5u) wheel_raw[1] = (uint16_t)(wheel_raw[1] ? wheel_raw[1] + ((int32_t)v - wheel_raw[1]) / 2 : v);
    else if (a == 6u) { pedal_raw = (uint16_t)v; pedal_n++; }
    else batt_raw = (uint16_t)v;
}

/* ---- a tick: one column, one conversion */
static int step(void)
{
    static const uint8_t CH[3] = { SMK_ADC_A, SMK_ADC_B, SMK_ADC_C };
    int done = 0;
    if (adc_slot != 0xFF) {
        if (ADC_CON & 0x80u) { adc_ok++; analog(adc_slot, ADC_RES & 0x3FFu); } else adc_miss++;
        ADC_CON = 0x40u; ADC_CON = 0; adc_slot = 0xFF;
    }
    raw[col] = (uint8_t)rows();
    if (col == 0u) common(grp);
    latch();
    uint32_t next = (col + 1u) & 15u; col = (uint8_t)next;
    {   /* this slot's address went out with the word just latched; the next slot's goes with the next word */
        uint32_t s = slot, ns = s + 1u == NSLOT ? 0u : s + 1u;
        adc_start(CH[(sched[s] >> 4) % 3u]); adc_slot = (uint8_t)s; slot = (uint8_t)ns;
        muxw = 0xFFFFFFC7u | (uint32_t)(sched[ns] & 7u) << 3;
    }
    if (next == 0u) { keys_frame(time_ticks()); buttons_frame(); enc_frame(); leds_frame(); frames++; done = 1; }
    shift(next);
    return done;
}

XT void panel_init(void)
{
    smk_trim_defaults(&trim);
    sched_init();
    for (int i = 0; i < 27; i++) white_of[i] = -1;
    for (int w = 0; w < 16; w++) white_of[WHITE_NOTE[w]] = (int8_t)w;
    pins_out_low(PA, (1u << 0) | (1u << 1) | (1u << 3) | (1u << 4));   /* /OE, latch, SPI2 clock and data */
    pin_in_pu(PA, (1u << 2) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 9));
    pin_in_pu(PH, (1u << 6) | (1u << 8) | (1u << 9));
    pin_analog(PA, (1u << 8) | (1u << 10));
    pin_analog(PC, 1u << 9);
    common(2);
    IOMAP_CON1 = (IOMAP_CON1 & ~0x30000u) | 0x20000u;                  /* SPI2 group C: PA3 / PA4 */
    SPI2_CON = 0x4020u; SPI2_BAUD = 5u; SPI2_CON |= 1u;
    for (int e = 0; e < SMK_NENC; e++) enc_last[e] = 0xFF;
    for (int p = 0; p < 16; p++) smk_pad_init(&pads[p]);
    for (int f = 0; f < 4; f++) smk_fad_init(&fads[f]);
    col = 0; shift(0);
}
XT int smk_trim_read(void)
{
    static uint8_t rec[SMK_TRIM_LEN + 4] __attribute__((aligned(4))) BOOTBSS;   /* (read into before it is looked at) */
    if (flash_read(SMK_TRIM_AT, rec, (SMK_TRIM_LEN + 3u) & ~3u)) return 0;
    smk_trim t; smk_trim_defaults(&t);
    if (!smk_trim_load(&t, rec)) return 0;
    if (ticking) irq_off();
    trim = t;
    if (ticking) irq_on();
    return 1;
}
static void hold_off(void) { if (ticking) irq_off(); }
static void hold_on(void) { if (ticking) irq_on(); }

int panel_step(void)
{
    if (ticking) { uint32_t f = frames; if (f == frames_seen) return 0; frames_seen = f; return 1; }
    uint32_t t0 = time_ticks();
    int d = step();
    while ((uint32_t)(time_ticks() - t0) < 100u * time_mhz()) { }
    return d;
}
void panel_tick(void) { if (ticking) step(); }
void panel_ticking(void) { frames_seen = frames; ticking = 1; }
void panel_dark(void) { }
void panel_glow_us(int us) { (void)us; }
void panel_glow_off(void) { }

uint32_t panel_keys(void) { hold_off(); uint32_t n = notes; hold_on(); return n; }
static uint32_t fm1_ids(uint32_t smk)
{
    uint32_t o = 0;
    for (int i = 0; i < 14; i++) if ((smk >> FM1_BTN[i]) & 1u) o |= 1u << i;
    return o;
}
uint32_t panel_buttons(void) { return fm1_ids(btn_held); }
uint32_t panel_raw_buttons(void) { return fm1_ids(btn_held); }
uint32_t panel_key_edges(uint32_t *rel)
{
    hold_off(); uint32_t p = note_pr, r = note_rel; note_pr = note_rel = 0; hold_on();
    if (rel) *rel = r;
    return p;
}
uint32_t panel_button_edges(uint32_t *rel)
{
    hold_off();
    uint32_t p = btn_pr & ~XBTN_MASK, r = btn_rel & ~XBTN_MASK;
    btn_pr &= XBTN_MASK; btn_rel &= XBTN_MASK;
    hold_on();
    if (rel) *rel = fm1_ids(r);
    return fm1_ids(p);
}
uint32_t smk_xbuttons(void) { return btn_held & XBTN_MASK; }
uint32_t smk_xbutton_edges(uint32_t *rel)
{
    hold_off();
    uint32_t p = btn_pr & XBTN_MASK, r = btn_rel & XBTN_MASK;
    btn_pr &= ~XBTN_MASK; btn_rel &= ~XBTN_MASK;
    hold_on();
    if (rel) *rel = r;
    return p;
}
static int enc_take(int e) { hold_off(); int s = enc_steps[e]; enc_steps[e] = 0; hold_on(); return s * enc_dir; }
int panel_enc_take(int e) { return e >= 0 && e < 7 ? enc_take(FM1_ENC[e]) : 0; }
int panel_raw_enc_take(int e) { return panel_enc_take(e); }
int smk_enc_take(int k) { return k >= 0 && k < SMK_NENC ? enc_take(k) : 0; }
/* the FM-1's calibration (the matrix wired otherwise on some FM-1s) has no use here: only "as built" is taken */
XT int panel_map(const uint8_t btn[14], const uint8_t enc[7], const int8_t dir[7])
{
    for (int i = 0; i < 14; i++) if (btn[i] != i) return -1;
    for (int i = 0; i < 7; i++) if (enc[i] != i || dir[i] != 1) return -1;
    return 0;
}

/* ---- the lights: ids 0..13 the buttons, 14 + k the note keys (the white ones are the pads) */
static volatile uint8_t pads_dirty = 1;
XT static void set_level(int id, uint8_t full, uint8_t soft)
{
    if (id < 0 || id >= 14 + 27) return;
    if (led_full[id] != full || led_soft[id] != soft) { led_full[id] = full; led_soft[id] = soft; if (id >= 14) pads_dirty = 1; }
}
XT void panel_led(int id, int on) { if (id >= 0 && id < 41) set_level(id, (uint8_t)(on != 0), led_soft[id]); }
XT void panel_glow(int id) { if (id >= 0 && id < 41 && !led_soft[id]) set_level(id, led_full[id], 1); }
XT void panel_glow_clear(void) { for (int i = 0; i < 41; i++) set_level(i, led_full[i], 0); }
XT void panel_level(int id, int lvl)
{
    if (id < 0 || id >= 41) return;
    set_level(id, (uint8_t)(lvl >= 4), (uint8_t)(lvl > 0 && lvl < 4 ? lvl : 0));
}
XT void smk_xled(int sb, int on) { if (sb >= 0 && sb < SMK_NLED) xled[sb] = (uint8_t)(on != 0); }

/* ---- the pads' colours: a frame of 36 latch bytes and 16 x 24 bit bytes, sent by board.c */
static uint32_t tint = 0xFFA040u, own[16], dimlvl;
static uint32_t sent_grb[16], sent_ok;                 /* the frame last sent: sent again only when it differs */
static uint8_t frame_buf[SMK_PADLED_LATCH + 16 * 24] __attribute__((aligned(4))) BOOTBSS;
int lcd_pads_send(const uint8_t *frame, uint32_t n);                 /* board.c: 0 sent, -1 the bus busy */
XT void smk_pad_tint(uint32_t rgb) { if (rgb != tint) { tint = rgb; pads_dirty = 1; } }
XT void smk_pad_color(int p, uint32_t rgb) { if (p >= 0 && p < 16 && own[p] != rgb) { own[p] = rgb; pads_dirty = 1; } }
XT void smk_pad_dim(int l) { uint32_t d = l < 0 ? 0u : l > 2 ? 2u : (uint32_t)l; if (d != dimlvl) { dimlvl = d; pads_dirty = 1; } }
XT void smk_pads_flush(void)
{
    static const uint8_t SCALE[5] = { 0, 14, 48, 128, 255 };        /* a level's share of the full colour (/255) */
    if (!pads_dirty) return;
    uint32_t want[16];
    for (int p = 0; p < 16; p++) {
        int id = 14 + WHITE_NOTE[p];
        uint32_t lvl = led_full[id] ? 4u : led_soft[id], c = own[p] ? own[p] : tint, g = 0;
        uint32_t sc = (uint32_t)SCALE[lvl] >> dimlvl;
        for (int k = 0; k < 24; k += 8) {
            uint32_t v = ((c >> k) & 255u) * sc / 255u;
            if (lvl && ((c >> k) & 255u) > 48u && !v) v = 1;                /* a lit pad stays visible */
            g |= v << k;
        }
        want[p] = ((g >> 8) & 255u) << 16 | ((g >> 16) & 255u) << 8 | (g & 255u);
    }
    pads_dirty = 0;
    int same = sent_ok;
    for (int p = 0; same && p < 16; p++) same = want[p] == sent_grb[p];
    if (same) return;                                       /* (leds() lights the same pads again every round: no resend) */
    for (uint32_t i = 0; i < SMK_PADLED_LATCH; i++) frame_buf[i] = 0;   /* (.lowbss: the latch's zeros, every frame) */
    for (int p = 0; p < 16; p++) {
        uint8_t *o = frame_buf + SMK_PADLED_LATCH + SMK_PAD_SLOT[p] * 24;
        for (int i = 0; i < 24; i++) o[i] = (want[p] >> (23 - i)) & 1u ? 0xF0u : 0xC0u;
    }
    if (lcd_pads_send(frame_buf, sizeof frame_buf) == 0) { for (int p = 0; p < 16; p++) sent_grb[p] = want[p]; sent_ok = 1; }
    else pads_dirty = 1;                                    /* (the bus busy: the next round) */
}

/* ---- the extras */
XT XT void smk_pad_sens(int l) { pad_sens = (uint8_t)(l < 0 ? 0 : l > 3 ? 3 : l); }
XT void smk_knob_dir(int reversed) { enc_dir = (int8_t)(reversed ? -1 : 1); }
int smk_pad_raw(int p) { return p >= 0 && p < 16 ? pad_rawv[p] : 0; }
int smk_wheel_raw(int w) { return w >= 0 && w < 2 ? wheel_raw[w] : 0; }
int smk_pedal_raw(void) { return pedal_raw; }
int smk_last_hit(void) { return last_hit; }
uint32_t smk_hits(void) { return hits; }
void smk_pad_mode(int drums) { pad_drums = (uint8_t)(drums != 0); }
int smk_pad_held(int p) { return p >= 0 && p < 16 && ((pad_down >> p) & 1u); }
int smk_key_vel(int k) { return k >= 0 && k < 27 && key_vel[k] ? key_vel[k] : 100; }
XT void smk_vel_mode(int m) { vel_mode = (uint8_t)(m >= 0 && m < KV_N ? m : 0); }
int smk_ext_take(void)
{
    if (extq_r == extq_w) return -1;
    int v = extq[extq_r]; extq_r = (uint8_t)((extq_r + 1u) & 31u);
    return v;
}
XT int smk_fader(int i) { return i >= 0 && i < 4 ? fads[i].v : -1; }
static smk_pw_t pw; static smk_mw_t mw;
static uint32_t ms_now(void) { return irq_ticks() / (SMK_TICK_HZ / 1000u); }   /* (the 10 kHz tick: wraps after ~5 days; time_ticks
                                                                                    * / LSB wrapped every 71.6 s) */
float smk_bend(void) { uint32_t r = wheel_raw[0]; return r ? smk_pw_bend(&pw, (int32_t)r, ms_now()) : 0.0f; }
float smk_mod(void) { uint32_t r = wheel_raw[1]; return r ? smk_mw_val(&mw, (int32_t)r) : 0.0f; }
XT int smk_pedal_take(void)
{
    static smk_ped_t ped; static uint32_t seen;
    uint32_t n = pedal_n;
    if (n == seen) return -1;
    seen = n;
    return smk_ped_sample(&ped, pedal_raw, ms_now());
}
int smk_batt_raw(void) { return batt_raw; }
XT void smk_stats(uint32_t *f, uint32_t *ok, uint32_t *miss) { *f = frames; *ok = adc_ok; *miss = adc_miss; }
