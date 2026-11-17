/* SPDX-License-Identifier: GPL-3.0-only
 * PIANO (engine/piano.c) on the PC: its flash image (tools/mkpiano.py, build/piano/piano.img) loaded at the end of
 * mapped pages (a read past it faults), the engine driven through its gb_engine as sound.c drives it (start, a block
 * at a time render with the voice's pitch, release; the voice's pan applied). A demo to listen to:
 * build/piano/piano_demo.wav.
 *   gcc -O2 -std=gnu11 -o build/piano/piano_test tests/piano_test.c engine/piano.c engine/common.c -lm */
#include "../engine/engine.h"
#include "../engine/piano.h"
#include "../engine/erk_bank.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>

volatile uint8_t gb_flash_ok = 1;          /* (sample.c's, not linked here) */
static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint8_t *map; static size_t map_len; static const uint8_t *img; static size_t img_len;
static void load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { printf("piano: no image %s (python3 tools/mkpiano.py)\n", path); exit(2); }
    fseek(f, 0, SEEK_END); img_len = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    size_t pg = (size_t)sysconf(_SC_PAGESIZE), pages = (img_len + pg - 1) / pg;
    map_len = (pages + 1) * pg;
    map = mmap(0, map_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mprotect(map + pages * pg, pg, PROT_NONE);                  /* a guard page right after the image */
    uint8_t *p = map + pages * pg - img_len;                    /* (img_len is a multiple of 4) */
    if (fread(p, 1, img_len, f) != img_len) { printf("piano: short read\n"); exit(2); }
    fclose(f);
    img = p;
}
static void flash(int on)                                     /* the window: off = every page unreadable */
{
    size_t pg = (size_t)sysconf(_SC_PAGESIZE);
    mprotect(map, map_len - pg, on ? PROT_READ : PROT_NONE);
    gb_flash_ok = (uint8_t)on;
}

static gb_patch P;
static void patch(void)
{
    memset(&P, 0, sizeof P);
    for (int k = 0; k < 8; k++) P.p[k] = gb_nv(gb_eng_piano.ed[k].def, gb_eng_piano.ed[k].min, gb_eng_piano.ed[k].max);
}
static void pset(int k, int v) { P.p[k] = gb_nv(v, gb_eng_piano.ed[k].min, gb_eng_piano.ed[k].max); }
static void vstart(gb_voice *v, int note, float vel)
{
    memset(v, 0, sizeof *v);
    v->state = V_ON; v->note = (uint8_t)note; v->vel = vel; v->pan_l = v->pan_r = 0.70710678f;
    gb_eng_piano.start(v, &P, 1);
}
static void vrelease(gb_voice *v) { v->state = V_REL; gb_eng_piano.release(v, &P); }
/* n blocks into buf (mono, may be 0); returns 1 while the voice lives */
static int vrender(gb_voice *v, float pitch, int blocks, float *buf, int *finite)
{
    gb_mod m; memset(&m, 0, sizeof m);
    m.pitch = pitch; m.dt = gb_note_hz(pitch) * GB_ISR;
    float o[GB_BLOCK];
    for (int b = 0; b < blocks; b++) {
        if (v->state == V_FREE) { if (buf) memset(buf + b * GB_BLOCK, 0, sizeof o * (size_t)(blocks - b)); return 0; }
        int alive = gb_eng_piano.render(v, &P, &m, o);
        for (int i = 0; i < GB_BLOCK; i++) {
            if (!isfinite(o[i]) && finite) *finite = 0;
            if (buf) buf[b * GB_BLOCK + i] = o[i];
        }
        if (!alive) v->state = V_FREE;
    }
    return v->state != V_FREE;
}
static float rms(const float *x, int a, int b) { double s = 0; for (int i = a; i < b; i++) s += (double)x[i] * x[i]; return (float)sqrt(s / (b - a)); }
static float peak(const float *x, int n) { float p = 0; for (int i = 0; i < n; i++) p = fabsf(x[i]) > p ? fabsf(x[i]) : p; return p; }
static int S(float sec) { return (int)(sec * GB_SR / GB_BLOCK); }       /* seconds -> blocks */
#define MAXB 8000
static float B1[MAXB * GB_BLOCK], B2[MAXB * GB_BLOCK];

