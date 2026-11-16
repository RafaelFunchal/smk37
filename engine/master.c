/* SPDX-License-Identifier: GPL-3.0-only
 * The master bus, after the effects' returns: DUST, then the punch-in effects, then the DJ filter (the master gain
 * and the limiter follow, in sound.c). And the one-knob filter each track has. Written for this project.
 *   DUST    an old sampler and a record: drive into a soft clip, a lower rate (down to ~11 kHz), fewer bits (down to
 *           8), a low-pass closing to ~3 kHz, and, while the transport plays, a little hiss and crackle
 *   PUNCH   27 effects, one at a time, on the whole mix while a key is held, faded in and out (pk_fade). Loops,
 *           the gate and the reverse start on the grid (a 1/16 of the transport); the loops, the reverse, the tape
 *           stop and the half speed borrow the delay's memory while they run (the echo starts again empty after);
 *           REWIND plays the mix's last third of a second backwards (a line of its own, mono, half rate);
 *           FLANGER, WOBBLE and OCT UP read a short line of their own.
 *   FILTER  left of centre a low-pass closing, right a high-pass opening, centre off; it glides (G3) and fades in
 *           over the first 5 % of the knob, so the centre is exactly the dry sound. */
#include "engine.h"

#define CAP GB_DLY_FRAMES                     /* stereo frames the borrowed memory holds */
GB_INLINE int16_t s16(float x) { x = gb_clampf(x, -1.0f, 1.0f) * 32767.0f; return (int16_t)(int32_t)x; }
GB_INLINE float f16(int16_t v) { return (float)v * (1.0f / 32767.0f); }
GB_INLINE float tanpi(float x) { x = gb_minf(x, 0.47f); float c = gb_sin_turn(0.5f * x + 0.25f); return gb_sin_turn(0.5f * x) / (c > 1e-4f ? c : 1e-4f); }

/* a state-variable filter (zero-delay feedback): the coefficients once a block */
typedef struct { float a1, a2, a3, k; } svfc;
static svfc svf_c(float fc, float k)
{
    svfc c; float g = tanpi(fc * GB_ISR);
    c.k = k; c.a1 = 1.0f / (1.0f + g * (g + k)); c.a2 = g * c.a1; c.a3 = g * c.a2;
    return c;
}
GB_INLINE float svf_lp(const svfc *c, float *ic, float in)
{
    float v3 = in - ic[1], v1 = c->a1 * ic[0] + c->a2 * v3, v2 = ic[1] + c->a2 * ic[0] + c->a3 * v3;
    ic[0] = 2.0f * v1 - ic[0]; ic[1] = 2.0f * v2 - ic[1];
    return v2;
}
GB_INLINE float svf_hp(const svfc *c, float *ic, float in)
{
    float v3 = in - ic[1], v1 = c->a1 * ic[0] + c->a2 * v3, v2 = ic[1] + c->a2 * ic[0] + c->a3 * v3;
    ic[0] = 2.0f * v1 - ic[0]; ic[1] = 2.0f * v2 - ic[1];
    return in - c->k * v1 - v2;
}

/* ---------------------------------------------------------------- the one-knob filter (master and tracks) */
__attribute__((noinline)) void gb_svf_dj(float *l, float *r, float x, float *st, float q)
{
    float a = gb_absf(x);
    if (a < 1e-4f) { st[0] = st[1] = st[2] = st[3] = 0.0f; return; }
    float mix = gb_minf(a * 20.0f, 1.0f);
    float fc = x < 0.0f ? 20000.0f * gb_exp2f(9.5f * x) : 20.0f * gb_exp2f(9.5f * x);   /* 28 Hz .. off .. 14 kHz */
    svfc c = svf_c(fc, q);
    if (x < 0.0f) {
        for (int n = 0; n < GB_BLOCK; n++) { float y = svf_lp(&c, st, l[n]); l[n] += (y - l[n]) * mix; }
        for (int n = 0; n < GB_BLOCK; n++) { float y = svf_lp(&c, st + 2, r[n]); r[n] += (y - r[n]) * mix; }
    } else {
        for (int n = 0; n < GB_BLOCK; n++) { float y = svf_hp(&c, st, l[n]); l[n] += (y - l[n]) * mix; }
        for (int n = 0; n < GB_BLOCK; n++) { float y = svf_hp(&c, st + 2, r[n]); r[n] += (y - r[n]) * mix; }
    }
    for (int i = 0; i < 4; i++) st[i] = gb_flush(st[i]);
}

