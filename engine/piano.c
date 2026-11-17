/* SPDX-License-Identifier: MPL-2.0
 * Erik37: Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * Adapted for SLOOP 3 from Erik37's app/dsp/erk.c: the voice of that piano engine as a SLOOP engine (float, the
 * voice layer's blocks; the voices allocated by SLOOP).
 *
 * PIANO. A voice: one or two sample streams (the bank's velocity layers, crossfaded during the attack, then the
 * dominant one alone), block ADPCM (erk_codec.h) decoded in place from the flash window and 4-point (cubic Hermite)
 * interpolated; a one-pole brightness filter (velocity, tone, una corda, darkening with time towards the key's
 * floor); the TONE's two shelves; a block-rate envelope (the key's two-slope decay with its knee, the two-stage
 * damper release; the top keys have no damper and ring on); constant-power pan by key across WIDTH; at the release
 * the bank's damper noise, mixed into the same voice. Its own envelopes (ownenv 1): render's 0 ends the voice.
 * Not here (Erik37's bus, or SLOOP's): sympathetic resonance, the reverb, the pad layer, the metronome, the voice
 * allocator, the half pedal (SLOOP's MIDI layer holds the note-offs while the sustain pedal is down).
 *
 * Erik37 runs 32-sample blocks at 44.1 kHz in fixed point; here 64-frame blocks at GB_SR: its per-block multipliers
 * (Q30) are raised to the power 2 x 44100 / GB_SR, its block counts kept in its 32-sample units (+2 a block).
 *
 *   E0 TONE      CONCERT STUDIO VINTAGE JAZZ: Erik37's voicings (erk_set_tone): the velocity filter's brightness,
 *                a low and a high shelf, VINTAGE and JAZZ the unisons a little apart (a detune by key)
 *   E1 BRILL     brilliance 0..100 (50 neutral): the filter x 2^(0.7 b) and the high shelf +-6 dB (b = -1..+1)
 *   E2 TOUCH     the velocity curve: SOFT NORMAL HARD FIXED (Erik37's touch curves; FIXED: every note at 100)
 *   E3 DECAY     the held decay's length x 0.25 .. 4 (64: as recorded): both slopes and the knee
 *   E4 DAMPER    the release's length x 0.25 .. 4 (64: as recorded): the damper's two stages
 *   E5 NOISE     the damper (release) noise 0..100 (40: Erik37's default; 100: its 1.1 level)
 *   E6 WIDTH     the keys spread across the stereo field 0..100 (80: Erik37's default), constant power
 *   E7 UNA CORDA the soft pedal held at this depth 0..100 (0: off): a softer hammer, quieter, darker */
#include "piano.h"
#include "erk_bank.h"
#include "erk_codec.h"
#include "sample.h"                 /* gb_flash_ok */

/* Erik37's constants, in its 32-sample blocks */
#define REL1 83                     /* damper first stage (~60 ms) */
#define DARK_FROM 345               /* the filter's darkening starts after ~0.25 s */
#define ENV_MIN 6.1035156e-05f      /* 2^-14, -84 dB: a held note ends */
#define ENV_MIN_LET 4.8828125e-04f  /* 2^-11, -66 dB: a let-go one */
#define G_MIN (24.0f / 32768.0f)    /* a let-go note below this (-63 dB of a full note) ends */
#define XF_AT 300                   /* two layers: from ~0.22 s the voice hands over to the dominant one ... */
#define XF_LEN 128                  /* ... over ~90 ms */
#define MAXINC (2u << 16)           /* at most two source samples an output frame (a block decodes <= 128) */
#define LEVEL (0.85f / 32768.0f)    /* the loudest note's peak about 0.5, as SAMPLE's */

static const gb_pd ED[8] = { { 0, 3, 0 }, { 0, 100, 50 }, { 0, 3, 1 }, { 0, 127, 64 }, { 0, 127, 64 }, { 0, 100, 40 },
                             { 0, 100, 80 }, { 0, 100, 0 } };
GB_INLINE int ev(const gb_patch *p, int k) { return gb_iv(p->p[k], ED[k].min, ED[k].max); }