/* the period by normalised autocorrelation within [lo, hi] samples, parabolic interpolation */
static float period(const float *x, int n, float lo, float hi)
{
    int a = (int)lo, b = (int)hi + 1, best = a; double bv = -2;
    static double r[4096];
    for (int l = a - 1; l <= b + 1; l++) {
        double s = 0, e0 = 0, e1 = 0;
        for (int i = 0; i + l < n; i++) { s += x[i] * x[i + l]; e0 += x[i] * x[i]; e1 += x[i + l] * x[i + l]; }
        r[l] = s / sqrt(e0 * e1 + 1e-30);
        if (l >= a && l <= b && r[l] > bv) { bv = r[l]; best = l; }
    }
    double y0 = r[best - 1], y1 = r[best], y2 = r[best + 1], d = y0 - 2 * y1 + y2;
    return (float)best + (d != 0 ? (float)(0.5 * (y0 - y2) / d) : 0.0f);
}

static void keys_test(void)
{
    printf("every key at four velocities\n");
    static const float vel[4] = { 0.15f, 0.45f, 0.75f, 1.0f };
    gb_voice v;
    int bad = 0, quiet = 0, loud = 0, order = 0;
    float lo = 9, hi = 0;
    for (int k = 21; k <= 108; k++) {
        float r[4];
        for (int j = 0; j < 4; j++) {
            int fin = 1;
            vstart(&v, k, vel[j]);
            vrender(&v, (float)k, S(0.4f), B1, &fin);
            float pk = peak(B1, S(0.4f) * GB_BLOCK);
            r[j] = rms(B1, 0, S(0.4f) * GB_BLOCK);
            bad += !fin; quiet += pk < 0.003f; loud += pk > 1.2f;
            lo = pk < lo ? pk : lo; hi = pk > hi ? pk : hi;
        }
        if (!(r[0] < r[1] && r[1] < r[2] && r[2] < r[3])) { order++; printf("    key %d rms %.4f %.4f %.4f %.4f\n", k, r[0], r[1], r[2], r[3]); }
    }
    printf("  peaks %.4f .. %.4f\n", lo, hi);
    CHECK(bad == 0, "%d renders not finite", bad);
    CHECK(quiet == 0, "%d renders nearly silent", quiet);
    CHECK(loud == 0, "%d renders past 1.2", loud);
    CHECK(order == 0, "%d keys not louder with velocity", order);
    /* notes outside the piano's keys: transposed from the end keys, finite */
    for (int k = 0; k < 128; k += 127) { int fin = 1; vstart(&v, k, 0.8f); vrender(&v, (float)k, 50, B1, &fin); CHECK(fin && peak(B1, 50 * GB_BLOCK) > 0.0f, "note %d", k); }
}

