/* SPDX-License-Identifier: GPL-3.0-only
 * MIDI apart from the wires (app/midi.c): the bytes, USB-MIDI packets, the channels, the drums' notes, the
 * controllers and the clock in (steady, a tempo change, jitter, a late pulse, a counter that wraps). */
#include "../app/midi.h"
#include "../app/edproto.h"
#include "../engine/engine.h"
#include <stdio.h>
#include <string.h>
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* the messages a byte string makes, as text "90 3C 64, F8, ..." */
static void feed(const uint8_t *b, int n, char *out)
{
    midi_parser p; memset(&p, 0, sizeof p); midi_msg m; out[0] = 0;
    for (int i = 0; i < n; i++) {
        int r = midi_byte(&p, b[i], &m);
        char t[32];
        if (r == MIDI_MSG) { int l = midi_msg_len(m.st); if (l == 1) sprintf(t, "%02X,", m.st); else if (l == 2) sprintf(t, "%02X %02X,", m.st, m.d1); else sprintf(t, "%02X %02X %02X,", m.st, m.d1, m.d2); strcat(out, t); }
        if (r == MIDI_SYSEX) { sprintf(t, "SX%u%s,", (unsigned)p.sx_len, midi_is_soft_key(&p) ? "K" : ""); strcat(out, t); }
    }
}
static double bpm_of(double us_beat) { return 60.0e6 / us_beat; }
int main(void)
{
    printf("midi: bytes, channels, drums, controllers, the clock in\n");
    char got[256];
    { const uint8_t b[] = { 0x90, 60, 100, 62, 90, 0x80, 60, 0 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "90 3C 64,90 3E 5A,80 3C 00,"), "running status: %s", got); }
    { const uint8_t b[] = { 0x90, 60, 0xF8, 100, 0xB0, 74, 0xFA, 20 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "F8,90 3C 64,FA,B0 4A 14,"), "realtime inside: %s", got); }
    { const uint8_t b[] = { 0xC3, 5, 7, 0xD0, 40 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "C3 05,C3 07,D0 28,"), "two-byte messages: %s", got); }
    { const uint8_t b[] = { 0x90, 60, 0x80, 61, 0 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "80 3D 00,"), "a status drops the unfinished: %s", got); }
    { const uint8_t b[] = { 0xF0, 0x22, 0x24, 0x35, 0x7D, 0xF7, 0x90, 1, 2 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "SX6K,90 01 02,"), "soft key: %s", got); }
    { const uint8_t b[] = { 0xF0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0xF8, 0xF7, 0x45, 0x90, 9, 9 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "F8,SX12,90 09 09,"), "a long SysEx, realtime in it, data after: %s", got); }
    { const uint8_t b[] = { 0x90, 60, 0xF2, 1, 2, 3, 0x90, 64, 64 }; feed(b, sizeof b, got); CHECK(!strcmp(got, "F2 01 02,90 40 40,"), "system common ends running status: %s", got); }
    { const uint8_t b[] = { 0xF0, 0x22, 0x24, 0x35, 0x7D, 0x80, 0xF7 }; feed(b, sizeof b, got); CHECK(!strcmp(got, ""), "an interrupted SysEx is no key: %s", got); }

    /* USB-MIDI packets */
    { uint8_t pk[4]; const uint8_t m[3] = { 0x93, 64, 100 }; midi_usb_packet(m, 3, pk); CHECK(pk[0] == 9 && pk[1] == 0x93 && pk[2] == 64 && pk[3] == 100, "note on packet"); CHECK(midi_usb_bytes(pk) == 3, "3 bytes"); }
    { uint8_t pk[4]; const uint8_t m[1] = { 0xF8 }; midi_usb_packet(m, 1, pk); CHECK(pk[0] == 0x0F && pk[1] == 0xF8 && !pk[2], "clock packet"); CHECK(midi_usb_bytes(pk) == 1, "1 byte"); }
    { uint8_t pk[4]; const uint8_t m[2] = { 0xC2, 9 }; midi_usb_packet(m, 2, pk); CHECK(pk[0] == 0x0C && midi_usb_bytes(pk) == 2, "program change"); }
    { const uint8_t pk[4] = { 0x06, 0x7D, 0xF7, 0 }; CHECK(midi_usb_bytes(pk) == 2, "SysEx end, 2 bytes"); }

    /* channels */
    midi_cfg c; midi_cfg_default(&c);
    CHECK(midi_track_of(&c, 0, 2, 5) == 0 && midi_track_of(&c, 3, 2, 5) == 4 && midi_track_of(&c, 4, 2, 5) == MIDI_DRUM_TRACK, "channels 1, 4, 5: the synths shown 1 and 4, the drums shown 5th");
    CHECK(midi_track_of(&c, 9, 0, 5) == MIDI_DRUM_TRACK, "channel 10: the drums");
    CHECK(midi_track_of(&c, 12, 2, 5) == 2, "channel 13: the track selected");
    c.in = MIDI_IN_SEL; CHECK(midi_track_of(&c, 0, 4, 5) == 4 && midi_track_of(&c, 9, 4, 5) == 4, "SEL: all to the track selected");
    c.in = MIDI_IN_OFF; CHECK(midi_track_of(&c, 0, 4, 5) == -1, "OFF");
    CHECK(midi_ch_of_track(0) == 0 && midi_ch_of_track(3) == 9 && midi_ch_of_track(4) == 3, "out channels: synth 4 on 4, the drums on 10");
    c.in = 9; c.sync = 7; c.out = 3; c.clk_out = 5; c.dmap = 2; midi_cfg_check(&c);
    CHECK(c.in == MIDI_IN_TRACKS && c.sync == MIDI_SYNC_INT && c.out == MIDI_OUT_KEYS && c.clk_out == 1 && c.dmap == MIDI_DMAP_GM, "a bad file brought in range");

    /* the drums: every sound comes back as itself, both maps; GM's names */
    for (int m = 0; m < MIDI_DMAP_N; m++) for (int p = 0; p < 16; p++) CHECK(midi_drum_in(midi_drum_out(p, m), m) == p, "map %d sound %d round trip", m, p);
    CHECK(midi_drum_in(36, 0) == 0 && midi_drum_in(38, 0) == 2 && midi_drum_in(42, 0) == 4 && midi_drum_in(46, 0) == 5 && midi_drum_in(49, 0) == 11, "GM: kick snare hat open crash");
    CHECK(midi_drum_in(37, 1) == 1 && midi_drum_in(51, 1) == 15 && midi_drum_in(35, 1) == -1 && midi_drum_in(52, 1) == -1, "PADS: 36 to 51");
    CHECK(midi_drum_in(20, 0) == -1 && midi_drum_in(100, 0) == -1, "GM: out of the map");

    /* controllers */
    CHECK(midi_cc(74).what == CC_BRIGHT && midi_cc(7).what == CC_LEVEL && midi_cc(64).what == CC_SUSTAIN, "74 7 64");
    CHECK(midi_cc(73).what == CC_PARAM && midi_cc(73).param == SP_ATK && midi_cc(72).param == SP_REL && midi_cc(71).what == CC_RES, "attack, release, resonance");
    CHECK(midi_cc(91).what == CC_SEND_REV && midi_cc(94).what == CC_SEND_DLY && midi_cc(123).what == CC_ALL_OFF, "sends, all off");
    CHECK(midi_cc(2).what == CC_NONE, "a number with nothing on it");

    /* the clock in: 120 BPM, in microseconds (tpu 1), 6 beats, blocks of 1451 us asking what to play */
    {
        midi_clock k; mclk_init(&k, 1);
        uint32_t t0 = 1000, pulse = 20833;                       /* 500000 / 24 */
        uint64_t got_u = 0; int blocks = 0;
        mclk_start(&k);
        uint32_t t = t0, nextp = t0, cap = (uint32_t)(2.0 * 1451.0 / pulse * 65536.0);
        int pulses = 0;
        while (pulses < 24 * 6) {
            if ((int32_t)(t - nextp) >= 0) { mclk_pulse(&k, nextp, MIDI_FROM_USB); nextp += pulse; pulses++; }
            got_u += mclk_advance(&k, t, cap); blocks++;
            t += 1451;
        }
        double gotp = (double)got_u / 65536.0;
        CHECK(gotp > 24 * 6 - 2 && gotp <= 24 * 6, "followed the pulses: %.2f of %d", gotp, 24 * 6 - 1);
        CHECK(k.bpm > 119.5f && k.bpm < 120.5f, "tempo shown %.1f", k.bpm);
        CHECK(mclk_alive(&k, t) && !mclk_alive(&k, t + 600000u), "alive, then not");
        CHECK(mclk_alive(&k, k.last - 3000u), "a block's time a little before the last pulse: alive");
        { uint64_t d0 = k.done; mclk_advance(&k, k.last - 3000u, 1u << 20); CHECK(k.done - d0 < 65536u, "a time before the last pulse: no jump"); }
        /* never past the next pulse when it is late */
        uint64_t before = k.done; for (int i = 0; i < 50; i++) mclk_advance(&k, t + 30000u * (uint32_t)i, cap);
        CHECK(k.done - before < 65536u, "a late pulse: waited for (%.2f)", (double)(k.done - before) / 65536.0);
    }
    /* jitter of +-2 ms on each pulse: the tempo stays 140 */
    {
        midi_clock k; mclk_init(&k, 1); mclk_start(&k);
        double beat = 60.0e6 / 140.0, pulse = beat / 24.0; uint32_t r = 7;
        float lo = 999, hi = 0;
        for (int i = 0; i < 24 * 40; i++) {
            r = r * 1664525u + 1013904223u; int j = (int)(r >> 20) % 4001 - 2000;
            mclk_pulse(&k, (uint32_t)(5000.0 + pulse * i) + (uint32_t)j, MIDI_FROM_TRS);
            if (i > 24 * 10 && k.bpm) { if (k.bpm < lo) lo = k.bpm; if (k.bpm > hi) hi = k.bpm; }
        }
        CHECK(lo > 139.0f && hi < 141.0f, "140 with jitter: shown %.1f .. %.1f", lo, hi);
        CHECK(k.src == MIDI_FROM_TRS, "its source");
    }
    /* a tempo change 100 -> 130: taken at once (5 BPM away) */
    {
        midi_clock k; mclk_init(&k, 1); mclk_start(&k);
        uint32_t t = 0; double p1 = 60.0e6 / 100.0 / 24.0, p2 = 60.0e6 / 130.0 / 24.0;
        for (int i = 0; i < 24 * 10; i++) { mclk_pulse(&k, t, 1); t += (uint32_t)p1; }
        CHECK(k.bpm > 99.5f && k.bpm < 100.5f, "100: %.1f", k.bpm);
        for (int i = 0; i < 24 * 2 + 1; i++) { mclk_pulse(&k, t, 1); t += (uint32_t)p2; }
        CHECK(k.bpm > 129.0f && k.bpm < 131.0f, "130 within 2 beats: %.1f", k.bpm);
    }
    /* the counter wraps (60 ticks a microsecond, 72 s around): no harm */
    {
        midi_clock k; mclk_init(&k, 60); mclk_start(&k);
        uint32_t t = 0xFFFFFFFFu - 60u * 300000u, pulse = 60u * 20833u; uint64_t sum = 0;
        for (int i = 0; i < 24 * 4; i++) { mclk_pulse(&k, t, 1); sum += mclk_advance(&k, t + pulse / 2u, 1u << 20); t += pulse; }
        CHECK(k.bpm > 119.5f && k.bpm < 120.5f, "wrapped: tempo %.1f", k.bpm);
        CHECK(sum > (uint64_t)(24 * 4 - 3) << 16, "wrapped: followed (%.1f)", (double)sum / 65536.0);
    }
    (void)bpm_of;
    {   /* the editor's frames: packed, sent as USB packets, read back by the parser into its buffer, unpacked */
        uint8_t raw[300], fr[400], back[300], sx[400]; uint32_t n = 0;
        for (int i = 0; i < 300; i++) raw[i] = (uint8_t)(i * 37 + 5);
        uint32_t fl = ed_frame(ED_READ, raw, 300, fr, sizeof fr);
        CHECK(fl == 6 + (300 * 8 + 6) / 7 && fr[0] == 0xF0 && fr[fl - 1] == 0xF7, "a frame (%u)", fl);
        int bad7 = 0; for (uint32_t i = 1; i + 1 < fl; i++) bad7 |= fr[i] & 0x80; CHECK(!bad7, "7-bit bytes");
        midi_parser mp; memset(&mp, 0, sizeof mp); mp.sxe = sx; mp.sxe_cap = sizeof sx;
        uint32_t at = 0; int got = 0; midi_msg m;
        while (at < fl) { uint8_t pk[4]; int k = ed_usb_packet(fr, fl, at, pk); int nb = midi_usb_bytes(pk);
                          CHECK(nb == k, "a packet's bytes"); for (int i = 0; i < nb; i++) if (midi_byte(&mp, pk[1 + i], &m) == MIDI_SYSEX) got = 1; at += (uint32_t)k; }
        CHECK(got && mp.sx_len == fl, "the SysEx whole (%u)", mp.sx_len);
        int cmd = ed_parse(sx, mp.sx_len, back, sizeof back, &n);
        CHECK(cmd == ED_READ && n == 300 && !memcmp(back, raw, 300), "unpacked (%d, %u)", cmd, n);
        uint8_t other[6] = { 0xF0, 0x22, 0x24, 0x35, 0x7D, 0xF7 };
        CHECK(ed_parse(other, 6, back, sizeof back, &n) < 0, "not ours");
    }
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