void master_init(gb_sound *s, int16_t *mem)
{
    sm_init(&s->mfilt, 0.0f, 60.0f);
    sm_init(&s->throw_dly, 0.0f, 12.0f); sm_init(&s->throw_rev, 0.0f, 12.0f);
    s->beat = GB_SR * 0.5f; s->grid = -1.0f; s->duck_pos = 1e9f;
    s->du_seed = 0x2545F491u;
    s->pk.fx = s->pk.want = -1;
    s->hist = mem; s->modl = mem ? mem + GB_HIST : 0; s->cho = mem ? mem + GB_HIST + 2 * GB_MODL : 0;
    s->cho_depth = 0.5f; s->cho_rate = 0.6f; s->flt_q = 0.75f;
    s->tx[0] = 0; s->tx[1] = 64; s->tx[2] = 0; s->tx[3] = 40; s->tx[4] = 2; s->tx[5] = 0; s->tx[6] = 0;   /* (TEXTURE as SLOOP starts it) */
    if (mem) memset(mem, 0, sizeof(int16_t) * GB_PUNCH_MEM);
}
/* 3.3.2: after a value gone wrong (snd_mix): nothing kept that could carry it on. The settings stay; the delay,
 * the reverb, the chorus, the punch-in's lines and every filter's state start again from silence */
/* 3.3.3: a smoothed value gone wrong put back (its target if that is sound, else safe), and a value that is not a
 * number put at safe: snd_panic's own fix of what it does not clear, so the next block is not wrong again */
GB_INLINE int bad_f(float x) { return !(x > -1e9f && x < 1e9f); }
static void fix_sm(gb_smooth *m, float safe) { if (bad_f(m->tgt)) m->tgt = safe; if (bad_f(m->cur)) m->cur = m->tgt; if (bad_f(m->k) || m->k <= 0.0f) m->k = 0.01f; }
static void fix_f(float *x, float safe) { if (bad_f(*x)) *x = safe; }
GB_SMALL void snd_panic(gb_sound *s)
{
    s->panics++;
    snd_all_off(s);
    fix_sm(&s->master, 0.2f); fix_sm(&s->dly_time, 11025.0f); fix_sm(&s->mfilt, 0.0f); fix_sm(&s->throw_dly, 0.0f); fix_sm(&s->throw_rev, 0.0f);
    fix_f(&s->dly_fb, 0.4f); fix_f(&s->dly_damp, 0.3f); fix_f(&s->rev_fb, 0.82f); fix_f(&s->rev_damp, 0.35f); fix_f(&s->dly_mix, 1.0f);
    fix_f(&s->cho_ph, 0.0f); fix_f(&s->cho_depth, 0.5f); fix_f(&s->cho_rate, 0.6f); fix_f(&s->du_ph, 0.0f); fix_f(&s->dust, 0.0f); fix_f(&s->duck, 0.0f);
    fix_f(&s->duck_pos, 1e8f); fix_f(&s->beat, GB_SR * 0.5f); fix_f(&s->flt_q, 0.75f); fix_f(&s->pk.saved_time, 11025.0f);
    if (bad_f(s->grid)) s->grid = -1.0f;
    for (int t = 0; t < GB_NTRACK; t++) {
        gb_track *k = &s->trk[t];
        fix_sm(&k->level, 0.7f); fix_sm(&k->pan, 0.5f); fix_sm(&k->send_dly, 0.0f); fix_sm(&k->send_rev, 0.0f); fix_sm(&k->send_cho, 0.0f); fix_sm(&k->filt, 0.0f);
        fix_f(&k->bend, 0.0f); fix_f(&k->lfo, 0.0f); fix_f(&k->lfo_rnd, 0.0f); fix_f(&k->lfo_fade, 0.0f); fix_f(&k->last_pitch, 60.0f); fix_f(&k->meter, 0.0f);
    }
    /* the lines hold 16 bits: never a NaN, only a burst. Cleared once; a panic again soon after (a value that comes
     * back wrong every block) does not clear 200 KB a block, which left the loop no time (the FM-1 seemed frozen) */
    if (s->panic_quiet) goto states;
    s->panic_quiet = 256;
    if (s->dly) memset(s->dly, 0, sizeof(int16_t) * GB_DLY_FRAMES * 2);
    for (int i = 0; i < 8; i++) if (s->rev_line[i]) memset(s->rev_line[i], 0, sizeof(int16_t) * s->rev_n[i]);
    if (s->hist) memset(s->hist, 0, sizeof(int16_t) * GB_HIST);
    if (s->modl) memset(s->modl, 0, sizeof(int16_t) * 2 * GB_MODL);
    if (s->cho) memset(s->cho, 0, sizeof(int16_t) * GB_CHO);
states:
    memset(s->rev_lp, 0, sizeof s->rev_lp);
    s->dly_lp[0] = s->dly_lp[1] = 0.0f; s->lim_gain = 1.0f; s->lim_env = 0.0f;
    memset(s->lc, 0, sizeof s->lc); memset(s->mfilt_st, 0, sizeof s->mfilt_st);
    s->du_hold[0] = s->du_hold[1] = s->du_lp[0] = s->du_lp[1] = 0.0f; s->du_click = 0.0f;
    for (int t = 0; t < GB_NTRACK; t++) { memset(s->trk[t].filt_st, 0, sizeof s->trk[t].filt_st); memset(s->trk[t].dist_st, 0, sizeof s->trk[t].dist_st); }
    s->pk.fx = -1; s->pk.mix = 0.0f; s->pk.lend = 0; s->freeze = 0; s->hist_stop = 0;
    tfx_reset();
}