static void decay_release_test(void)
{
    printf("decay, release, dampers, the release noise\n");
    gb_voice v;
    int fin = 1;
    vstart(&v, 60, 0.8f);
    int alive = vrender(&v, 60, S(4.0f), B1, &fin);
    float early = rms(B1, S(0.1f) * GB_BLOCK, S(0.4f) * GB_BLOCK), late = rms(B1, S(3.5f) * GB_BLOCK, S(4.0f) * GB_BLOCK);
    printf("  C4 held: rms %.4f at 0.1 s, %.4f at 3.5 s\n", early, late);
    CHECK(alive && fin, "a held C4 ended within 4 s");
    CHECK(late < early * 0.5f && late > early * 0.005f, "a held note's decay: %.4f -> %.4f", early, late);
    /* released at 0.5 s: much quieter 0.3 s later than held, and over within 3 s */
    patch(); pset(5, 0);                                         /* (no noise: the string alone) */
    vstart(&v, 60, 0.8f); vrender(&v, 60, S(0.5f), B1, 0); vrelease(&v);
    alive = vrender(&v, 60, S(0.6f), B2, &fin);
    float rel = rms(B2, S(0.3f) * GB_BLOCK, S(0.6f) * GB_BLOCK), held;
    vstart(&v, 60, 0.8f); vrender(&v, 60, S(0.8f), 0, 0); vrender(&v, 60, S(0.3f), B1, 0);
    held = rms(B1, 0, S(0.3f) * GB_BLOCK);
    printf("  C4 0.3 s after its release: rms %.5f (held: %.4f)\n", rel, held);
    CHECK(rel < held * 0.05f, "the damper: %.5f vs %.4f held", rel, held);
    vstart(&v, 60, 0.8f); vrender(&v, 60, S(0.5f), 0, 0); vrelease(&v);
    int b = 0; while (vrender(&v, 60, 1, 0, &fin) && b < S(10.0f)) b++;
    printf("  C4 released: ends after %.2f s\n", b * GB_BLOCK / GB_SR);
    CHECK(v.state == V_FREE && b < S(3.0f), "a released note did not end (%d blocks)", b);
    /* the keys without a damper ring on */
    const erk_bank_hdr *h = (const erk_bank_hdr *)(const void *)img;
    const erk_key *ky = (const erk_key *)(const void *)(img + sizeof *h);
    int nd = -1, n_nd = 0;
    for (int k = 0; k < ERK_KEYS; k++) if (ky[k].flags & ERK_NODAMP) { n_nd++; if (nd < 0) nd = k + ERK_KEY0; }
    printf("  %d keys without a damper, from %d\n", n_nd, nd);
    CHECK(nd > 0, "no key without a damper");
    if (nd > 0) {
        vstart(&v, nd, 0.8f); vrender(&v, nd, S(0.3f), 0, 0); vrelease(&v);
        alive = vrender(&v, nd, S(1.0f), B2, &fin);
        vstart(&v, nd, 0.8f); vrender(&v, nd, S(1.3f), B1, 0);
        float a = rms(B2, S(0.7f) * GB_BLOCK, S(1.0f) * GB_BLOCK), hb = rms(B1, S(1.0f) * GB_BLOCK, S(1.3f) * GB_BLOCK);
        printf("  key %d 1 s after its release: rms %.5f (held: %.5f)\n", nd, a, hb);
        CHECK(alive && a > hb * 0.9f, "an undamped key was damped");
        CHECK(fabsf(a - hb) < hb * 0.01f, "an undamped key's release changed it (no noise either): %.5f vs %.5f", a, hb);
    }
    /* the release noise: the same note let go with and without it; the difference is the noise */
    patch(); pset(5, 0);
    vstart(&v, 48, 1.0f); vrender(&v, 48, S(0.3f), 0, 0); vrelease(&v); vrender(&v, 48, S(0.4f), B1, 0);
    patch(); pset(5, 100);
    vstart(&v, 48, 1.0f); vrender(&v, 48, S(0.3f), 0, 0); vrelease(&v); vrender(&v, 48, S(0.4f), B2, &fin);
    for (int i = 0; i < S(0.4f) * GB_BLOCK; i++) B2[i] -= B1[i];
    float nz = rms(B2, 0, S(0.2f) * GB_BLOCK), str = rms(B1, 0, S(0.2f) * GB_BLOCK);
    printf("  C3's release noise: rms %.5f (the string's release: %.5f)\n", nz, str);
    CHECK(nz > 1e-4f && nz > str * 0.02f, "no release noise");
    patch();
}

static void flash_test(void)
{
    printf("the flash window off (a save)\n");
    gb_voice v, w;
    int fin = 1;
    vstart(&v, 64, 0.9f); vrender(&v, 64, S(0.2f), 0, 0);
    gb_voice keep = v;
    flash(0);                                                    /* any read of the image faults from here */
    int alive = vrender(&v, 64, 100, B1, &fin);
    CHECK(alive && peak(B1, 100 * GB_BLOCK) == 0.0f, "a voice not silent while the flash is off");
    CHECK(memcmp(&keep, &v, sizeof v) == 0, "a voice moved on while the flash was off");
    vstart(&w, 67, 0.9f);                                        /* a note started then: silent, and over */
    alive = vrender(&w, 67, 2, B1, &fin);
    CHECK(!alive && peak(B1, 2 * GB_BLOCK) == 0.0f, "a note started while the flash is off sounded");
    vrelease(&v);                                                /* released then: its noise waits for the flash */
    alive = vrender(&v, 64, 10, B1, &fin);
    CHECK(alive && peak(B1, 10 * GB_BLOCK) == 0.0f, "a voice released while the flash is off");
    flash(1);
    vrender(&v, 64, S(0.3f), B1, &fin);
    CHECK(fin && peak(B1, S(0.3f) * GB_BLOCK) > 1e-4f, "the voice did not come back with the flash");
    /* the bank gone: silence, the voices end */
    vstart(&v, 60, 0.8f); vrender(&v, 60, 10, 0, 0);
    piano_bank_set(0);
    CHECK(!piano_bank_ok() && !vrender(&v, 60, 1, B1, 0) && peak(B1, GB_BLOCK) == 0.0f, "no bank: still sounding");
    uint8_t bad[64]; memset(bad, 0, sizeof bad);
    piano_bank_set(bad);
    CHECK(!piano_bank_ok(), "a bad image taken");
    piano_bank_set(img);
    CHECK(piano_bank_ok(), "the image refused after");
}

