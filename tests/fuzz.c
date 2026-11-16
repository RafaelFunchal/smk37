/* SPDX-License-Identifier: GPL-3.0-only
 * The readers of what comes from outside, fed damaged inputs (built with -fsanitize=address,undefined: no read or
 * write out of bounds, no undefined behaviour), and what they load then played: a project file, SLOOP 2.8's
 * projects and records, a user slot (and SLICE on it), the FM6 / SYN images, your sounds' bank, the editor's frames.
 *   fuzz [ROUNDS] [SEED] */
#include "../app/project.h"
#include "../app/psound.h"
#include "../app/presets.h"
#include "../app/sloopimport.h"
#include "../app/usersnd.h"
#include "../app/xbank.h"
#include "../app/edproto.h"
#include "../engine/sample.h"
#include "../engine/slice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t rs = 0x9E3779B97F4A7C15ull;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 16); }
static void mutate(uint8_t *b, uint32_t n, int k)          /* k changes: a byte, a bit, a run, an extreme value */
{
    for (int i = 0; i < k; i++) {
        uint32_t at = rnd() % n;
        switch (rnd() % 6) {
        case 0: b[at] = (uint8_t)rnd(); break;
        case 1: b[at] ^= (uint8_t)(1u << (rnd() & 7)); break;
        case 2: b[at] = (uint8_t)(rnd() & 1 ? 0xFF : 0); break;
        case 3: { uint32_t l = 1 + rnd() % 16; for (uint32_t j = 0; j < l && at + j < n; j++) b[at + j] = (uint8_t)rnd(); } break;
        case 4: b[at] = (uint8_t)(b[at] + (rnd() % 5) - 2); break;
        default: if (at + 4 <= n) { static const uint32_t X[] = { 0, 1, 0x7F, 0x80, 0xFF, 0x7FFF, 0x8000, 0xFFFF, 0x7FFFFFFF, 0x80000000u, 0xFFFFFFFFu, 0x7FC00000u, 0x7F800000u };
                     uint32_t v = X[rnd() % (sizeof X / 4)]; memcpy(b + at, &v, 4); } break;
        }
    }
}

static seq_t Q; static seq_hist H; static gb_sound S; static int16_t fx[GB_FX_MEM]; static int32_t out[2 * GB_BLOCK];
static project_extra X; static psounds PS;
static void on(seq_t *q, int t, int n, int v) { (void)q; snd_note_on(&S, GB_OWNER(OWN_SEQ, t), t, n, (float)v / 127.0f); }
static void off(seq_t *q, int t, int n) { (void)q; snd_note_off(&S, GB_OWNER(OWN_SEQ, t), t, n); }
static void lk(seq_t *q, int t, const seq_lock *l, int n)    /* (as the firmware: a lock's value onto the track's sound) */
{
    (void)q;
    for (int i = 0; i < n; i++) if (l[i].param < GB_NPARAM) S.trk[t].patch.p[l[i].param] = (float)l[i].value * (1.0f / 255.0f);
}
static void fresh(void)
{
    seq_init(&Q, 44100, 120, &H); snd_init(&S, fx, 0);
    Q.note_on = on; Q.note_off = off; Q.locks = lk;
    memset(&X, 0, sizeof X); memset(&PS, 0, sizeof PS); X.ps = &PS;
}
static float play(int blocks)                              /* the loaded project played: its peak (no NaN, no wild value) */
{
    float pk = 0.0f;
    seq_play(&Q, 1);
    for (int b = 0; b < blocks; b++) {
        seq_advance(&Q, GB_BLOCK); snd_render(&S, out);
        for (int i = 0; i < 2 * GB_BLOCK; i++) { float a = (float)(out[i] < 0 ? -out[i] : out[i]); if (a > pk) pk = a; }
    }
    seq_play(&Q, 0); snd_kill(&S);
    return pk;
}
static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- a project file */
static uint8_t seedp[16384], buf[16384];
static void projects(int rounds)
{
    fresh();
    for (int t = 0; t < SEQ_NTRK; t++) S.trk[t].patch.engine = (uint8_t)((t * 3 + 1) % gb_nengines);
    for (int i = 0; i < 60; i++) seq_step_toggle(&Q, i % SEQ_NTRK, i % 16, 30 + i, 50 + i);
    seq_step_ratchet(&Q, 2, 3, 3); seq_lock_set(&Q, 2, 3, 6, 77); seq_step_cond(&Q, 1, 0, SEQ_CHANCE + 2);
    Q.song_n = 3; Q.song_len = 12; Q.song[1].bar = 4; Q.song[2].bar = 8;
    for (int t = 0; t < SEQ_NTRK; t++) ps_fill(&PS, t, 0, &S.trk[t].patch, S.trk[t].patch.p, 5);
    PS.s[1][2].engine = ENG_FM6; PS.s[1][2].p[6] = 200;
    int n = project_save(&Q, &S, &X, seedp, sizeof seedp);
    printf("projects: %d rounds on a %d-byte file\n", rounds, n);
    if (n <= 0) { fails++; return; }
    int loaded = 0;
    for (int r = 0; r < rounds; r++) {
        uint32_t len = (uint32_t)n;
        memcpy(buf, seedp, len);
        mutate(buf + 5, len - 5, 1 + (int)(rnd() % 12));
        if (rnd() % 8 == 0) len = 5 + rnd() % (len - 5);
        if (rnd() % 16 == 0) buf[4] = (uint8_t)(1 + rnd() % 14);   /* (an older format's reading of the same bytes) */
        fresh();
        if (project_load(&Q, &S, &X, buf, len) == 0) {
            loaded++;
            for (int t = 0; t < SEQ_NTRK; t++) {
                CHECK(S.trk[t].patch.engine < gb_nengines, "an engine out of range");
                for (int i = 0; i < SEQ_NPAT; i++) CHECK(Q.t[t].pat[i].n <= SEQ_PATEV && Q.t[t].pat[i].nlock <= SEQ_LOCKS && Q.t[t].pat[i].steps >= 1 && Q.t[t].pat[i].steps <= SEQ_MAXSTEPS, "a pattern out of range");
                /* each pattern's own sound onto the track, as a change of pattern does */
                gb_patch p = S.trk[t].patch; float own[GB_NPARAM]; int pre;
                ps_give(&PS, t, (int)(rnd() % SEQ_NPAT), &p, own, &pre);
                CHECK(p.engine < gb_nengines, "a pattern's sound with an engine out of range");
                if (rnd() & 1) S.trk[t].patch = p;
            }
            CHECK(Q.song_n <= SEQ_SONG, "a song out of range");
            float pk = play(40);
            CHECK(pk == pk && pk < 2.2e9f, "a loaded project played wild (%g)", pk);
        }
    }
    printf("  %d loaded and played\n", loaded);
}