void snd_punch(gb_sound *s, int fx) { s->pk.want = (int8_t)(fx >= 0 && fx < PK_N ? fx : -1); }

/* ---------------------------------------------------------------- the chorus: two taps swaying ~0.6 Hz around
 * 13 ms, left and right a quarter turn apart (a send: what goes in comes out wet only) */
void chorus_block(gb_sound *s, const float *in, float *ml, float *mr)
{
    if (!s->cho) return;
    uint32_t p0 = s->cho_pos;
    for (int n = 0; n < GB_BLOCK; n++) s->cho[(p0 + (uint32_t)n) & (GB_CHO - 1u)] = s16(in[n]);
    float ph = s->cho_ph, dep = 40.0f + 200.0f * gb_clampf(s->cho_depth, 0.0f, 1.0f);
    float d0l = 573.0f + dep * gb_sin_turn(ph), d0r = 573.0f + dep * gb_sin_turn(ph + 0.25f);
    ph += gb_clampf(s->cho_rate, 0.05f, 10.0f) * (float)GB_BLOCK * GB_ISR; ph -= ph >= 1.0f ? 1.0f : 0.0f;
    float d1l = 573.0f + dep * gb_sin_turn(ph), d1r = 573.0f + dep * gb_sin_turn(ph + 0.25f);
    for (int n = 0; n < GB_BLOCK; n++) {
        float u = (float)n * (1.0f / GB_BLOCK);
        for (int c = 0; c < 2; c++) {
            float d = c ? d0r + (d1r - d0r) * u : d0l + (d1l - d0l) * u;
            float r = (float)(p0 + (uint32_t)n) - d;
            int32_t i = (int32_t)r; if ((float)i > r) i--;
            float f = r - (float)i; uint32_t a = (uint32_t)i & (GB_CHO - 1u), z = (a + 1u) & (GB_CHO - 1u);
            float x0 = f16(s->cho[a]), y = (x0 + (f16(s->cho[z]) - x0) * f) * 0.8f;
            if (c) mr[n] += y; else ml[n] += y;
        }
    }
    s->cho_ph = ph; s->cho_pos = (p0 + GB_BLOCK) & (GB_CHO - 1u);
}

