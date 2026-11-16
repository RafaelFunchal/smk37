/* SPDX-License-Identifier: GPL-3.0-only
 * The send effects: a stereo tempo delay and an eight-line feedback-delay-network reverb. Lines in 16 bits
 * (RAM is the scarce resource); a tail that has fallen to its last bits is rounded towards zero, so a loop can
 * never hold a tiny pattern for ever (OpenPocket's lesson). Written for this project. */
#include "engine.h"

static const uint16_t REV_N[8] = { 1117, 1289, 1453, 1607, 1777, 1949, 2111, 2297 };   /* mutually prime, ~25..52 ms */
_Static_assert(GB_REV_LOW == 1117 + 1289 + 1453 + 1607 + 1777 && GB_REV_SAMPLES == GB_REV_LOW + 1949 + 2111 + 2297, "the reverb's lines");

GB_INLINE int16_t to16(float x)
{
    float y = gb_clampf(x, -1.0f, 1.0f) * 32767.0f;
    int32_t i = (int32_t)y;                                  /* towards zero: tails die out (see above) */
    return (int16_t)i;
}
GB_INLINE float from16(int16_t v) { return (float)v * (1.0f / 32767.0f); }

GB_SMALL void fx_init(gb_sound *s, int16_t *mem, int16_t *rev_low)
{
    s->dly = mem; s->dly_len = GB_DLY_FRAMES; s->dly_pos = 0;
    memset(s->dly, 0, sizeof(int16_t) * GB_DLY_FRAMES * 2);
    sm_init(&s->dly_time, GB_SR * 0.375f, 120.0f);           /* a dotted eighth at 120 BPM */
    s->dly_fb = 0.45f; s->dly_damp = 0.3f; s->dly_mix = 1.0f;
    /* the lines: the first five in the block below (low RAM on the FM-1), the last three after the delay */
    int16_t *hi = mem + GB_DLY_FRAMES * 2, *lo = rev_low ? rev_low : hi + (GB_REV_SAMPLES - GB_REV_LOW);
    for (int i = 0; i < 8; i++) {
        if (i < 5) { s->rev_line[i] = lo; lo += REV_N[i]; } else { s->rev_line[i] = hi; hi += REV_N[i]; }
        s->rev_n[i] = REV_N[i]; s->rev_pos[i] = 0; s->rev_lp[i] = 0.0f;
        memset(s->rev_line[i], 0, sizeof(int16_t) * REV_N[i]);
    }
    s->rev_fb = 0.82f; s->rev_damp = 0.35f;
}

/* The delay. Its length is a power of two, so the read index wraps with a mask (no test per sample); the time is
 * kept at GB_BLOCK + 2 frames or more, so nothing this block writes is read in this block. */
_Static_assert((GB_DLY_FRAMES & (GB_DLY_FRAMES - 1)) == 0, "the delay's length must be a power of two");
#define DLY_MIN ((float)(GB_BLOCK + 2))
GB_HOT __attribute__((noinline)) static void delay_block(gb_sound *s, const float *dl, const float *dr, float *ol, float *or_)
{
    /* the time glides (no click when it changes, G3); clamped at the block's two ends, so not per sample */
    const float tmax = (float)(GB_DLY_FRAMES - 2);
    float st = sm_step(&s->dly_time);
    float t0 = gb_clampf(s->dly_time.cur, DLY_MIN, tmax), t1 = gb_clampf(s->dly_time.cur + st * GB_BLOCK, DLY_MIN, tmax);
    float tm = t0, dt = (t1 - t0) * (1.0f / GB_BLOCK);
    if (s->pk.lend == 2) { sm_end(&s->dly_time, st); return; }   /* its memory lent to a punch-in */
    int lend = s->pk.lend == 1;                                  /* lent from the next block: this one fades out */
    float fb = gb_clampf(s->dly_fb, 0.0f, 0.95f), dmp1 = 1.0f - gb_clampf(s->dly_damp, 0.0f, 0.95f);
    fb = gb_maxf(fb, 0.7f * s->throw_dly.cur);                    /* the echo throw: 70 % at least */
    if (s->pk.fx == PK_ECHO) fb = gb_maxf(fb, 0.55f * s->pk.mix);
    int16_t *d = s->dly;
    const uint32_t mask = GB_DLY_FRAMES - 1;
    uint32_t pos = s->dly_pos;
    /* in three passes, each with few live values (16 registers): nothing this block writes is read in it */
    float yl[GB_BLOCK], yr[GB_BLOCK];
    GB_UNROLL2
    for (int n = 0; n < GB_BLOCK; n++) {                        /* 1. the taps, interpolated */
        tm += dt;
        uint32_t di = (uint32_t)tm; float fr = tm - (float)di;
        uint32_t r0 = (pos + (uint32_t)n - di) & mask, r1 = (r0 - 1u) & mask;
        float a = from16(d[2 * r0]), b = from16(d[2 * r0 + 1]);
        yl[n] = a + (from16(d[2 * r1]) - a) * fr;
        yr[n] = b + (from16(d[2 * r1 + 1]) - b) * fr;
    }
    if (lend) {
        for (int n = 0; n < GB_BLOCK; n++) { float g = 1.0f - (float)(n + 1) * (1.0f / GB_BLOCK); ol[n] += yl[n] * g; or_[n] += yr[n] * g; }
        sm_end(&s->dly_time, st); s->pk.lend = 2;
        return;
    }
    float lp0 = s->dly_lp[0], lp1 = s->dly_lp[1], mx = gb_clampf(s->dly_mix, 0.0f, 1.5f);
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) {                        /* 2. the echo out (its level: MIX); the feedback softened */
        ol[n] += yl[n] * mx; or_[n] += yr[n] * mx;
        lp0 += (yl[n] + GB_BIAS - lp0) * dmp1; lp1 += (yr[n] + GB_BIAS - lp1) * dmp1;
        yl[n] = lp0; yr[n] = lp1;
    }
    GB_UNROLL2
    for (int n = 0; n < GB_BLOCK; n++) {                        /* 3. in, crossed: left's echo feeds the right */
        d[2 * pos] = to16(dl[n] + yr[n] * fb);
        d[2 * pos + 1] = to16(dr[n] + yl[n] * fb);
        pos = (pos + 1u) & mask;
    }
    sm_end(&s->dly_time, st);
    s->dly_lp[0] = lp0; s->dly_lp[1] = lp1; s->dly_pos = pos;
}