/* ---- the bank */
static erk_bank bk;
static int bank_ok, bank_nrel, bank_nseg;                  /* (bank_nseg: the samples before the release noises) */
static uint8_t bank_gen;
static float bank_rg;                /* the damper noise's floor (the header's rel_gain), kept in RAM: release() reads no flash */
/* the loops' levels evened out (tools/mkpiano.py flat_table, after the data: "FLT1", u16 count, u16 0, then PIANO_NFLT
 * gains a sample, 64 = 1.0): a loop's level, measured round it, brought to its start's; none in an older image */
#define PIANO_NFLT 16
static const uint8_t *bank_flt;
GB_SMALL void piano_bank_set(const uint8_t *img)
{
    bank_ok = 0; bank_gen++;
    if (!img || erk_bank_open(&bk, img)) return;
    const erk_bank_hdr *h = bk.hdr;
    uint32_t ns = (uint32_t)h->nzones * h->nlayers;
    if (!h->nzones || !h->nlayers || h->nlayers > ERK_MAXL || h->nrel > ERK_MAXREL || ns > 128 || h->data_size > (4u << 20)) return;
    for (uint32_t i = 0; i < ns + h->nrel; i++) {
        const erk_smp *m = i < ns ? &bk.smp[i] : &bk.rel[i - ns];
        uint32_t lend = m->loop_len ? m->loop + m->loop_len : m->len;
        if (!m->len || !m->rate || m->loop % ERK_BLK || lend < m->loop + 1 || lend > (8u << 20) ||
            m->off > h->data_size || (lend + ERK_BLK - 1) / ERK_BLK * ERK_BLKB > h->data_size - m->off) return;
    }
    for (int k = 0; k < ERK_KEYS; k++) {
        const erk_key *kk = &bk.key[k];
        if (kk->zone >= h->nzones || (h->nrel && kk->rel >= h->nrel) || !kk->inc) return;
    }
    for (int v = 0; v < 128; v++) if (bk.vel[v].l0 >= h->nlayers || bk.vel[v].l1 >= h->nlayers) return;
    bank_nrel = h->nrel; bank_nseg = (int)ns; bank_rg = (float)h->rel_gain * (1.0f / 32768.0f);
    {   const uint8_t *t = bk.data + h->data_size;
        bank_flt = t[0] == 'F' && t[1] == 'L' && t[2] == 'T' && t[3] == '1' && (uint32_t)(t[4] | t[5] << 8) == ns + h->nrel ? t + 8 : 0;
    }
    bank_ok = 1;
}
int piano_bank_ok(void) { return bank_ok; }

/* ---- small maths (no libm): x^e for x > 0 near 1 as Erik37's multipliers are, from ln x = 2 atanh((x-1)/(x+1)) */
static float atanh2(float z) { float z2 = z * z; return 2.0f * z * (1.0f + z2 * (0.33333333f + z2 * (0.2f + z2 * (0.14285714f + z2 * 0.11111111f)))); }
static float qpow(uint32_t q, uint32_t one, float e)     /* (q / one)^e; the difference taken in integers */
{
    float z = (float)(int32_t)(q - one) / (float)(q + one);
    return gb_exp2f(e * atanh2(z) * 1.44269504f);
}
static float powf_(float x, float e)                      /* x^e, x > 0 */
{
    union { float f; uint32_t u; } b; b.f = x;
    int ex = (int)((b.u >> 23) & 255u) - 127; b.u = (b.u & 0x007FFFFFu) | 0x3F800000u;   /* x = m 2^ex, m 1..2 */
    float l2 = (float)ex + atanh2((b.f - 1.0f) / (b.f + 1.0f)) * 1.44269504f;
    return gb_exp2f(e * l2);
}
/* sin(pi/2 x), x 0..1 */
static float sinq(float x) { return gb_sin_turn(0.25f * gb_clampf(x, 0.0f, 1.0f)); }

/* ---- a sample stream: the decoder's state (offsets into the bank's data, no pointers), the interpolator's last
 * four samples, the position in Q16, its weight (the layer's share x the sample's gain) */