/* ---------------------------------------------------------------- DUST */
static void dust_block(gb_sound *s, float *ml, float *mr)
{
    float d = s->dust;
    if (d < 0.004f) return;
    float drive = 1.0f + 3.0f * d, out_g = (1.0f + d) / drive;
    float inc = 1.0f - 0.75f * d;                                       /* the rate: 1 .. 1/4 */
    float lv = gb_exp2f(15.0f - 7.0f * d), ilv = 1.0f / lv;            /* 2^15 .. 2^8 levels */
    float fc = 16000.0f * gb_exp2f(-2.4f * d), a = 1.0f - gb_exp2f(-GB_TWO_PI * fc * GB_ISR * 1.442695f);
    float hiss = s->playing ? 0.004f * d : 0.0f;
    uint32_t crackle = s->playing ? (uint32_t)(d * 5.0f * GB_ISR * 4294967296.0f) : 0u;   /* ~5 a second at full */
    float ph = s->du_ph, h0 = s->du_hold[0], h1 = s->du_hold[1], lp0 = s->du_lp[0], lp1 = s->du_lp[1], ck = s->du_click;
    uint32_t sd = s->du_seed;
    for (int n = 0; n < GB_BLOCK; n++) {
        ph += inc;
        if (ph >= 1.0f) {                                               /* a new sample, held until the next */
            ph -= 1.0f;
            /* (3.1.8: what is below its coarsest step is nothing, before the curve squares it: the last of a reverb,
             * squared, is below what a float holds) */
            h0 = gb_absf(ml[n]) < 1e-6f ? 0.0f : gb_sat(ml[n] * drive) * out_g; h1 = gb_absf(mr[n]) < 1e-6f ? 0.0f : gb_sat(mr[n] * drive) * out_g;
            h0 = (float)(int32_t)(h0 * lv + (h0 >= 0.0f ? 0.5f : -0.5f)) * ilv;
            h1 = (float)(int32_t)(h1 * lv + (h1 >= 0.0f ? 0.5f : -0.5f)) * ilv;
        }
        float nz = 0.0f;
        if (hiss > 0.0f) {
            sd ^= sd << 13; sd ^= sd >> 17; sd ^= sd << 5;
            nz = (float)(int32_t)sd * (hiss * (1.0f / 2147483648.0f));
            if ((sd >> 7) < (crackle >> 7)) ck = ((sd & 0x80u) ? 1.0f : -1.0f) * (0.05f + 0.2f * (float)(sd & 0x7Fu) * (1.0f / 127.0f)) * d;
        }
        /* (3.1.8: its filter settles at GB_BIAS, taken off again on the way out: quick as it is, it fell through the
         * denormals after every sound, and the bias left in would be a trace on the master bus) */
        lp0 += (h0 + nz + ck + GB_BIAS - lp0) * a; lp1 += (h1 + nz + ck + GB_BIAS - lp1) * a;
        ck *= 0.55f;
        ml[n] = lp0 - GB_BIAS; mr[n] = lp1 - GB_BIAS;
    }
    s->du_ph = ph; s->du_hold[0] = h0; s->du_hold[1] = h1; s->du_seed = sd; s->du_click = gb_flush(ck);
    s->du_lp[0] = lp0; s->du_lp[1] = lp1;
}

/* ---------------------------------------------------------------- the punch-ins */
static float beat_of(const gb_sound *s) { return s->beat > 2000.0f ? s->beat : GB_SR * 0.5f; }
static int capture_fx(int fx)
{
    return fx == PK_LOOP4 || fx == PK_LOOP8 || fx == PK_LOOP16 || fx == PK_LOOP32 || fx == PK_STUTTER || fx == PK_LOOP8T ||
           fx == PK_REVERSE || fx == PK_TAPESTOP || fx == PK_HALF;
}
static uint32_t to_grid(const gb_sound *s)                  /* frames to the next 1/16 of the transport */
{
    if (s->grid < 0.0f) return 0;
    float step = beat_of(s) * 0.25f, ph = s->grid - step * (float)(int)(s->grid / step);
    return ph < 1.0f ? 0u : (uint32_t)(step - ph);
}
static void pk_begin(gb_sound *s, int fx)
{
    gb_punch *p = &s->pk;
    float b = beat_of(s);
    memset(p->st, 0, sizeof p->st);
    p->fx = (int8_t)fx; p->stage = 0; p->t = 0; p->w = 0; p->k = 0; p->r = 0.0f; p->g = 1.0f; p->ph = 0.0f;
    p->hold[0] = p->hold[1] = 0.0f; p->mix = 0.0f; p->wait = 0; p->len = 0;
    static const float FRAC[PK_N] = { [PK_LOOP4] = 1.0f, [PK_LOOP8] = 0.5f, [PK_LOOP16] = 0.25f, [PK_LOOP32] = 0.125f,
                                      [PK_STUTTER] = 1.0f / 6.0f, [PK_LOOP8T] = 1.0f / 3.0f, [PK_REVERSE] = 0.5f };
    if (capture_fx(fx)) {
        p->lend = s->dly ? 1 : 0;
        if (FRAC[fx] > 0.0f) {
            float len = b * FRAC[fx];
            uint32_t cap = fx == PK_REVERSE ? CAP / 2u - 64u : CAP - 64u;
            while (len > (float)cap) len *= 0.5f;                        /* a slow tempo: half as long */
            p->len = (uint32_t)len;
            p->wait = to_grid(s);
        }
    }
    if (fx == PK_ECHO) { p->saved_time = s->dly_time.tgt; sm_set(&s->dly_time, gb_minf(b * 0.75f, (float)(GB_DLY_FRAMES - 2))); }
    if (fx == PK_REWIND) { s->hist_stop = 1; p->r = (float)s->hist_pos - 1.0f - 1102.0f; }
    if (fx == PK_FREEZE) s->freeze = 1;
}
static void pk_end(gb_sound *s)
{
    gb_punch *p = &s->pk;
    if (p->lend) {                                              /* the delay's memory given back, empty */
        if (p->lend == 2) memset(s->dly, 0, sizeof(int16_t) * GB_DLY_FRAMES * 2);
        s->dly_lp[0] = s->dly_lp[1] = 0.0f; p->lend = 0;
    }
    if (p->fx == PK_ECHO) sm_set(&s->dly_time, p->saved_time);
    if (p->fx == PK_REWIND) s->hist_stop = 0;
    if (p->fx == PK_FREEZE) s->freeze = 0;
    p->fx = -1;
}

