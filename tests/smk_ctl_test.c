/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37's controls as numbers (hal/smk37/smk_ctl.h): the calibration record, key velocity, the pads' trigger,
 * the faders' filter, the wheels, the pedal.   cc -std=c11 -o build/smk_ctl_test tests/smk_ctl_test.c */
#include <stdio.h>
#include <string.h>
#include "../hal/smk37/smk_ctl.h"

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    smk_trim t; smk_trim_defaults(&t);
    /* the calibration record: taken with its marker and rising curves, refused otherwise */
    uint8_t rec[0x22C]; memset(rec, 0, sizeof rec);
    for (int i = 0; i < 53; i++) for (int j = 0; j < 5; j++) { uint16_t v = (uint16_t)(100 + 200 * j + i); rec[8 + i * 10 + 2 * j] = (uint8_t)v; rec[9 + i * 10 + 2 * j] = (uint8_t)(v >> 8); }
    CHECK(!smk_trim_load(&t, rec) && !t.from_unit, "no marker: taken");
    memcpy(rec + 0x224, "trimdat", 8);
    CHECK(smk_trim_load(&t, rec) && t.from_unit && t.key[0][0] == 116 && t.pad[0][0] == 100, "the record not taken as it is");
    rec[8 + 2] = 0xFF; rec[9 + 2] = 0xFF;                   /* pad 0's second point above its third: not rising */
    smk_trim t2; smk_trim_defaults(&t2);
    CHECK(!smk_trim_load(&t2, rec) && t2.pad[0][0] == 50, "a curve not rising: taken");
    /* key velocity: the stock curve; faster is louder, the ends 1 and 127 */
    smk_trim_defaults(&t);
    int last = 128, mono = 1;
    for (uint32_t us = 0; us < 120000u; us += 500u) { int v = smk_key_velocity(t.key[0], us); if (v > last) mono = 0; last = v; }
    CHECK(mono, "velocity rises as the stroke slows");
    CHECK(smk_key_velocity(t.key[0], 0) == 127 && smk_key_velocity(t.key[0], 200000u) == 1, "the ends: %d, %d", smk_key_velocity(t.key[0], 0), smk_key_velocity(t.key[0], 200000u));
    CHECK(smk_vel_apply(KV_SOFT, 64) < 64 && smk_vel_apply(KV_HARD, 64) > 64 && smk_vel_apply(KV_FIX100, 3) == 100, "KEY VELOCITY's curves");
    /* a pad: a hit (its velocity after the peak), held, then let go, once */
    smk_pad_t p; smk_pad_init(&p);
    for (int i = 0; i < 500; i++) smk_pad_run(&p, t.pad[0], 60, 32);   /* (the baseline settles on the rest: a count each 8 readings, ~0.9 s) */
    CHECK(p.base >= 58 && p.base <= 62, "the baseline follows the rest: %d", p.base);
    int hits = 0, rels = 0, vel = 0;
    static const uint16_t HIT[] = { 60, 200, 600, 800, 700, 500, 400, 300, 300, 300, 200, 100, 70, 62, 61, 60, 60, 60, 60, 60 };
    for (unsigned i = 0; i < sizeof HIT / sizeof HIT[0]; i++) { int r = smk_pad_run(&p, t.pad[0], HIT[i], 32); if (r > 0) { hits++; vel = r; } if (r < 0) rels++; }
    CHECK(hits == 1 && rels == 1, "one hit, one release: %d, %d", hits, rels);
    CHECK(vel > 64, "a firm hit: velocity %d", vel);
    for (int i = 0; i < 60; i++) smk_pad_run(&p, t.pad[0], 60 + (i & 7), 32);   /* (noise under the threshold) */
    hits = 0; for (int i = 0; i < 60; i++) if (smk_pad_run(&p, t.pad[0], 60 + (i & 7), 32) > 0) hits++;
    CHECK(hits == 0, "noise at rest: %d hits", hits);
    /* a fader: its first reading at once (it once came half: a median with zeros); a spike dropped; both ends */
    smk_fad_t f; smk_fad_init(&f);
    smk_fad_run(&f, 800);
    CHECK(f.v == 100, "the first reading: %d (want 100)", f.v);
    smk_fad_run(&f, 0); CHECK(f.v == 100, "a lone spike moved it: %d", f.v);
    for (int i = 0; i < 20; i++) smk_fad_run(&f, 1023);
    CHECK(f.v == 127, "the top: %d", f.v);
    for (int i = 0; i < 30; i++) smk_fad_run(&f, 0);
    CHECK(f.v == 0, "the bottom: %d", f.v);
    /* the pitch wheel: rest 0, the ends +-1 */
    smk_pw_t w = { 0 };
    CHECK(smk_pw_bend(&w, 535, 0) == 0.0f, "at rest");
    CHECK(smk_pw_bend(&w, 640, 10) > 0.9f && smk_pw_bend(&w, 440, 20) < -0.9f, "the ends");
    CHECK(smk_pw_bend(&w, 540, 30) == 0.0f, "back at rest");
    smk_mw_t mw = { 0 };
    CHECK(smk_mw_val(&mw, 615) == 0.0f && smk_mw_val(&mw, 474) > 0.95f, "the mod wheel: bottom 0, up 1");
    /* the pedal: either polarity, learnt at power-on; held over two minutes once proven, it stays down */
    smk_ped_t ped; memset(&ped, 0, sizeof ped);
    smk_ped_sample(&ped, 985, 0);
    int ev = -1; for (int i = 0; i < 4; i++) { int r = smk_ped_sample(&ped, 0, 10 + i); if (r >= 0) ev = r; }
    CHECK(ev == 1, "pressed: down (%d)", ev);
    ev = -1; for (int i = 0; i < 4; i++) { int r = smk_ped_sample(&ped, 985, 100 + i); if (r >= 0) ev = r; }
    CHECK(ev == 0, "let go: up (%d)", ev);
    for (uint32_t ms = 200; ms < 40000; ms += 50) smk_ped_sample(&ped, 985, ms);   /* (up 40 s after a down: proven) */
    for (int i = 0; i < 4; i++) smk_ped_sample(&ped, 0, 40000 + i);
    ev = -1; for (uint32_t ms = 40100; ms < 200000; ms += 50) { int r = smk_ped_sample(&ped, 0, ms); if (r >= 0) ev = r; }
    CHECK(ev == -1 && ped.down, "a proven pedal held 160 s: let go by itself (%d)", ev);
    printf("smk_ctl: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