enum { SI_REL = 0x80, SI_NONE = 0xFF };
enum { SF_DONE = 1, SF_PRIMED = 2 };
typedef struct {
    uint32_t nxt, pos, frac;        /* the next ADPCM block's header (byte in the data); samples decoded; Q16 */
    float w, wt;                    /* the weight now, and at this block's end */
    float lg;                       /* the loop's level correction now (bank_flt; 1 before the loop) */
    int16_t s1, s2, x[4];
    uint8_t nib, hdr, si, fl;
} pst;
typedef struct {
    pst st[2], nz;                  /* the layers; the damper noise */
    float y, lo, hi;                /* the brightness filter, the shelves */
    float a, af, dark;              /* the filter's coefficient, its floor, the darkening a block */
    float env, m, m2, mr1, mr2;     /* the envelope and its multipliers a block */
    float amp, g, ng;               /* the velocity's level; the gain as last applied; the noise's level */
    float glo, ghi;                 /* the shelves' gains - 1 */
    float w0s, w1s, wend;           /* the layer handover: the start weights, the kept one's final weight */
    float inc;                      /* source samples an output frame at the key's pitch, Q16 */
    uint8_t wait_rel;               /* (PS_WAIT) let go before it could begin */
    uint32_t blocks;                /* 32-sample blocks since the start (Erik37's units) */
    uint16_t rblocks, knee;
    uint8_t key, state, flags, nst, keep, xfd, gen, rel;
} pvoice;
_Static_assert(sizeof(pvoice) <= sizeof(((gb_voice *)0)->s.raw), "PIANO's voice fits gb_voice.s");
enum { PS_DEAD, PS_ON, PS_REL, PS_RING, PS_NOISE,        /* NOISE: the note has ended, its damper noise goes on */
       PS_WAIT };                                       /* WAIT: started while a save had the flash: begins once it is back */
#define PV(v) ((pvoice *)(void *)(v)->s.raw)

static const erk_smp *smp_of(uint8_t si) { return si & SI_REL ? &bk.rel[si & 0x7Fu] : &bk.smp[si]; }
static void pst_init(pst *s, uint8_t si, float w)
{
    s->si = si; s->fl = 0; s->w = s->wt = w; s->frac = 0;
    s->lg = 1.0f;
}
/* n samples of a stream into out (Erik37's stream_decode, in runs: up to the ADPCM block's end or the loop's end;
 * past a one-shot's end, zeros, nothing read) */