/* The reverb: eight lines, a Householder matrix (energy-preserving), damping in each line. Every line is longer
 * than a block, so the block is done line by line: read the 64 outputs of each line (each in its own short loop,
 * its filter in a register), mix, then write the 64 inputs back to the same places. The same numbers as sample by
 * sample, without eight indexes to test and eight filters to keep in memory at every sample. */
static float rev_y[8][GB_BLOCK] GB_LOWRAM;                  /* (written before read, each block) */
GB_HOT __attribute__((noinline)) static void rev_read(const int16_t *line, uint32_t pos, uint32_t len, float *lpp, float k, float *y)
{
    float lp = *lpp;
    int n = 0;
    while (n < GB_BLOCK) {                                     /* one or two runs: the wrap is between them */
        int run = (int)(len - pos) < GB_BLOCK - n ? (int)(len - pos) : GB_BLOCK - n;
        const int16_t *src = line + pos;
        GB_UNROLL4
        for (int j = 0; j < run; j++) { lp += (from16(src[j]) - lp) * k; y[n + j] = lp; }
        n += run; pos += (uint32_t)run; if (pos >= len) pos = 0;
    }
    *lpp = gb_flush(lp);                                       /* (no bias here: the lines' filters are heard, and what is
                                                                  heard reaches the master's filter: a trace of it kept that one busy) */
}
GB_HOT __attribute__((noinline)) static uint32_t rev_write(int16_t *line, uint32_t pos, uint32_t len, const float *y, const float *sum, const float *in, float g)
{
    int n = 0;
    while (n < GB_BLOCK) {
        int run = (int)(len - pos) < GB_BLOCK - n ? (int)(len - pos) : GB_BLOCK - n;
        int16_t *dst = line + pos;
        GB_UNROLL4
        for (int j = 0; j < run; j++) dst[j] = to16((y[n + j] - sum[n + j]) * g + in[n + j]);
        n += run; pos += (uint32_t)run; if (pos >= len) pos = 0;
    }
    return pos;
}
GB_HOT __attribute__((noinline)) static void reverb_block(gb_sound *s, const float *rl, const float *rr, float *ol, float *or_)
{
    float g = gb_clampf(s->rev_fb, 0.0f, 0.97f), k = 1.0f - gb_clampf(s->rev_damp, 0.0f, 0.95f);
    if (s->freeze) { g = 0.995f; k = 0.85f; }                   /* FREEZE: held (a little damping: it never rings) */
    float sum[GB_BLOCK], inp[GB_BLOCK], inm[GB_BLOCK];
    for (int i = 0; i < 8; i++) rev_read(s->rev_line[i], s->rev_pos[i], s->rev_n[i], &s->rev_lp[i], k, rev_y[i]);
    float ev[GB_BLOCK], od[GB_BLOCK];                         /* the even and the odd lines' sums */
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) ev[n] = rev_y[0][n] + rev_y[2][n] + rev_y[4][n] + rev_y[6][n];
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) od[n] = rev_y[1][n] + rev_y[3][n] + rev_y[5][n] + rev_y[7][n];
    GB_UNROLL4
    for (int n = 0; n < GB_BLOCK; n++) {
        sum[n] = (ev[n] + od[n]) * 0.25f;                      /* 2/N */
        float in = (rl[n] + rr[n]) * 0.35f;
        inp[n] = in; inm[n] = -in;
        ol[n] += ev[n] * 0.5f; or_[n] += od[n] * 0.5f;
    }
    for (int i = 0; i < 8; i++)
        s->rev_pos[i] = rev_write(s->rev_line[i], s->rev_pos[i], s->rev_n[i], rev_y[i], sum, i & 1 ? inm : inp, g);
}

GB_HOT void fx_render(gb_sound *s, const float *dl, const float *dr, const float *rl, const float *rr, float *ol, float *or_)
{
    delay_block(s, dl, dr, ol, or_);
    reverb_block(s, rl, rr, ol, or_);
}