static float measure(int key, float pitch)                    /* a sounding voice's period, around the pitch's */
{
    gb_voice v;
    float per = GB_SR / gb_note_hz(pitch);
    vstart(&v, key, 0.7f); vrender(&v, (float)key, S(0.3f), 0, 0); vrender(&v, pitch, 2, 0, 0); vrender(&v, pitch, 160, B1, 0);
    return period(B1, 160 * GB_BLOCK - (int)(per * 1.2f), per * 0.89f, per * 1.12f);
}
static void pitch_test(void)
{
    printf("pitch: the keys' tuning, zone 0's keys, the bend\n");
    static const int keys[] = { 23, 26, 33, 45, 57, 69, 81 };
    float got[7];
    for (unsigned j = 0; j < sizeof keys / sizeof keys[0]; j++) {
        int k = keys[j];
        float per = GB_SR / gb_note_hz((float)k);
        got[j] = measure(k, (float)k);
        float cents = 1200.0f * log2f(per / got[j]);
        printf("  key %d: period %.2f samples (equal temperament %.2f): %+.1f cents (the recording's tuning)\n", k, got[j], per, cents);
        CHECK(fabsf(cents) < 25.0f, "key %d off by %.1f cents", k, cents);
    }
    /* zone 0 dropped: key 23 plays zone 1's sample 3 semitones down from key 26 */
    float c = 1200.0f * log2f(got[0] / got[1]);
    printf("  key 23 against key 26 (the same sample now): %.1f cents (300 expected)\n", c);
    CHECK(fabsf(c - 300.0f) < 3.0f, "zone 0's key 23: %.1f cents from key 26", c);
    /* the bend: +2 semitones on a sounding A4, against A4 */
    float b = measure(69, 71.0f), a4 = got[5];
    c = 1200.0f * log2f(a4 / b);
    printf("  A4 bent +2: %.1f cents above A4 (200 expected)\n", c);
    CHECK(fabsf(c - 200.0f) < 3.0f, "the bend: %.1f cents", c);
    b = measure(69, 68.5f); c = 1200.0f * log2f(a4 / b);
    printf("  A4 bent -0.5: %.1f cents (-50 expected)\n", c);
    CHECK(fabsf(c + 50.0f) < 3.0f, "the bend down: %.1f cents", c);
}