static __attribute__((noinline)) void pst_decode(pst *s, float *out, int n)
{
    const erk_smp *m = smp_of(s->si);
    const uint8_t *d = bk.data;
    uint32_t nxt = s->nxt, pos = s->pos, lend = m->loop_len ? m->loop + m->loop_len : m->len;
    int32_t nib = s->nib, h = s->hdr, sh = h & 15, c1 = erk_filt[h >> 4][0], c2 = erk_filt[h >> 4][1], s1 = s->s1, s2 = s->s2;
    while (n > 0) {
        if ((s->fl & SF_DONE) || pos >= lend) {
            s->fl |= SF_DONE; s1 = s2 = 0;
            while (n > 0) { *out++ = 0.0f; n--; }
            break;
        }
        if (nib >= ERK_BLK) {                       /* the next block's header */
            h = d[nxt]; sh = h & 15; c1 = erk_filt[h >> 4][0]; c2 = erk_filt[h >> 4][1];
            nxt += ERK_BLKB; nib = 0;
        }
        int32_t k = ERK_BLK - nib;
        k = k < n ? k : n;
        k = (uint32_t)k < lend - pos ? k : (int32_t)(lend - pos);
        n -= k; pos += (uint32_t)k;
        const uint8_t *bp = d + nxt - (ERK_BLKB - 1) + (nib >> 1);
        for (int32_t e = nib + k; nib < e; nib++) {
            int32_t q = (nib & 1) ? *bp++ >> 4 : *bp & 15;
            int32_t v = ((c1 * s1 + c2 * s2 + 32) >> 6) + ((q ^ 8) - 8) * (1 << sh);
            v = v < -32768 ? -32768 : v > 32767 ? 32767 : v;
            s2 = s1; s1 = v;
            *out++ = (float)v;
        }
        if (pos == lend) {                          /* the loop's end (or the sample's) */
            if (m->loop_len) { nxt = m->off + (m->loop / ERK_BLK) * ERK_BLKB; s1 = m->h1; s2 = m->h2; pos = m->loop; }
            else s->fl |= SF_DONE;
            nib = ERK_BLK;
        }
    }
    s->nxt = nxt; s->pos = pos; s->nib = (uint8_t)nib; s->hdr = (uint8_t)h; s->s1 = (int16_t)s1; s->s2 = (int16_t)s2;
}
/* (the flash readable) its first samples: x = 0, s0, s1, s2 (Erik37's stream_start) */
static void pst_prime(pst *s)
{
    float a[3];
    s->nxt = smp_of(s->si)->off; s->pos = 0; s->nib = ERK_BLK; s->hdr = 0; s->s1 = s->s2 = 0;
    pst_decode(s, a, 3);
    s->x[0] = 0; s->x[1] = (int16_t)a[0]; s->x[2] = (int16_t)a[1]; s->x[3] = (int16_t)a[2];
    s->fl |= SF_PRIMED;
}
/* a block of a stream, added into buf, weighted (ramped to wt): decoded first, then interpolated */
static __attribute__((noinline)) void pst_run(pst *s, uint32_t inc, float *buf)
{
    float a[4 + 2 * GB_BLOCK];
    uint32_t f = s->frac;
    int n = (int)((f + inc * GB_BLOCK) >> 16);
    a[0] = s->x[0]; a[1] = s->x[1]; a[2] = s->x[2]; a[3] = s->x[3];
    pst_decode(s, a + 4, n);
    float lg1 = 1.0f;                               /* the loop's level correction where this block ends */
    const erk_smp *m = smp_of(s->si);
    if (bank_flt && m->loop_len && s->pos > m->loop) {
        uint32_t ph = (s->pos - m->loop) * (PIANO_NFLT * 256u) / m->loop_len, i = (ph >> 8) % PIANO_NFLT, j = (i + 1u) % PIANO_NFLT;
        const uint8_t *g = bank_flt + (uint32_t)(s->si & SI_REL ? bank_nseg + (s->si & 0x7Fu) : s->si) * PIANO_NFLT;
        lg1 = ((float)g[i] + (float)((int)g[j] - (int)g[i]) * (float)(ph & 255u) * (1.0f / 256.0f)) * (1.0f / 64.0f);
    }
    float lg = s->lg, dlg = (lg1 - lg) * (1.0f / GB_BLOCK);
    float w = s->w, dw = (s->wt - s->w) * (1.0f / GB_BLOCK);
    for (int i = 0; i < GB_BLOCK; i++) {
        f += inc;
        const float *x = a + (f >> 16);
        float t = (float)(f & 0xFFFFu) * (1.0f / 65536.0f);
        float c1 = 0.5f * (x[2] - x[0]), c2 = x[0] - 2.5f * x[1] + 2.0f * x[2] - 0.5f * x[3];
        float c3 = 0.5f * (x[3] - x[0]) + 1.5f * (x[1] - x[2]);
        w += dw; lg += dlg;
        buf[i] += ((((c3 * t) + c2) * t + c1) * t + x[1]) * w * lg;
    }
    s->lg = lg1;
    s->x[0] = (int16_t)a[n]; s->x[1] = (int16_t)a[n + 1]; s->x[2] = (int16_t)a[n + 2]; s->x[3] = (int16_t)a[n + 3];
    s->frac = f & 0xFFFFu; s->w = s->wt;
}

/* ---- the voice */
static const struct { float bright, lo_db, hi_db; int16_t detune; } TONE[4] = {
    { 1.00f, 0.0f, 0.0f, 0 },       /* CONCERT: the recording as it is */
    { 1.15f, -1.0f, 2.5f, 0 },      /* STUDIO: closer, brighter, tighter */
    { 0.80f, 1.5f, -3.5f, 22 },     /* VINTAGE: darker, warmer, the unisons a little apart */
    { 0.88f, 2.5f, -1.5f, 6 },      /* JAZZ: round, warm low mids, softer top */
};
static const float CURVE_EXP[3] = { 0.55f, 0.95f, 1.7f };