/* ---- SLOOP 2.8's: a project's sections, the working project, its records */
static uint8_t A[SLOOP_PROJ_LEN], W[SLOOP_PROJ_LEN];
static void w16(uint8_t *b, int v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }
static void w32(uint8_t *b, uint32_t v) { w16(b, (int)(v & 0xFFFF)); w16(b + 2, (int)(v >> 16)); }
static void sbase(uint8_t *p)
{
    memset(p, 0, SLOOP_PROJ_LEN); w32(p, 0x46554E36u); w32(p + 4, 3816u); w16(p + 8, 100); w16(p + 10, 30);
    for (int t = 0; t < 4; t++) {
        uint8_t *tk = p + 76 + t * 934;
        w16(tk + 0, 104); w16(tk + 2 * 29, 16); w16(tk + 2 * 30, 2); w16(tk + 2 * 32, 64);
        for (int s = 0; s < 64; s++) { tk[134 + 10 * s] = (uint8_t)(40 + s % 30); tk[134 + 10 * s + 4] = 1; tk[134 + 10 * s + 5] = (uint8_t)(s % 3); tk[134 + 10 * s + 7] = 90; }
        for (int i = 0; i < 24; i++) { tk[838 + 4 * i] = (uint8_t)(i < 4 ? i * 5 : 0xFF); tk[838 + 4 * i + 1] = (uint8_t)(60 + i); }
        tk[132] = (uint8_t)t; tk[133] = 0;
    }
}
static void sloops(int rounds)
{
    printf("SLOOP 2.8: %d rounds (projects, sounds, kits, MIDI, songs)\n", rounds);
    int imp = 0;
    for (int r = 0; r < rounds; r++) {
        sbase(A); sbase(W);
        mutate(A + 8, SLOOP_PROJ_LEN - 8, 1 + (int)(rnd() % 40));
        mutate(W + 8, SLOOP_PROJ_LEN - 8, 1 + (int)(rnd() % 40));
        if (rnd() % 4 == 0) mutate(A, 8, 1);
        fresh();
        const uint8_t *secs[4] = { A, rnd() & 1 ? W : 0, 0, rnd() & 1 ? A : 0 };
        int po[5] = { -1, -1, -1, -1, -1 };
        if (sloop_import(secs, W, &Q, &S, &X, po)) {
            imp++;
            for (int t = 0; t < 5; t++) CHECK(S.trk[t].patch.engine < gb_nengines && (po[t] < 0 || po[t] < gb_npresets || po[t] >= US_BASE), "an import out of range (%d %d)", S.trk[t].patch.engine, po[t]);
            float pk = play(30);
            CHECK(pk == pk && pk < 2.2e9f, "an import played wild");
        }
        fresh(); sloop_sound(A + 76 + (rnd() % 4) * 934, (int)(rnd() % 5), &S);
        uint8_t pl[2048]; for (uint32_t i = 0; i < sizeof pl; i++) pl[i] = (uint8_t)rnd();
        int pn = (int)(rnd() % sizeof pl);
        gb_dsnd kits[4][16]; uint8_t cr[4]; sloop_synkits(pl, pn, kits, cr);
        uint16_t ml[16]; sloop_mset ms; sloop_midi(pl, pn, ml, &ms);
        gb_patch p; char nm[13]; uint8_t f4[4], a4[4]; int tr; sloop_upreset(pl, &p, nm, f4, a4, &tr);
        CHECK(strlen(nm) <= 12, "a record's name not ended");
        fresh(); sloop_song(pl, pn, secs, &Q);
        CHECK(Q.song_n <= SEQ_SONG, "a SLOOP song out of range");
    }
    printf("  %d imported and played\n", imp);
}

