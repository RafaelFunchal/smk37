/* SPDX-License-Identifier: GPL-3.0-only
 * The denormal hunt (3.1.8). A float that decays towards zero without a floor ends among the denormals, where an
 * FPU without their fast path takes a trap or microcode for every operation: on a small CPU, a block that takes
 * many times its usual time, a crackle long after the last note. Here the PC's FPU is told to trap on a denormal
 * operand and on an underflow; every factory sound is played, let go, and its tail, the echoes and the reverb are
 * rendered until they are long gone; then every INSERT, every SLICER, every punch-in, the texture, the master's
 * filter, the chorus. Each place that trapped is listed (addr2line gives the line). The rule: no scenario with more
 * than MAXB blocks in which something trapped (a state crossing the denormals once as a note dies is nothing; a place
 * that stays there is what costs).
 *   denorm_test [SECONDS_OF_TAIL]     (x86-64 only; elsewhere it says so and passes) */
#define _GNU_SOURCE
#include "../app/presets.h"
#include "../engine/sample.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__x86_64__) && defined(__linux__)
#include <signal.h>
#include <ucontext.h>
#include <xmmintrin.h>
#define NSITE 256
#define MAXB 8
static long sc_blocks, sc_worst; static char sc_worst_what[48]; static int sc_bad;
static unsigned long site[NSITE]; static long hits[NSITE]; static int nsite; static const char *what = ""; static char site_what[NSITE][48];
static void on_fpe(int sig, siginfo_t *si, void *uc_)
{
    ucontext_t *uc = uc_; unsigned long ip = (unsigned long)uc->uc_mcontext.gregs[REG_RIP];
    int i = 0; for (; i < nsite; i++) if (site[i] == ip) break;
    if (i == nsite && nsite < NSITE) { site[nsite] = ip; strncpy(site_what[nsite], what, 47); nsite++; }
    if (i < NSITE) hits[i]++;
    sc_blocks++;
    uc->uc_mcontext.fpregs->mxcsr |= 0x0100 | 0x0800;      /* mask them until the block ends (the instruction goes on) */
    uc->uc_mcontext.fpregs->mxcsr &= ~0x003Fu;
    (void)sig; (void)si;
}
static void arm(void) { _mm_setcsr((_mm_getcsr() & ~(0x0100u | 0x0800u)) & ~0x003Fu); }
static void trap_init(void) { struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_sigaction = on_fpe; sa.sa_flags = SA_SIGINFO; sigaction(SIGFPE, &sa, 0); }
#else
#define MAXB 8
static long sc_blocks, sc_worst; static char sc_worst_what[48]; static int sc_bad;
static int nsite; static void arm(void) {} static void trap_init(void) { printf("denormals: not an x86-64 Linux host, nothing trapped\n"); }
static const char *what;
#endif

static int16_t fx[GB_FX_MEM], pm[GB_PUNCH_MEM];
static gb_sound S;
static void run(float secs) { int32_t o[GB_BLOCK * 2]; int nb = (int)(secs * GB_SR / GB_BLOCK); for (int b = 0; b < nb; b++) { arm(); snd_render(&S, o); if (S.grid >= 0) { S.grid += GB_BLOCK; if (S.grid >= S.beat * 4) { S.grid -= S.beat * 4; S.bar++; } } } }
static void fresh(void)
{
    if (sc_blocks > MAXB) { sc_bad++; printf("  %s: %ld blocks with a denormal\n", sc_worst_what, sc_blocks); }
    if (sc_blocks > sc_worst) sc_worst = sc_blocks;
    sc_blocks = 0; strncpy(sc_worst_what, what, 47);
    snd_init(&S, fx, pm); sm_jump(&S.master, 0.5f);
    for (int t = 0; t < GB_NTRACK; t++) { sm_jump(&S.trk[t].level, 1.0f); sm_jump(&S.trk[t].send_dly, 0.5f); sm_jump(&S.trk[t].send_rev, 0.5f); sm_jump(&S.trk[t].send_cho, 0.3f); }
    S.beat = GB_SR * 0.5f; S.grid = -1;
}
static void play(int trk, const int *notes, int n, float hold, float tail)
{
    for (int i = 0; i < n; i++) snd_note_on(&S, GB_OWNER(OWN_KEY, i), trk, notes[i], 0.9f);
    run(hold);
    for (int i = 0; i < n; i++) snd_note_off(&S, GB_OWNER(OWN_KEY, i), trk, notes[i]);
    run(tail);
}