static void pn_release(gb_voice *v, const gb_patch *p);
/* the note itself, the flash readable (its tables and samples are there) */
GB_SMALL static void pn_begin(gb_voice *v, const gb_patch *p)
{
    pvoice *q = PV(v);
    memset(q, 0, sizeof *q);
    q->state = PS_DEAD;
    if (!bank_ok) return;
    int key = gb_clampi((int)v->note, ERK_KEY0, ERK_KEY0 + ERK_KEYS - 1) - ERK_KEY0;
    const erk_key *k = &bk.key[key];
    int tone = ev(p, 0), curve = ev(p, 2);
    float una = (float)ev(p, 7) * 0.01f, b = ((float)ev(p, 1) * 0.01f - 0.5f) * 2.0f;
    float r = 2.0f * 44100.0f / GB_SR;              /* Erik37's blocks in one of ours */
    float dsc = gb_exp2f((float)(ev(p, 3) - 64) * (1.0f / 32.0f)), rsc = gb_exp2f((float)(ev(p, 4) - 64) * (1.0f / 32.0f));
    /* the hammer: the velocity through the touch curve, then the una corda */
    float t = gb_clampf(v->vel, 1.0f / 127.0f, 1.0f);
    int vel = curve == 3 ? 100 : gb_clampi((int)(1.0f + 126.0f * powf_(t, CURVE_EXP[curve]) + 0.5f), 1, 127);
    if (una > 0.0f) vel = gb_clampi(1 + (int)((float)vel * (1.0f - una * 0.2f)), 1, 127);
    const erk_vel *e = &bk.vel[vel];
    int nl = bk.hdr->nlayers;
    q->key = (uint8_t)key; q->flags = k->flags; q->gen = bank_gen; q->rel = k->rel;
    float inc = (float)k->inc * (44100.0f / GB_SR);
    if (TONE[tone].detune) {                        /* VINTAGE, JAZZ: the unisons a little apart, by key */
        int32_t c = (int32_t)(((uint32_t)key * 2654435761u) >> 24) - 128;
        inc *= 1.0f + (float)(c * TONE[tone].detune) * (1.0f / 2097152.0f);
    }
    q->inc = inc;
    /* the layers: one stream, or two crossfaded through the attack */
    const erk_smp *s0 = &bk.smp[k->zone * nl + e->l0], *s1 = &bk.smp[k->zone * nl + e->l1];
    uint8_t i0 = (uint8_t)(k->zone * nl + e->l0), i1 = (uint8_t)(k->zone * nl + e->l1);
    int w = e->w;
    if (e->l0 == e->l1 || w < 64) w = 0;                       /* (a layer's share below -54 dB: that layer alone) */
    else if (w > 32767 - 64) { s0 = s1; i0 = i1; w = 0; }
    if (w) {
        q->nst = 2;
        pst_init(&q->st[0], i0, gb_minf((float)(32767 - w) * (float)s0->gain * (1.0f / (32768.0f * 4096.0f)), 1.0f));
        pst_init(&q->st[1], i1, gb_minf((float)w * (float)s1->gain * (1.0f / (32768.0f * 4096.0f)), 1.0f));
        q->keep = w >= 16384;
        q->wend = gb_minf((float)(q->keep ? s1->gain : s0->gain) * (1.0f / 4096.0f), 1.0f);
    } else {
        q->nst = 1;
        pst_init(&q->st[0], i0, gb_minf((float)s0->gain * (1.0f / 4096.0f), 1.0f));
    }
    q->w0s = q->st[0].w; q->w1s = q->st[1].w;
    q->nz.si = SI_NONE;
    /* the level, the envelope */
    float amp = (float)e->amp * (1.0f / 32768.0f);
    if (una > 0.0f) amp *= 1.0f - una * 0.2747f;
    q->amp = amp;
    q->env = 1.0f;
    q->m = qpow(k->m1, 1u << 30, r / dsc);
    q->m2 = qpow(k->m2, 1u << 30, r / dsc);
    q->mr1 = qpow(k->mr1, 1u << 30, r / rsc);
    q->mr2 = qpow(k->mr2, 1u << 30, r / rsc);
    q->knee = (uint16_t)gb_clampi((int)((float)k->knee * dsc + 0.5f), 1, 65535);
    /* the brightness filter: by velocity, the tone, the una corda; never below the key's floor */
    float bs = TONE[tone].bright * gb_exp2f(0.7f * b);
    float a = gb_clampf((float)e->a * bs, 200.0f, 16384.0f);
    if (una > 0.0f) a *= 1.0f - una * 0.3662f;
    a = gb_maxf(a, (float)k->a_floor);
    q->a = a * (1.0f / 16384.0f);
    q->af = gb_minf((float)k->a_floor, a) * (1.0f / 16384.0f);
    q->dark = qpow(k->dark, 65536u, 2.0f);
    /* the tone's shelves (Erik37 has them on its bus: the same per voice) */
    q->glo = TONE[tone].lo_db != 0.0f ? gb_db(TONE[tone].lo_db) - 1.0f : 0.0f;
    float hdb = TONE[tone].hi_db + 6.0f * b;
    q->ghi = gb_absf(hdb) > 0.01f ? gb_db(hdb) - 1.0f : 0.0f;
    /* the pan: by key across WIDTH (constant power), on SLOOP's (the voice layer's centre is 1/sqrt 2) */
    float pp = 0.5f + 0.4f * (float)ev(p, 6) * 0.01f * (float)(key * 2 - 87) * (1.0f / 87.0f);
    v->pan_l *= 1.41421356f * sinq(1.0f - pp);
    v->pan_r *= 1.41421356f * sinq(pp);
    q->g = q->env * q->amp;                         /* start at the target: keep the hammer's transient */
    q->state = PS_ON;
}
/* a note asked for while a save has the flash window off (a chord played as the autosave
 * began lost its piano notes): kept waiting, begun as soon as the flash is back (fw/main.c ends an autosave at a key);
 * let go meanwhile, it begins and is let go at once */