/* ---- your sounds' bank, the FM6 and SYN images */
static uint8_t usb[US_FILE], img[XB_SYN_LEN > XB_FM6_LEN ? XB_SYN_LEN : XB_FM6_LEN];
static void banks(int rounds)
{
    printf("your sounds, FM6, SYN: %d rounds\n", rounds);
    fresh();
    memset(usb, 0xFF, sizeof usb);
    us_sound s; memset(&s, 0, sizeof s);
    for (int k = 0; k < US_N; k += 3) { us_take(&S, k % 5, S.trk[k % 5].patch.p, "SOUND", &s); s.engine = (uint8_t)(k % gb_nengines); us_put(usb, k, &s); }
    static uint8_t seed[US_FILE]; memcpy(seed, usb, sizeof usb);
    for (int r = 0; r < rounds; r++) {
        memcpy(usb, seed, sizeof usb); mutate(usb, sizeof usb, 1 + (int)(rnd() % 30));
        us_from_bytes(usb, rnd() % 4 ? (int)sizeof usb : (int)(rnd() % sizeof usb));
        for (int k = 0; k < US_N; k++) {
            CHECK(strlen(us_name(k)) < 12, "a sound's name not ended");
            if (us_get(usb, k, &s)) {
                fresh(); us_apply(&S, k % 5, &s);
                CHECK(S.trk[k % 5].patch.engine < gb_nengines, "a sound with an engine out of range (%d)", S.trk[k % 5].patch.engine);
                if (k % 8 == 0) { snd_note_on(&S, GB_OWNER(OWN_KEY, 0), k % 5, 60, 1.0f); for (int b = 0; b < 8; b++) snd_render(&S, out); }
            }
        }
        for (uint32_t i = 0; i < sizeof img; i++) img[i] = (uint8_t)rnd();
        int fm6 = (int)(rnd() & 1); uint32_t used;
        xb_head(img, fm6, rnd(), fm6 ? XB_FM6_LEN : XB_SYN_LEN);
        if (rnd() & 1) mutate(img, sizeof img, 2);
        if (!xb_check(img, fm6 ? XB_FM6_LEN : XB_SYN_LEN, fm6, &used) && !fm6) {
            int k = (int)(rnd() % 4);
            xb_kit_unpack(img + 16 + (uint32_t)k * XB_KIT_LEN, gb_dsyn[k], &gb_dsyn_crush[k]); drum_syn_forget(k);
            fresh(); gb_patch_default(&S.trk[3].patch, ENG_DRUM); gb_patch_set(&S.trk[3].patch, SP_E0, GB_KIT_SYN + k);
            for (int i = 0; i < 16; i++) snd_note_on(&S, GB_OWNER(OWN_KEY, 0), 3, i, 1.0f);
            for (int b = 0; b < 30; b++) snd_render(&S, out);
        }
    }
}