/* ---- the demo: a few chords and a scale, a small voice pool, stereo */
#define NV 24
static gb_voice pool[NV];
static int16_t *wav; static size_t wn, wcap;
static void demo_on(int note, float vel)
{
    int i = 0;
    for (int j = 0; j < NV; j++) if (pool[j].state == V_FREE) { i = j; goto got; }
    for (int j = 0; j < NV; j++) if (pool[j].age < pool[i].age) i = j;
got:
    vstart(&pool[i], note, vel); pool[i].age = (uint32_t)wn;
}
static void demo_off(int note) { for (int j = 0; j < NV; j++) if (pool[j].state == V_ON && pool[j].note == note) vrelease(&pool[j]); }
static double cpu_s; static long vblocks;
static void demo_run(float sec)
{
    for (int b = 0; b < S(sec); b++) {
        float l[GB_BLOCK] = { 0 }, r[GB_BLOCK] = { 0 }, o[GB_BLOCK];
        gb_mod m; memset(&m, 0, sizeof m);
        for (int j = 0; j < NV; j++) {
            gb_voice *v = &pool[j];
            if (v->state == V_FREE) continue;
            m.pitch = v->note;
            clock_t c = clock();
            int alive = gb_eng_piano.render(v, &P, &m, o);
            cpu_s += (double)(clock() - c) / CLOCKS_PER_SEC; vblocks++;
            for (int i = 0; i < GB_BLOCK; i++) { l[i] += o[i] * v->pan_l; r[i] += o[i] * v->pan_r; }
            if (!alive) v->state = V_FREE;
        }
        if (wn + 2 * GB_BLOCK > wcap) { wcap = wcap ? 2 * wcap : 1 << 20; wav = realloc(wav, wcap * 2); }
        for (int i = 0; i < GB_BLOCK; i++) {
            float x = gb_clampf(l[i] * 1.4f, -1, 1), y = gb_clampf(r[i] * 1.4f, -1, 1);
            wav[wn++] = (int16_t)(x * 32767); wav[wn++] = (int16_t)(y * 32767);
        }
    }
}
static void put32(FILE *f, uint32_t x) { fputc(x & 255, f); fputc(x >> 8 & 255, f); fputc(x >> 16 & 255, f); fputc(x >> 24, f); }
static void put16(FILE *f, uint32_t x) { fputc(x & 255, f); fputc(x >> 8 & 255, f); }
static void demo(const char *path)
{
    patch();
    static const int chords[4][4] = { { 48, 55, 64, 72 }, { 45, 52, 60, 69 }, { 41, 53, 57, 65 }, { 43, 50, 59, 67 } };
    for (int c = 0; c < 4; c++) {
        for (int j = 0; j < 4; j++) demo_on(chords[c][j], 0.45f + 0.12f * (float)j);
        demo_run(1.1f);
        for (int j = 0; j < 4; j++) demo_off(chords[c][j]);
        demo_run(0.15f);
    }
    static const int sc[] = { 0, 2, 4, 5, 7, 9, 11, 12 };
    for (int o = 0; o < 2; o++) for (int j = 0; j < 8; j++) {
        int n = 48 + 12 * o + sc[j];
        demo_on(n, 0.4f + 0.04f * (float)(o * 8 + j)); demo_run(0.22f); demo_off(n);
    }
    demo_on(21, 0.9f); demo_on(33, 0.9f); demo_on(96, 0.7f); demo_on(108, 0.7f);   /* the ends: zone 0's keys, the undamped top */
    demo_run(2.0f); demo_off(21); demo_off(33); demo_off(96); demo_off(108);
    demo_on(60, 1.0f); demo_on(64, 1.0f); demo_on(67, 1.0f); demo_on(72, 1.0f); demo_run(3.5f);
    demo_off(60); demo_off(64); demo_off(67); demo_off(72); demo_run(1.5f);
    FILE *f = fopen(path, "wb");
    if (!f) { printf("piano: cannot write %s\n", path); return; }
    fwrite("RIFF", 1, 4, f); put32(f, 36 + (uint32_t)wn * 2); fwrite("WAVEfmt ", 1, 8, f); put32(f, 16); put16(f, 1); put16(f, 2);
    put32(f, (uint32_t)GB_SR); put32(f, (uint32_t)GB_SR * 4); put16(f, 4); put16(f, 16); fwrite("data", 1, 4, f); put32(f, (uint32_t)wn * 2);
    fwrite(wav, 2, wn, f); fclose(f);
    printf("demo: %s (%.1f s); the host: %.2f us a voice-block\n", path, (double)wn / 2 / GB_SR, cpu_s * 1e6 / (double)(vblocks ? vblocks : 1));
}

int main(int argc, char **argv)
{
    load(argc > 1 ? argv[1] : "build/piano/piano.img");
    piano_bank_set(img);
    CHECK(piano_bank_ok(), "the image refused");
    if (!piano_bank_ok()) { printf("piano: %d checks, %d failed\n", checks, fails); return 1; }
    patch();
    keys_test();
    decay_release_test();
    flash_test();
    pitch_test();
    demo(argc > 2 ? argv[2] : "build/piano/piano_demo.wav");
    printf("piano: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