int main(int argc, char **argv)
{
    float tail = argc > 1 ? (float)atof(argv[1]) : 25.0f;
    smp_bank_load("assets/samples.bin");
    trap_init();
    static char w[64];
    /* every factory sound: a chord (a kit: four of its sounds), held, let go, the tail and the effects after it */
    for (int i = 0; i < gb_npresets; i++) {
        snprintf(w, sizeof w, "sound %s", gb_presets[i].name); what = w;
        fresh();
        int drum = gb_presets[i].engine == ENG_DRUM, trk = drum ? 3 : 1;
        S.trk[trk].patch.engine = gb_presets[i].engine; preset_apply(&S, trk, i);
        int tr = drum ? 0 : preset_trans(i);
        const int syn[3] = { 48 + tr, 55 + tr, 64 + tr }, kit[4] = { 36, 38, 42, 46 }, kit2[4] = { 41, 45, 49, 51 };
        if (drum) { play(trk, kit, 4, 0.3f, 1.0f); play(trk, kit2, 4, 0.3f, tail); }
        else play(trk, syn, 3, 0.7f, tail);
    }
    /* every engine at its defaults, then with each of its eight values at its least and at its most, low, middle and
     * high: a long note (a square's flat halves through an open filter: the band's state falls to nothing in each),
     * then its end */
    for (int e = 0; e < ENG_N; e++) for (int var = 0; var < 17; var++) for (int oct = 0; oct < 3; oct++) {
        snprintf(w, sizeof w, "engine %d value %d %s note %d", e, (var - 1) / 2, var == 0 ? "default" : (var & 1) ? "least" : "most", 28 + 24 * oct); what = w;
        fresh();
        int trk = e == ENG_DRUM ? 3 : 1; gb_patch *p = &S.trk[trk].patch; gb_patch_default(p, e);
        if (var) { int k = (var - 1) / 2; p->p[k] = (var & 1) ? 0.0f : 1.0f; }
        const int n1[1] = { e == ENG_DRUM ? 36 + oct * 4 : 28 + 24 * oct };
        play(trk, n1, 1, 1.5f, 2.5f);
    }
    /* a pluck held: SUS at 0, a short decay, the key down for a long time */
    for (int e = 0; e < ENG_N; e++) {
        if (e == ENG_DRUM) continue;
        snprintf(w, sizeof w, "engine %d: a pluck held", e); what = w;
        fresh(); gb_patch *p = &S.trk[1].patch; gb_patch_default(p, e);
        gb_patch_set(p, SP_SUS, 0); gb_patch_set(p, SP_DEC, 20);
        const int n1[1] = { 57 }; play(1, n1, 1, 12.0f, 1.0f);
    }
    /* a key held long after its sound ended (a sample played out, a pluck gone), the track turned far down, its filter
     * low, its distortion on: what is left of the voice must be silence, not a trace kept up through them */
    for (int e = 0; e < ENG_N; e++) for (int lv = 0; lv < 2; lv++) {
        if (e == ENG_DRUM) continue;
        snprintf(w, sizeof w, "engine %d: held after its end, level %s", e, lv ? "low" : "full"); what = w;
        fresh(); gb_patch *p = &S.trk[1].patch; gb_patch_default(p, e);
        gb_patch_set(p, SP_SUS, 0); gb_patch_set(p, SP_DEC, 30); gb_patch_set(p, SP_DIST, 100);
        sm_jump(&S.trk[1].filt, -0.95f); sm_jump(&S.trk[1].level, lv ? 0.002f : 1.0f); S.flt_q = 6.0f;
        const int n1[1] = { 60 }; play(1, n1, 1, 10.0f, 1.0f);
    }
    /* every INSERT and SLICER setting on a plain sound, then silence */
    for (int it = 0; it <= gb_sp[SP_ITYPE].max; it++) for (int sl = 0; sl <= 1; sl++) {
        snprintf(w, sizeof w, "insert %d slicer %d", it, sl); what = w;
        fresh(); gb_patch *p = &S.trk[1].patch; gb_patch_default(p, 0);
        gb_patch_set(p, SP_ITYPE, it); gb_patch_set(p, SP_IA, 90); gb_patch_set(p, SP_IB, 100); gb_patch_set(p, SP_IC, 64); gb_patch_set(p, SP_IMIX, 127);
        gb_patch_set(p, SP_SLCR, sl); gb_patch_set(p, SP_SLDEPTH, 127); gb_patch_set(p, SP_DIST, 80);
        S.grid = 0; S.playing = 1;
        const int n3[3] = { 45, 57, 64 }; play(1, n3, 3, 0.6f, tail * 0.5f);
    }
    /* every punch-in over a sound that ends, the master's filter both ways, the dust, the duck, the texture */
    for (int k = 0; k < PK_N + 6; k++) {
        snprintf(w, sizeof w, k < PK_N ? "punch-in %d" : "master %d", k < PK_N ? k : k - PK_N); what = w;
        fresh(); gb_patch_default(&S.trk[1].patch, 0); S.grid = 0; S.playing = 1;
        if (k == PK_N) sm_set(&S.mfilt, -0.8f); if (k == PK_N + 1) sm_set(&S.mfilt, 0.8f);
        if (k == PK_N + 2) S.dust = 1.0f; if (k == PK_N + 3) S.duck = 1.0f;
        if (k == PK_N + 4) { S.tx[0] = 1; S.tx[1] = 100; S.tx[3] = 100; S.tx[4] = 60; }
        if (k == PK_N + 5) { S.lowcut = 1; S.flt_q = 8.0f; sm_set(&S.trk[1].filt, -0.9f); }
        const int n3[3] = { 40, 52, 59 };
        for (int i = 0; i < 3; i++) snd_note_on(&S, GB_OWNER(OWN_KEY, i), 1, n3[i], 0.9f);
        run(0.4f); if (k < PK_N) snd_punch(&S, k);
        run(1.2f); for (int i = 0; i < 3; i++) snd_note_off(&S, GB_OWNER(OWN_KEY, i), 1, n3[i]);
        run(2.0f); if (k < PK_N) snd_punch(&S, -1);
        run(tail * 0.5f);
    }
#if defined(__x86_64__) && defined(__linux__)
    for (int i = 0; i < nsite; i++) printf("  denormal at %#lx (%ld blocks; first in: %s)\n", site[i], hits[i], site_what[i]);
#endif
    what = "end"; fresh();
    printf("denormals: %d places trapped at least once; the worst scenario: %ld blocks; %d scenarios over %d blocks\n", nsite, sc_worst, sc_bad, MAXB);
    return sc_bad != 0;
}