GB_SMALL static void pn_start(gb_voice *v, const gb_patch *p, int fresh)
{
    (void)fresh;
    pvoice *q = PV(v);
    if (bank_ok && !gb_flash_ok) { memset(q, 0, sizeof *q); q->state = PS_WAIT; q->gen = bank_gen; return; }
    pn_begin(v, p);
}

GB_SMALL static void pn_release(gb_voice *v, const gb_patch *p)
{
    if (PV(v)->state == PS_WAIT) { PV(v)->wait_rel = 1; return; }   /* (it begins once the flash is back, then is let go) */
    pvoice *q = PV(v);
    if (q->state != PS_ON) return;
    if (q->flags & ERK_NODAMP) { q->state = PS_RING; return; }   /* no damper: rings on at its decay */
    q->state = PS_REL; q->m = q->mr1; q->rblocks = 0;
    if (!bank_ok || q->gen != bank_gen || !bank_nrel) return;
    /* the damper's noise: louder for loud, still-ringing notes. Nothing read from the flash here (a save may have it
     * off): its sample starts at the next block that can read */
    float g = q->env * q->amp, rg = bank_rg;
    float lvl = (float)ev(p, 5) * (9000.0f / 100.0f / 32768.0f) * (rg + (1.0f - rg) * gb_sqrtf(g)) * gb_sqrtf(q->amp);
    if (lvl * 32768.0f < 64.0f) return;
    pst_init(&q->nz, (uint8_t)(SI_REL | (q->rel < bank_nrel ? q->rel : 0)), 0.0f);
    q->ng = lvl;
}

