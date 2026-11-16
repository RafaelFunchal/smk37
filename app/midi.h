/* SPDX-License-Identifier: GPL-3.0-only
 * MIDI (midi.c), apart from the wires (hal/fm1/usb.c: USB, hal/fm1/uart.c: the TRS jack): the bytes made into
 * messages, the clock followed, the channels, the drum notes and the controllers. Nothing here touches the sound
 * or the sequencer: fw/main.c does, with these answers. The PC tests run all of it (tests/midi_test.c).
 *
 * What SLOOP 3 does with MIDI (as SLOOP 2.8's players know it, from its README):
 *   IN   PISTES: channel 1-5 the track of that number, 10 the drums, the others the track selected;
 *        SEL: every channel the track selected; OFF
 *   SYNC INT: our own tempo; EXT: the clock in (24 a beat, USB or TRS), START / CONTINUE / STOP
 *   OUT  OFF; KEYS: what the keys play; SEQ: the sequencer, the arpeggiator and the rolls too. Track 1-5 on
 *        channel 1-5, the drums on 10
 *   CLK  the clock out on USB, with START / STOP (only while SYNC is INT: no loop through a computer)
 *   BATT the drums' notes: GM (36 kick, 38 snare, 42 hat ...) or PADS (36 to 51: the 16 sounds in order) */
#pragma once
#include <stdint.h>

enum { MIDI_IN_OFF, MIDI_IN_TRACKS, MIDI_IN_SEL, MIDI_IN_N };
enum { MIDI_SYNC_INT, MIDI_SYNC_EXT, MIDI_SYNC_N };
enum { MIDI_OUT_OFF, MIDI_OUT_KEYS, MIDI_OUT_SEQ, MIDI_OUT_N };
enum { MIDI_DMAP_GM, MIDI_DMAP_PADS, MIDI_DMAP_N };
typedef struct { uint8_t in, sync, out, clk_out, dmap; } midi_cfg;
extern const char *const midi_in_name[MIDI_IN_N], *const midi_sync_name[MIDI_SYNC_N],
                  *const midi_out_name[MIDI_OUT_N], *const midi_dmap_name[MIDI_DMAP_N];
void midi_cfg_default(midi_cfg *c);
void midi_cfg_check(midi_cfg *c);              /* (from a file: every field brought in range) */

/* ---- messages. st: the status (channel in its low 4 bits); realtime and the system ones carry no data */
typedef struct { uint8_t st, d1, d2, src; } midi_msg;
enum { MIDI_FROM_USB = 1, MIDI_FROM_TRS = 2 };
#define MIDI_SX_MAX 8
typedef struct {
    uint8_t st, need, got, d1;                 /* running status, the data bytes it takes, those come */
    uint8_t sys;                               /* a system common message under way (no running status) */
    uint8_t sx, sxn, sxb[MIDI_SX_MAX];         /* in a SysEx; its first bytes (F0 included) */
    uint32_t sx_len;                           /* its whole length */
    uint8_t *sxe; uint32_t sxe_cap;            /* (optional) the whole SysEx kept here too, up to sxe_cap bytes */
} midi_parser;
enum { MIDI_NONE = 0, MIDI_MSG = 1, MIDI_SYSEX = 2 };
/* a byte: MIDI_MSG when it ends a message (in m), MIDI_SYSEX when it ends a SysEx (its start in p->sxb, its
 * length p->sx_len). Running status, realtime anywhere (even inside another message), a status cancelling what
 * was unfinished */
int midi_byte(midi_parser *p, uint8_t b, midi_msg *m);
/* the bytes a USB-MIDI event packet carries (0..3), from its code index (pk[0] & 15) */
int midi_usb_bytes(const uint8_t pk[4]);
/* a message as a USB-MIDI event packet (cable 0); n its bytes (1..3) */
void midi_usb_packet(const uint8_t *msg, int n, uint8_t pk[4]);
int midi_msg_len(uint8_t st);                  /* the bytes of a message with that status, status included */
/* the soft key (F0 22 24 35 7D F7): the FM-1 to the chip's USB download mode, as its first firmware does */
int midi_is_soft_key(const midi_parser *p);

/* ---- the channels */
int midi_track_of(const midi_cfg *c, int ch, int sel, int ntrk);   /* ch 0..15: the track, -1 none */
int midi_ch_of_track(int t);                    /* 0..15: track 1-5 on 1-5, the drums (track 4) on 10 */
#define MIDI_DRUM_TRACK 3
int midi_drum_in(int note, int map);            /* a note: the drum sound 0..15, -1 none */
int midi_drum_out(int pad, int map);            /* a drum sound: its note */

/* ---- the controllers: what a CC number turns */
enum { CC_NONE, CC_PARAM, CC_LEVEL, CC_PAN, CC_SEND_DLY, CC_SEND_REV, CC_SEND_CHO, CC_BRIGHT, CC_RES, CC_SUSTAIN,
       CC_ALL_OFF, CC_RESET };
typedef struct { uint8_t what, param; } midi_cc_dest;
midi_cc_dest midi_cc(int cc);                   /* (CC_BRIGHT, CC_RES: the engine's cutoff, resonance) */

/* ---- the clock in: 24 pulses a beat. The sequencer goes by the pulses, smoothly between two of them (from the
 * interval before) but never past the next one before it comes, so it cannot drift; behind, it catches up at
 * twice the tempo at most. The tempo shown is the mean of the last 8 beats, changed only when 0.8 BPM away (a
 * steady 140 stays 140); a beat 5 BPM away is a new tempo, taken at once. Times are in ticks of a free-running
 * counter (tpu ticks a microsecond), compared by differences: a counter that wraps does no harm */
#define MIDI_CLK_NB 8
typedef struct {
    uint32_t tpu;                              /* the counter's ticks a microsecond */
    uint32_t last, iv;                         /* the last pulse, the interval between pulses (smoothed) */
    uint32_t beat_t, bdt[MIDI_CLK_NB];         /* pulse 0 of the beat under way; the last beats' lengths */
    uint8_t n24, nb, bi, alive, have, src;     /* pulses into the beat; beats kept; next; pulses come; a pulse
                                                  since START (the downbeat); from where (MIDI_FROM_*) */
    uint32_t pulses;                           /* pulses since the downbeat */
    uint64_t done;                             /* what the sequencer was given, in pulses << 16 */
    float bpm;                                 /* the tempo shown (0: none yet) */
} midi_clock;
void mclk_init(midi_clock *c, uint32_t tpu);
void mclk_start(midi_clock *c);                /* START or CONTINUE: the next pulse is the downbeat */
void mclk_pulse(midi_clock *c, uint32_t now, int src);
int mclk_alive(const midi_clock *c, uint32_t now);   /* a pulse in the last half second */
/* how far the sequencer goes now, in pulses << 16 (at most cap): 0 until the downbeat */
uint64_t mclk_advance(midi_clock *c, uint32_t now, uint64_t cap);