void master_pre(gb_sound *s, const float *ml, const float *mr, float *dl, float *dr, float *rl, float *rr, float *dry_l, float *dry_r)
{
    gb_punch *p = &s->pk;
    if (p->fx == PK_ECHO) {                                     /* the whole mix into the delay */
        float g = p->mix * 0.8f;
        for (int n = 0; n < GB_BLOCK; n++) { dl[n] += ml[n] * g; dr[n] += mr[n] * g; }
    }
    if (p->fx == PK_FREEZE) {                                   /* poured in for 0.15 s, then held */
        for (int n = 0; n < GB_BLOCK; n++) {
            float pour = p->t + (uint32_t)n < 6615u ? 1.4f : 0.0f;
            rl[n] = ml[n] * pour; rr[n] = mr[n] * pour;
            dry_l[n] = ml[n]; dry_r[n] = mr[n];
        }
    }
}

/* the loops: the capture, then the loop, an edge of 32 frames faded at each turn */
GB_INLINE float edge(uint32_t j, uint32_t len) { uint32_t e = j < len - 1u - j ? j : len - 1u - j; return e >= 32u ? 1.0f : (float)e * (1.0f / 32.0f); }
static int fx_loop(gb_sound *s, float *wl, float *wr)
{
    gb_punch *p = &s->pk; int16_t *b = s->dly;
    for (int n = 0; n < GB_BLOCK; n++) {
        if (p->stage == 0) { if (p->wait) { p->wait--; continue; } if (p->lend != 2) continue; p->stage = 1; p->w = 0; }
        if (p->stage == 1) {
            b[2 * p->w] = s16(wl[n]); b[2 * p->w + 1] = s16(wr[n]);
            if (++p->w >= p->len) { p->stage = 2; p->k = 0; }
            continue;
        }
        uint32_t j = p->k; float g = edge(j, p->len);
        wl[n] = f16(b[2 * j]) * g; wr[n] = f16(b[2 * j + 1]) * g;
        if (++p->k >= p->len) p->k = 0;
    }
    return p->stage == 2;
}
/* the reverse: each 1/8 heard backwards during the next */
static int fx_reverse(gb_sound *s, float *wl, float *wr)
{
    gb_punch *p = &s->pk; int16_t *b = s->dly; uint32_t C = p->len;
    for (int n = 0; n < GB_BLOCK; n++) {
        if (p->stage == 0) { if (p->wait) { p->wait--; continue; } if (p->lend != 2) continue; p->stage = 1; p->w = 0; p->k = 0; }
        uint32_t wi = (p->k & 1u) * C + p->w;
        float il = wl[n], ir = wr[n];
        if (p->k > 0) {
            uint32_t ri = ((p->k - 1u) & 1u) * C + (C - 1u - p->w); float g = edge(p->w, C);
            wl[n] = f16(b[2 * ri]) * g; wr[n] = f16(b[2 * ri + 1]) * g;
        }
        b[2 * wi] = s16(il); b[2 * wi + 1] = s16(ir);
        if (++p->w >= C) { p->w = 0; p->k++; if (p->k > 1u) p->stage = 2; }
    }
    return p->stage == 2 || p->k > 0;
}
/* the tape stop and the half speed: the mix written on, read slower */
GB_INLINE float rd_ring(const int16_t *b, float r, int c)
{
    int32_t i = (int32_t)r; if ((float)i > r) i--;
    float f = r - (float)i; uint32_t a = (uint32_t)i & (CAP - 1u), z = (a + 1u) & (CAP - 1u);
    float x0 = f16(b[2 * a + c]); return x0 + (f16(b[2 * z + c]) - x0) * f;
}
static int fx_slow(gb_sound *s, float *wl, float *wr, int half)
{
    gb_punch *p = &s->pk; int16_t *b = s->dly;
    float T = gb_minf(beat_of(s), 26000.0f), B = gb_minf(beat_of(s), 16000.0f);
    for (int n = 0; n < GB_BLOCK; n++) {
        if (p->stage == 0) { if (p->lend != 2) continue; p->stage = 1; p->w = 0; p->k = 0; p->r = 0.0f; }
        uint32_t wi = p->w & (CAP - 1u);
        b[2 * wi] = s16(wl[n]); b[2 * wi + 1] = s16(wr[n]); p->w++;
        if (!half) {                                            /* slower and slower, then still */
            float rate = 1.0f - (float)p->k / T; rate = rate > 0.0f ? rate : 0.0f;
            float g = gb_minf(1.0f, rate * 3.0f);
            p->r += rate; if (p->r > (float)(p->w - 1u)) p->r = (float)(p->w - 1u);
            wl[n] = rd_ring(b, p->r, 0) * g; wr[n] = rd_ring(b, p->r, 1) * g;
        } else {                                                /* each beat stretched over two */
            uint32_t q = p->k % (uint32_t)(2.0f * B);
            if (q == 0) p->r = (float)(p->w - 1u);
            else p->r += 0.5f;
            float g = gb_minf(1.0f, gb_minf((float)q, 2.0f * B - (float)q) * (1.0f / 64.0f));
            wl[n] = rd_ring(b, p->r, 0) * g; wr[n] = rd_ring(b, p->r, 1) * g;
        }
        p->k++;
    }
    return p->stage == 1;
}
/* the rewind: the record slows, stops, and spins backwards faster and faster through the last third of a second */
static void fx_rewind(gb_sound *s, float *wl, float *wr)
{
    gb_punch *p = &s->pk;
    if (!s->hist) return;
    const float TA = 4410.0f, TB = 11025.0f, end = (float)s->hist_pos - 1.0f - 7600.0f;
    for (int n = 0; n < GB_BLOCK; n++) {
        float t = (float)(p->t + (uint32_t)n), rate;
        if (t < TA) rate = 0.5f * (1.0f - t / TA);
        else { float u = gb_minf((t - TA) / TB, 1.0f); rate = -2.0f * u * u; }
        p->r += rate;
        float y = 0.0f;
        if (p->r > end) {
            int32_t i = (int32_t)p->r; if ((float)i > p->r) i--;
            float f = p->r - (float)i; uint32_t a = (uint32_t)i & (GB_HIST - 1u), z = (a + 1u) & (GB_HIST - 1u);
            float x0 = f16(s->hist[a]);
            y = (x0 + (f16(s->hist[z]) - x0) * f) * gb_minf(1.0f, gb_absf(rate) * 6.0f + (t < TA * 0.5f ? 1.0f : 0.0f));
        }
        wl[n] = wr[n] = y;
    }
}
/* the short line: a tap d frames before frame n of this block (written already, see master_post) */
GB_INLINE float tap(const gb_sound *s, uint32_t p0, int n, float d, int c)
{
    float r = (float)(p0 + (uint32_t)n) - d;
    int32_t i = (int32_t)r; if ((float)i > r) i--;
    float f = r - (float)i; uint32_t a = (uint32_t)i & (GB_MODL - 1u), z = (a + 1u) & (GB_MODL - 1u);
    float x0 = f16(s->modl[2 * a + c]); return x0 + (f16(s->modl[2 * z + c]) - x0) * f;
}
static void fx_modl(gb_sound *s, float *wl, float *wr, uint32_t p0)
{
    gb_punch *p = &s->pk;
    if (!s->modl) return;
    float b = beat_of(s);
    for (int n = 0; n < GB_BLOCK; n++) {
        float t = (float)(p->t + (uint32_t)n);
        if (p->fx == PK_OCTUP) {                                /* two taps sliding, a triangle each: an octave up */
            const float W = 880.0f;                       /* (< GB_MODL - GB_BLOCK: never this block's frames) */
            float d1 = W - (t - W * (float)(int)(t / W)), d2 = d1 + W * 0.5f; if (d2 > W) d2 -= W;
            float g1 = 1.0f - gb_absf(2.0f * d1 / W - 1.0f), g2 = 1.0f - g1;
            wl[n] = tap(s, p0, n, d1 + 1.0f, 0) * g1 + tap(s, p0, n, d2 + 1.0f, 0) * g2;
            wr[n] = tap(s, p0, n, d1 + 1.0f, 1) * g1 + tap(s, p0, n, d2 + 1.0f, 1) * g2;
        } else if (p->fx == PK_WOBBLE) {                         /* the tape's speed wavering */
            float tt = t * GB_ISR, d = 130.0f + 60.0f * gb_sin_turn(0.9f * tt) + 22.0f * gb_sin_turn(5.1f * tt);
            wl[n] = tap(s, p0, n, d, 0); wr[n] = tap(s, p0, n, d + 3.0f, 1);
        } else {                                                /* FLANGER: over a bar, left and right apart */
            float gp = s->grid >= 0.0f ? s->grid + (float)n : t, ph = gp / (4.0f * b);
            float dl = 44.0f + 220.0f * (0.5f + 0.5f * gb_sin_turn(ph)), dr = 44.0f + 220.0f * (0.5f + 0.5f * gb_sin_turn(ph + 0.25f));
            float yl = tap(s, p0, n, dl, 0), yr = tap(s, p0, n, dr, 1);
            uint32_t i = (p0 + (uint32_t)n) & (GB_MODL - 1u);
            s->modl[2 * i] = s16(wl[n] + 0.55f * yl); s->modl[2 * i + 1] = s16(wr[n] + 0.55f * yr);
            wl[n] = 0.6f * (wl[n] + yl); wr[n] = 0.6f * (wr[n] + yr);
        }
    }
}
static void fx_simple(gb_sound *s, float *wl, float *wr)
{
    gb_punch *p = &s->pk; float *st = p->st, b = beat_of(s);
    float t0 = (float)p->t, bar = gb_minf((t0 + GB_BLOCK) / (4.0f * b), 1.0f);
    switch (p->fx) {
    case PK_LPSWEEP: gb_svf_dj(wl, wr, -(0.1f + 0.75f * bar), st, 1.3f); break;
    case PK_HPSWEEP: gb_svf_dj(wl, wr, 0.1f + 0.62f * bar, st, 1.3f); break;
    case PK_KILLLOW: case PK_KILLHIGH: case PK_KILLMID: {
        svfc lo = svf_c(250.0f, 1.4142f), hi = svf_c(2500.0f, 1.4142f);
        for (int n = 0; n < GB_BLOCK; n++) {
            float x[2] = { wl[n], wr[n] }, y[2];
            for (int c = 0; c < 2; c++) {
                float *q = st + 8 * c;
                if (p->fx == PK_KILLLOW) y[c] = svf_hp(&lo, q + 2, svf_hp(&lo, q, x[c]));
                else if (p->fx == PK_KILLHIGH) y[c] = svf_lp(&hi, q + 2, svf_lp(&hi, q, x[c]));
                else y[c] = svf_lp(&lo, q + 2, svf_lp(&lo, q, x[c])) + svf_hp(&hi, q + 6, svf_hp(&hi, q + 4, x[c]));
            }
            wl[n] = y[0]; wr[n] = y[1];
        }
        break; }
    case PK_PHONE: {
        svfc h = svf_c(500.0f, 1.2f), l = svf_c(2800.0f, 1.0f);
        for (int n = 0; n < GB_BLOCK; n++) {
            float m = (wl[n] + wr[n]) * 0.5f;
            m = svf_lp(&l, st + 2, svf_hp(&h, st, m));
            wl[n] = wr[n] = gb_sat(m * 3.0f) * 0.55f;
        }
        break; }
    case PK_CRUSH:
        for (int n = 0; n < GB_BLOCK; n++) {
            wl[n] = (float)(int32_t)(wl[n] * 16.0f + (wl[n] >= 0.0f ? 0.5f : -0.5f)) * (1.0f / 16.0f);
            wr[n] = (float)(int32_t)(wr[n] * 16.0f + (wr[n] >= 0.0f ? 0.5f : -0.5f)) * (1.0f / 16.0f);
        }
        break;
    case PK_ALIAS:                                              /* a sample kept for six: ~7 kHz, unfiltered */
        for (int n = 0; n < GB_BLOCK; n++) {
            if ((p->t + (uint32_t)n) % 6u == 0u) { p->hold[0] = wl[n]; p->hold[1] = wr[n]; }
            wl[n] = p->hold[0]; wr[n] = p->hold[1];
        }
        break;
    case PK_GATE: {                                             /* 1/16: open half of each step */
        float step = b * 0.25f, g = p->g;
        for (int n = 0; n < GB_BLOCK; n++) {
            float gp = s->grid >= 0.0f ? s->grid + (float)n : t0 + (float)n, ph = gp - step * (float)(int)(gp / step);
            g += ((ph < step * 0.55f ? 1.0f : 0.0f) - g) * 0.06f;
            wl[n] *= g; wr[n] *= g;
        }
        p->g = gb_flush(g); break; }
    case PK_AUTOPAN:
        for (int n = 0; n < GB_BLOCK; n++) {
            float gp = s->grid >= 0.0f ? s->grid + (float)n : t0 + (float)n, pn = 0.5f + 0.5f * gb_sin_turn(gp / b);
            float m = (wl[n] + wr[n]) * 0.7071f;
            wl[n] = m * gb_sin_turn(0.25f + pn * 0.25f); wr[n] = m * gb_sin_turn(pn * 0.25f);
        }
        break;
    case PK_MONO: for (int n = 0; n < GB_BLOCK; n++) wl[n] = wr[n] = (wl[n] + wr[n]) * 0.5f; break;
    case PK_FADEOUT:
        for (int n = 0; n < GB_BLOCK; n++) { float g = 1.0f - (t0 + (float)n) / (2.0f * b); g = g > 0.0f ? g * g : 0.0f; wl[n] *= g; wr[n] *= g; }
        break;
    default: break;
    }
}