GB_HOT static int pn_render(gb_voice *v, const gb_patch *p, const gb_mod *md, float *out)
{
    pvoice *q = PV(v);
    for (int i = 0; i < GB_BLOCK; i++) out[i] = 0.0f;
    if (q->state == PS_DEAD || !bank_ok || q->gen != bank_gen) return 0;
    if (!gb_flash_ok) return 1;                     /* (a save: nothing read; silent, the voice waits as it is) */
    if (q->state == PS_WAIT) {                       /* (the flash back: the note waited for begins now) */
        int let_go = q->wait_rel;
        pn_begin(v, p);
        if (q->state == PS_DEAD) return 0;
        if (let_go) pn_release(v, p);
    }
    if (q->state != PS_NOISE) {
        float buf[GB_BLOCK];
        /* the envelope at the block's end */
        float env = q->env * q->m;
        q->blocks += 2;
        if (q->blocks >= q->knee && q->blocks - 2 < q->knee && (q->state == PS_ON || q->state == PS_RING)) q->m = q->m2;
        if (q->state == PS_REL && (q->rblocks = (uint16_t)(q->rblocks + 2)) >= REL1 && q->rblocks - 2 < REL1) q->m = q->mr2;
        if (q->blocks > DARK_FROM) q->a = q->af + (q->a - q->af) * q->dark;
        q->env = env;
        if (q->nst == 2 && q->blocks >= XF_AT) {     /* the layer handover */
            if (q->xfd) {
                if (q->keep) q->st[0] = q->st[1];
                q->st[0].w = q->st[0].wt = q->wend;
                q->nst = 1;
            } else {
                float f = (float)(q->blocks - XF_AT + 1) * (1.0f / XF_LEN);
                if (f >= 1.0f) { f = 1.0f; q->xfd = 1; }
                float e0 = q->keep ? 0.0f : q->wend, e1 = q->keep ? q->wend : 0.0f;
                q->st[0].wt = q->w0s + (e0 - q->w0s) * f;
                q->st[1].wt = q->w1s + (e1 - q->w1s) * f;
            }
        }
        /* the pitch: the key's rate, moved by the voice's pitch (bend, glide, LFO) from the key */
        float kf = (float)(q->key + ERK_KEY0);
        uint32_t inc = (uint32_t)gb_minf(q->inc * gb_exp2f(gb_clampf(md->pitch - kf, -48.0f, 24.0f) * (1.0f / 12.0f)), (float)MAXINC);
        for (int i = 0; i < GB_BLOCK; i++) buf[i] = 0.0f;
        for (int s = 0; s < q->nst; s++) {
            if (!(q->st[s].fl & SF_PRIMED)) pst_prime(&q->st[s]);
            pst_run(&q->st[s], inc, buf);
        }
        float g1 = env * q->amp, g = q->g * LEVEL, dg = (g1 - q->g) * (LEVEL / GB_BLOCK);
        float y = q->y, a = q->a, lo = q->lo, hi = q->hi, glo = q->glo, ghi = q->ghi;
        if (glo != 0.0f || ghi != 0.0f) {
            for (int i = 0; i < GB_BLOCK; i++) {
                y += a * (buf[i] - y);
                float x = y;
                lo += (x - lo) * 0.035f; x += lo * glo;
                hi += (x - hi) * 0.348f; x += (x - hi) * ghi;
                g += dg; out[i] = x * g;
            }
        } else {
            GB_UNROLL4
            for (int i = 0; i < GB_BLOCK; i++) { y += a * (buf[i] - y); g += dg; out[i] = y * g; }
        }
        q->y = gb_flush(y); q->lo = gb_flush(lo); q->hi = gb_flush(hi); q->g = g1;
        if (env < (q->state == PS_ON ? ENV_MIN : ENV_MIN_LET) || (q->state != PS_ON && g1 < G_MIN) || (q->st[0].fl & SF_DONE))
            q->state = PS_NOISE;                    /* the note is over (its noise may go on) */
    }
    if (q->nz.si != SI_NONE) {                       /* the damper noise: as recorded, at its level, its own rate */
        pst *s = &q->nz;
        if (!(s->fl & SF_PRIMED)) { pst_prime(s); s->w = s->wt = gb_minf((float)smp_of(s->si)->gain * (1.0f / 4096.0f), 1.0f); }
        float nb[GB_BLOCK];
        for (int i = 0; i < GB_BLOCK; i++) nb[i] = 0.0f;
        uint32_t inc = (uint32_t)gb_minf((float)smp_of(s->si)->rate * (65536.0f / GB_SR), (float)MAXINC);
        pst_run(s, inc, nb);
        float ng = q->ng * LEVEL;
        GB_UNROLL4
        for (int i = 0; i < GB_BLOCK; i++) out[i] += nb[i] * ng;
        if (s->fl & SF_DONE) s->si = SI_NONE;
    }
    if (q->state == PS_NOISE && q->nz.si == SI_NONE) { q->state = PS_DEAD; return 0; }
    return 1;
}

const gb_engine gb_eng_piano = { "PIANO", ED, 16, 1, pn_start, pn_render, pn_release, 0, 0 };
