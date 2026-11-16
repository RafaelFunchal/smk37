/* SPDX-License-Identifier: GPL-3.0-only
 * The import of SLOOP 2.8's projects, on a flash written as SLOOP 2.8.5 writes it (its formats' facts). */
#include "../app/sloopimport.h"
#include "../app/presets.h"
#include "../app/store.h"
#include "../app/usersnd.h"
#include "../app/arp.h"
#include <stdio.h>
#include <string.h>
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static uint8_t fl[0x100000];
static int rd(uint32_t a, void *d, uint32_t n) { memcpy(d, fl + a, n); return 0; }
static void w16(uint8_t *b, int v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }
static void w32(uint8_t *b, uint32_t v) { w16(b, (int)(v & 0xFFFF)); w16(b + 2, (int)(v >> 16)); }
static void put_obj(int obj, int copy, uint32_t seq, uint8_t *pr)          /* as SLOOP's storage writes one */
{
    uint32_t s = 0x811C9DC5u; for (uint32_t i = 0; i < 3816u - 4u; i++) s = (s ^ pr[i]) * 16777619u;
    w32(pr + 3816u - 4u, s);
    uint32_t at = obj == 7 ? (copy ? 0xFE000u : 0x9F000u) : 0x97000u + (uint32_t)(obj - 1) * 8192u + (uint32_t)copy * 4096u;
    uint8_t *h = fl + at; memset(h, 0xFF, 4096);
    w32(h, 0x554C4546u); w16(h + 4, obj); w16(h + 6, copy); w32(h + 8, seq); w32(h + 12, 3816u); w32(h + 16, crc32_ieee(0, pr, 3816u));
    w32(h + 20, 0); w32(h + 24, 0); w32(h + 28, crc32_ieee(0, h, 28));
    memcpy(h + 256, pr, 3816u);
}
static uint8_t A[SLOOP_PROJ_LEN], B[SLOOP_PROJ_LEN], W[SLOOP_PROJ_LEN], got[4][SLOOP_PROJ_LEN], gw[SLOOP_PROJ_LEN];
static void base(uint8_t *p, int bpm)
{
    memset(p, 0, SLOOP_PROJ_LEN); w32(p, 0x46554E36u); w32(p + 4, 3816u); w16(p + 8, bpm); w16(p + 10, 30);
    for (int t = 0; t < 4; t++) {
        uint8_t *tk = p + 76 + t * 934;
        w16(tk + 0, 104); w16(tk + 2 * 29, 16); w16(tk + 2 * 30, 2); w16(tk + 2 * 32, 64); w16(tk + 2 * 39, 0);
        for (int s = 0; s < 64; s++) tk[134 + 10 * s + 5] = 2;    /* REST */
        tk[132] = 0; tk[133] = 0;
    }
}
static seq_t Q; static seq_hist H; static gb_sound S; static int16_t fx[GB_FX_MEM];
int main(void)
{
    printf("SLOOP 2.8 projects: read and imported\n");
    CHECK(crc32_ieee(0, "123456789", 9) == 0xCBF43926u, "the CRC is not zlib's");
    memset(fl, 0xFF, sizeof fl);
    base(A, 96); base(B, 96); base(W, 101);
    /* section A: track 1 a C3 on step 1 tied over 2 steps, an E3 on step 5 nudged +8/64; drums: kick on 1, hard snare on 5 */
    uint8_t *t1 = A + 76; uint8_t *s0 = t1 + 134;
    s0[0] = 48; s0[4] = 1; s0[5] = 0; s0[7] = 90; s0[10 + 5] = 1;            /* NOTE, then a TIE */
    s0[40] = 52; s0[44] = 1; s0[45] = 0; s0[47] = 100; s0[49] = 1;          /* step 5: ratchet x2 */
    t1[774 + 4] = 8 | (1 << 6);                                              /* nudge +8, FILL */
    uint8_t *d = A + 76 + 3 * 934 + 134;
    d[0] = 1; d[40] = 4; d[40 + 2] = 3 << 4;                                 /* kick step 1; snare (lane 2) step 5, HARD */
    w16(A + 76 + 2 * 30, 4);                                                 /* track 1 DIV 8T */
    A[76 + 132] = 1; A[76 + 133] = 0;                                        /* track 1: DIGITAL's first (keys) */
    base(B, 96); B[76 + 134 + 10 * 2] = 55; B[76 + 134 + 10 * 2 + 4] = 1; B[76 + 134 + 10 * 2 + 5] = 0;   /* section B: a G3 on step 3 */
    /* section A: a lock on track 1's step 5 (DIGITAL's E4, SLOOP's id 67); the working project: track 2's arp UP 1/16
     * OCT 2 GATE 64 HOLD, the kit (7), the kick's TUNE +3 and the snare's DECAY -2 (marked), DUST 64, TEXTURE RAIN */
    { uint8_t *l = A + 76 + 838; l[0] = 4; l[1] = 67; w16(l + 2, 20); l[4] = 6; l[5] = 39; w16(l + 6, -64); for (int i = 2; i < 24; i++) A[76 + 838 + 4 * i] = 0xFF; }
    for (int t = 0; t < 4; t++) for (int i = 0; i < 24; i++) { if (t || i) {} W[76 + t * 934 + 838 + 4 * i] = 0xFF; }
    for (int t = 1; t < 4; t++) for (int i = 0; i < 24; i++) A[76 + t * 934 + 838 + 4 * i] = 0xFF;
    { uint8_t *tk = W + 76 + 934; w16(tk + 2 * 17, 1); w16(tk + 2 * 18, 2); w16(tk + 2 * 19, 2); w16(tk + 2 * 20, 64); w16(tk + 2 * 23, 1); }
    { uint8_t *dk = W + 76 + 3 * 934; w16(dk + 106, 7);
      uint16_t wv[7] = { 0 }; uint32_t v0 = 3u, v2 = (uint32_t)((-2) & 7) << 4;
      for (uint32_t b = 0; b < 7; b++) { if ((v0 >> b) & 1u) wv[b / 16] |= (uint16_t)(1u << (b % 16)); if ((v2 >> b) & 1u) wv[(14 + b) / 16] |= (uint16_t)(1u << ((14 + b) % 16)); }
      uint32_t sum = 0x5Au; for (uint32_t i = 1; i < 8; i++) { w16(dk + 106 + 2 * i, wv[i - 1]); sum += (wv[i - 1] & 0xFFu) * (i * 2 + 1) + (wv[i - 1] >> 8) * (i * 2 + 2); }
      w16(W + 8 + 2 * 19, (int)(0x5400u | (sum & 0xFFu))); }
    w16(W + 8 + 2 * 27, 64); W[74] = 0x80 | 0x40 | 3; W[75] = 1; w16(W + 8 + 2 * 24, 100 | (64 + 10) << 7); w16(W + 8 + 2 * 15, 50);
    put_obj(1, 0, 3, A); put_obj(1, 1, 2, B);                                /* (copy A newer: section A) */
    put_obj(2, 1, 5, B);
    put_obj(7, 0, 9, W);
    fl[0x97000 + 8192 + 4096 + 300] ^= 1;                                    /* section B's copy damaged: none */
    CHECK(sloop_read(rd, 1, got[0]) && !memcmp(got[0] + 76 + 134, A + 76 + 134, 64), "section A not read");
    CHECK(!sloop_read(rd, 2, got[1]), "a damaged section read");
    CHECK(!sloop_read(rd, 3, got[2]), "an empty section read");
    CHECK(sloop_read(rd, 7, gw), "the working project not read");
    snd_init(&S, fx, 0); seq_init(&Q, 44100, 92, &H);
    for (int t = 0; t < 5; t++) S.trk[t].patch.engine = t == 3 ? ENG_DRUM : ENG_VA;
    const uint8_t *secs[4] = { got[0], 0, 0, 0 };
    project_extra X; memset(&X, 0, sizeof X); int po[5] = { -1, -1, -1, -1, -1 };
    CHECK(sloop_import(secs, gw, &Q, &S, &X, po), "nothing imported");
    const seq_pat *p = &Q.t[0].pat[0];
    printf("  tempo %.0f, track 1: %d notes, engine %s (%s)\n", Q.bpm, p->n, gb_engines[S.trk[0].patch.engine]->name, po[0] >= 0 ? gb_presets[po[0]].name : "-");
    CHECK(Q.bpm == 101.0f && Q.swing == 30, "the working project's tempo and swing");
    CHECK(p->n == 2 && p->ev[0].note == 48 && p->ev[0].tick == 0 && p->ev[0].len == 2 * SEQ_TPS && p->ev[0].vel == 90, "the tied C3");
    CHECK(p->ev[1].note == 52 && p->ev[1].tick == 4 * SEQ_TPS + 8 * SEQ_TPS / 64 && p->ev[1].ratchet == 2 && p->ev[1].cond == SEQ_FILL, "the nudged E3");
    const seq_pat *dp = &Q.t[3].pat[0];
    CHECK(dp->n == 2 && dp->ev[0].note == 0 && dp->ev[1].note == 2 && dp->ev[1].tick == 4 * SEQ_TPS && dp->ev[1].vel == 127, "the drums");
    CHECK(Q.t[0].div == SEQ_DIV_16 && Q.t[0].pat[0].steps == 16, "the working project's DIV and length");
    CHECK(po[1] >= 0 && gb_presets[po[1]].engine != ENG_DRUM, "a sound for track 2");
    {   /* the lock, the arpeggiator, the kit and its sounds, the master, TEXTURE */
        const seq_pat *lp = &Q.t[0].pat[0];
        gb_patch tmp; gb_patch_default(&tmp, ENG_FM); gb_patch_set(&tmp, SP_E4, 20);
        int want = (int)(tmp.p[SP_E4] * 255.0f + 0.5f);
        CHECK(lp->nlock == 2 && lp->lk[1].step == 6 && lp->lk[1].param == SEQ_LK_PAN && lp->lk[1].value == 0, "a PAN lock (%d)", lp->nlock);
        CHECK(lp->nlock >= 1 && lp->lk[0].step == 4 && lp->lk[0].param == SP_E4 && lp->lk[0].value == want, "the lock (%d, want %d)", lp->nlock ? lp->lk[0].value : -1, want);
        CHECK(X.arp[1][0] == AM_DN || X.arp[1][0] == sloop_amode[1], "arp mode %d", X.arp[1][0]);
        CHECK(X.arp[1][0] == sloop_amode[1] && X.arp[1][1] == sloop_arate[2] && X.arp[1][2] == 2 && X.arp[1][3] == 50 && X.arp[1][6] == 1, "the arpeggiator (%d %d %d %d %d)", X.arp[1][0], X.arp[1][1], X.arp[1][2], X.arp[1][3], X.arp[1][6]);
        CHECK(gb_ev(&S.trk[3].patch, 0) == 7 && S.trk[3].patch.dtune[0] == 3 && S.trk[3].patch.ddec[2] == -2 && S.trk[3].patch.dtune[1] == 0, "the kit (%d) and its sounds (%d %d)",
              gb_ev(&S.trk[3].patch, 0), S.trk[3].patch.dtune[0], S.trk[3].patch.ddec[2]);
        CHECK(S.dust > 0.49f && S.dust < 0.52f, "DUST %.2f", S.dust);
        CHECK(X.tex[0] == 3 && X.tex[1] == 100 && (int8_t)X.tex[2] == 10 && X.tex[3] == 50 && X.tex[4] == 1, "TEXTURE (%d %d %d %d %d)", X.tex[0], X.tex[1], (int8_t)X.tex[2], X.tex[3], X.tex[4]);
        /* its song: A 4 bars, A 2 (one entry of 6), C (not there: out), A again would join; B (damaged) out */
        static uint8_t pl[100]; memset(pl, 0, sizeof pl); w32(pl, 0x50455233u);
        uint8_t *ar = pl + 48; ar[0] = 4; ar[4] = 0; ar[5] = 4; ar[6] = 0; ar[7] = 2; ar[8] = 2; ar[9] = 8; ar[10] = 0; ar[11] = 3;
        int ns = sloop_song(pl, (int)sizeof pl, secs, &Q);
        CHECK(ns == 1 && Q.song_len == 9 && Q.song[0].pat[0] == 0 && Q.song[0].pat[4] == 0, "SLOOP's song (%d, %d bars)", ns, Q.song_len);
    }
    /* SLOOP's settings: its menu and its calibration (a unit whose PLAY and REC are swapped, KNOB 1 turned the other way) */
    {
        static uint8_t pl[1600], buf[3840]; memset(pl, 0, sizeof pl);
        w32(pl, 0x50455233u); w32(pl + 8, 1);                                   /* PER3, LOWCUT on */
        uint8_t *pn = pl + 16; w32(pn, 0x50414E35u);
        static const uint8_t B[14] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 12, 0, 1 }, E[7] = { 0, 1, 6, 2, 3, 4, 5 };
        for (int i = 0; i < 14; i++) pn[4 + i] = B[i];
        for (int i = 0; i < 7; i++) { pn[18 + i] = E[i]; pn[25 + i] = (uint8_t)(i == 3 ? -1 : 1); }
        w32(pl + 84, 2u | 3u << 4 | 1u << 8 | 1u << 26);                        /* LIGHTS MID, KEYS ALL, NOTES, 2ND CORE OFF */
        uint8_t *h = fl + 0xFD000; memset(h, 0xFF, 4096);
        w32(h, 0x554C4546u); w16(h + 4, 0); w16(h + 6, 1); w32(h + 8, 4); w32(h + 12, sizeof pl); w32(h + 16, crc32_ieee(0, pl, sizeof pl));
        w32(h + 20, 0); w32(h + 24, 0); w32(h + 28, crc32_ieee(0, h, 28)); memcpy(h + 256, pl, sizeof pl);
        sloop_set ss; memset(&ss, 0, sizeof ss);
        int ok = sloop_settings(rd, buf, &ss);
        CHECK(ok && ss.lowcut == 1 && ss.lights == 2 && ss.keys == 3 && ss.notes == 1 && ss.core2 == 0 && ss.accel == 1, "SLOOP's menu settings");
        CHECK(ss.cal && ss.btn[12] == 13 && ss.btn[13] == 12 && ss.btn[2] == 2 && ss.btn[0] == 0 && ss.enc[6] == 6 && ss.enc[2] == 2 && ss.dir[2] == -1 && ss.dir[0] == 1,
              "SLOOP's calibration (PLAY %d REC %d)", ss.btn[12], ss.btn[13]);
        /* SYN1..SYN4 after them: kit 2's closed hat at MIDI 70 + 3/16, its kick a square */
        uint8_t *u = pl + 88; w32(u, 0x31555344u); w16(u + 4, 1); w16(u + 6, 4);
        uint8_t *k2 = u + 8 + 2 * 364; k2[8] = 0x13;
        k2[12 + 3 * 22 + 2] = 70; k2[12 + 3 * 22 + 3] = 3; k2[12 + 3 * 22 + 21] = 128 + 8; k2[12 + 0 * 22 + 0] = 3;
        gb_dsnd syn[4][16]; uint8_t cr[4];
        int ks = sloop_synkits(pl, (int)sizeof pl, syn, cr);
        CHECK(ks && cr[2] == 0x13 && syn[2][4].note16 == 70 * 16 + 3 && syn[2][6].note16 == 69 * 16 + 3 && syn[2][4].qdb == 8 && syn[2][0].wave == 3,
              "SLOOP's SYN kits (%d %d)", syn[2][4].note16, syn[2][6].note16);
        /* the MIDI part after them: CC 20 track 2's LEVEL, CC 21 track 1's E4; FX CH 5, REMOTE 16, BEND 7, DR MAP, CLK, RESO 6 */
        uint8_t *mp = pl + 88 + 1464; memset(mp, 0, 40);
        w16(mp, 20 | 1 << 7 | 1 << 9); w16(mp + 2, 21 | 0 << 7 | (120 + 4) << 9);
        mp[32] = 1; mp[33] = 2; mp[34] = 3; mp[35] = 10 | 1 << 5; mp[36] = 7; mp[37] = 5; mp[38] = 1 | 2 | 6 << 2; mp[39] = 16;
        uint16_t ml[16]; sloop_mset ms;
        int nl = sloop_midi(pl, (int)sizeof pl, ml, &ms);
        CHECK(nl == 2 && ml[0] == (20 | 1 << 7 | 49 << 10) && ml[1] == (21 | (SP_E4 + 1) << 10) && ms.have && ms.fxch == 5 && ms.rmch == 16 && ms.bend == 7 && ms.dmap && ms.clk && ms.reso == 6 && ms.dprev,
              "SLOOP's MIDI LEARN and MIDI settings (%d)", nl);
        /* its favourites, in the copy's side record: 808 BOOM (its list's first), user preset 4, then an empty place */
        { uint8_t *sr = fl + 0xFD000 + 64; memset(sr, 0, 80); w32(sr + 40, 0x31564146u);
          for (int k = 0; k < 16; k++) w16(sr + 44 + 2 * k, 0xFFFF);
          w16(sr + 44, 0); w16(sr + 46, 0x8003); w32(sr + 76, crc32_ieee(0, sr + 40, 36));
          uint16_t fv[16]; static uint8_t scr2[3840]; int nf = sloop_favs(rd, scr2, fv);
          CHECK(nf == 2 && fv[0] == preset_of_sloop(0, 0) && fv[1] == 1003 && fv[2] == 0xFFFF, "SLOOP's favourites (%d: %d %d)", nf, fv[0], fv[1]);
          sr[50] ^= 1; CHECK(sloop_favs(rd, scr2, fv) == 0, "a damaged side record"); sr[50] ^= 1; }
    }
    /* SLOOP's user presets (bank 2, place 3: a DIGITAL sound kept by 2.8.5, its E2 and its delay send): your sounds */
    {
        static uint8_t pl[8 + 16 * 192], scr[3840], bank[US_FILE];
        memset(pl, 0, sizeof pl); memcpy(pl, "UPB1", 4); w16(pl + 4, 192); w16(pl + 6, 16);
        uint8_t *r = pl + 8 + 3 * 192; r[0] = 0xA5; r[1] = 1; r[2] = 1; r[3] = 71; memcpy(r + 4, "MY BELL", 7);
        w16(r + 16 + 2 * (63 + 2), 5); w16(r + 16 + 2 * 35, 50); w16(r + 16 + 2 * 1, 20);
        uint8_t *h = fl + 0xDE000 + 4096; memset(h, 0xFF, 4096);                 /* (object 6, copy B) */
        w32(h, 0x554C4546u); w16(h + 4, 6); w16(h + 6, 1); w32(h + 8, 2); w32(h + 12, sizeof pl); w32(h + 16, crc32_ieee(0, pl, sizeof pl));
        w32(h + 20, 0); w32(h + 24, 0); w32(h + 28, crc32_ieee(0, h, 28)); memcpy(h + 256, pl, sizeof pl);
        int n = us_from_sloop(rd, scr, bank);
        us_from_bytes(bank, US_LEN);
        us_sound u; int got = us_get(bank, 16 + 3, &u);
        CHECK(n == 1 && got && us_used(19) && !us_used(0) && !strcmp(us_name(19), "MY BELL") && u.engine == ENG_FM && u.fx[2] == 50, "SLOOP's user presets as your sounds (%d)", n);
        snd_init(&S, fx, 0); gb_patch_default(&S.trk[1].patch, ENG_VA); us_apply(&S, 1, &u);
        CHECK(S.trk[1].patch.engine == ENG_FM && gb_ev(&S.trk[1].patch, 2) == 5 && gb_pv(S.trk[1].patch.p, SP_ATK) == 20, "a sound of yours on a track");
        us_fav[0] = 7; us_fav[1] = US_BASE + 19; us_fav_put(bank);       /* the favourites, kept after the bank */
        us_fav[0] = us_fav[1] = 0xFFFF; us_from_bytes(bank, US_FILE);
        CHECK(us_fav[0] == 7 && us_fav[1] == US_BASE + 19 && us_fav[2] == 0xFFFF && us_used(19), "your favourites back");
        us_from_bytes(bank, US_LEN);
        CHECK(us_fav[0] == 0xFFFF && us_used(19), "a bank without favourites");
    }
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