/* ---- a user slot (the editor writes it), SAMPLE GRAIN and SLICE on it */
static uint8_t slot[SMP_USR_SIZE], sseed[SMP_USR_SIZE];
static void slots(int rounds)
{
    printf("user slots: %d rounds\n", rounds);
    memset(sseed, 0xFF, sizeof sseed);
    const smp_set *pn = smp_set_at(0);
    uint32_t off = 0;
    memcpy(sseed, "FSMP", 4); sseed[4] = 1; sseed[5] = 0; sseed[6] = 3; memcpy(sseed + 8, "FUZZ\0\0\0\0", 8);
    for (int i = 0; i < 3; i++) {
        smp_zone z = *smp_zone_at(pn->z0 + i); uint32_t nb = (z.n + 1u) / 2u;
        if (nb > 20000u) { z.n = 40000u; nb = 20000u; if (z.le >= z.n) z.le = z.ls = z.n - 1u; }
        memcpy(sseed + 512 + off, smp_data() + z.off, nb); z.off = off; off += nb;
        z.lo = (uint8_t)(i * 40); z.hi = (uint8_t)(i * 40 + 39);
        memcpy(sseed + 32 + 28 * i, &z, 28);
    }
    w32(sseed + 16, off);
    int ok = 0;
    for (int r = 0; r < rounds; r++) {
        memcpy(slot, sseed, sizeof slot);
        mutate(slot, 512, 1 + (int)(rnd() % 8));
        if (rnd() % 4 == 0) mutate(slot + 512, 60000, 50);
        int k = (int)(rnd() % SMP_NUSR);
        int h = smp_user_head_ok(slot);
        smp_user_scan(k, slot);
        CHECK(!!h == !!smp_user_nz(k), "the header's check and the scan disagree");
        if (!smp_user_nz(k)) { smp_user_scan(k, 0); continue; }
        ok++;
        static const int ENG[3] = { ENG_SAMPLE, ENG_GRAIN, ENG_SLICE };
        for (int e = 0; e < 3; e++) {
            fresh(); sm_jump(&S.master, 1.0f);
            gb_patch *p = &S.trk[1].patch; gb_patch_default(p, ENG[e]);
            gb_patch_set(p, SP_E0, ENG[e] == ENG_SLICE ? k + 1 : smp_nsets() + k);
            if (ENG[e] == ENG_SLICE) { gb_patch_set(p, SP_E0 + 1, (int)(rnd() % 5)); gb_patch_set(p, SP_E0 + 4, (int)(rnd() % 3)); gb_patch_set(p, SP_E0 + 5, (int)(rnd() & 1)); gb_patch_set(p, SP_E0 + 2, (int)(rnd() % 32)); }
            for (int n = 0; n < 4; n++) snd_note_on(&S, GB_OWNER(OWN_KEY, 0), 1, (int)(rnd() % 128), 1.0f);
            float pk = 0;
            for (int b = 0; b < 60; b++) { snd_render(&S, out); for (int i = 0; i < 2 * GB_BLOCK; i++) { float a = (float)(out[i] < 0 ? -out[i] : out[i]); if (a > pk) pk = a; } if (b == 30) snd_all_off(&S); }
            CHECK(pk == pk && pk < 2.2e9f, "a user slot played wild");
        }
        if (rnd() & 1) { mutate(slot, 512, 3); smp_user_scan(k, slot); }   /* (written again while it may sound) */
        smp_user_scan(k, 0);
    }
    printf("  %d slots taken and played\n", ok);
}

/* ---- the editor's frames */
static void frames(int rounds)
{
    printf("editor frames: %d rounds\n", rounds);
    uint8_t raw[600], fr[1400], back[700]; uint32_t n;
    int good = 0;
    for (int r = 0; r < rounds; r++) {
        uint32_t rn = rnd() % sizeof raw; for (uint32_t i = 0; i < rn; i++) raw[i] = (uint8_t)rnd();
        uint32_t fl = ed_frame((int)(rnd() % 16), raw, rn, fr, sizeof fr);
        if (!fl) continue;
        if (rnd() & 1) mutate(fr, fl, 1 + (int)(rnd() % 4));
        if (rnd() % 4 == 0) fl = rnd() % fl;
        uint32_t cap = rnd() & 1 ? sizeof back : rnd() % sizeof back;
        int c = ed_parse(fr, fl, back, cap, &n);
        if (c >= 0) { good++; CHECK(n <= cap, "a frame longer than its room"); }
        uint8_t pk[4]; for (uint32_t i = 0; i < fl + 3; i += 3) ed_usb_packet(fr, fl, i, pk);
    }
    printf("  %d frames read back\n", good);
}

int main(int argc, char **argv)
{
    int rounds = argc > 1 ? atoi(argv[1]) : 3000;
    if (argc > 2) rs ^= (uint64_t)strtoull(argv[2], 0, 0) * 0x2545F4914F6CDD1Dull;
    if (!smp_bank_load("assets/samples.bin")) { printf("no bank\n"); return 1; }
    projects(rounds);
    sloops(rounds);
    banks(rounds / 4);
    slots(rounds / 6);
    frames(rounds * 4);
    printf("%s\n", fails ? "FAILED" : "fuzz: nothing found");
    return fails != 0;
}