static void punch_block(gb_sound *s, float *ml, float *mr, const float *dry_l, const float *dry_r, uint32_t modl_p0)
{
    gb_punch *p = &s->pk;
    if (p->fx < 0) { if (p->want < 0) return; pk_begin(s, p->want); }
    float wl[GB_BLOCK], wr[GB_BLOCK];
    for (int n = 0; n < GB_BLOCK; n++) { wl[n] = ml[n]; wr[n] = mr[n]; }
    int ready = 1;
    switch (p->fx) {
    case PK_LOOP4: case PK_LOOP8: case PK_LOOP16: case PK_LOOP32: case PK_STUTTER: case PK_LOOP8T: ready = fx_loop(s, wl, wr); break;
    case PK_REVERSE: ready = fx_reverse(s, wl, wr); break;
    case PK_TAPESTOP: ready = fx_slow(s, wl, wr, 0); break;
    case PK_HALF: ready = fx_slow(s, wl, wr, 1); break;
    case PK_REWIND: fx_rewind(s, wl, wr); break;
    case PK_OCTUP: case PK_WOBBLE: case PK_FLANGER: fx_modl(s, wl, wr, modl_p0); break;
    case PK_FREEZE: if (p->t) for (int n = 0; n < GB_BLOCK; n++) { wl[n] = ml[n] - dry_l[n]; wr[n] = mr[n] - dry_r[n]; } break;   /* (its first block: master_pre had not run) */
    case PK_ECHO: break;                                        /* (master_pre: into the delay) */
    default: fx_simple(s, wl, wr); break;
    }
    /* heard as much as the fade says: up while held (once there is something to hear), down once let go */
    float inc = 1.0f / (132.0f + (float)(s->pk_fade > 15 ? 15 : s->pk_fade) * 1087.0f);
    int up = p->want == p->fx;
    float mix = p->mix;
    for (int n = 0; n < GB_BLOCK; n++) {
        if (up) { if (ready) mix = gb_minf(mix + inc, 1.0f); } else mix = gb_maxf(mix - inc, 0.0f);
        ml[n] += (wl[n] - ml[n]) * mix; mr[n] += (wr[n] - mr[n]) * mix;
    }
    p->mix = mix; p->t += GB_BLOCK;
    if (!up && mix <= 0.0f) pk_end(s);                         /* (the next one held starts at the next block) */
}

void master_post(gb_sound *s, float *ml, float *mr, const float *dry_l, const float *dry_r)
{
    dust_block(s, ml, mr);
    /* the lines the punch-ins read: the mix's past (mono, half rate), the short stereo line */
    if (s->hist && !s->hist_stop) {
        uint32_t hp = s->hist_pos; uint8_t odd = s->hist_odd;
        for (int n = 0; n < GB_BLOCK; n++) {
            if (odd) { s->hist[hp] = s16((ml[n] + mr[n]) * 0.5f); hp = (hp + 1u) & (GB_HIST - 1u); }
            odd ^= 1u;
        }
        s->hist_pos = hp; s->hist_odd = odd;
    }
    uint32_t p0 = s->mod_pos;
    if (s->modl) {
        if (s->pk.fx != PK_FLANGER)                             /* (the flanger writes its own, with its feedback) */
            for (int n = 0; n < GB_BLOCK; n++) { uint32_t i = (p0 + (uint32_t)n) & (GB_MODL - 1u); s->modl[2 * i] = s16(ml[n]); s->modl[2 * i + 1] = s16(mr[n]); }
        s->mod_pos = (p0 + GB_BLOCK) & (GB_MODL - 1u);
    }
    punch_block(s, ml, mr, dry_l, dry_r, p0);
    float sf = sm_step(&s->mfilt);
    sm_end(&s->mfilt, sf);
    gb_svf_dj(ml, mr, s->mfilt.cur, s->mfilt_st, 1.3f);
}
