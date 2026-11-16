/* SPDX-License-Identifier: GPL-3.0-only
 * TEXTURE (as SLOOP 2.8's GLO > TEXTURE, docs/sloop/SPEC-SON.md): a looping bed under the mix, without taking a
 * track. Written for this project after its behaviour:
 *   SRC   OFF, a generated noise (nothing to load): VINYL (hiss, rumble, crackles anywhere left to right, the odd
 *         pop), TAPE (hiss tilted dark, its level drifting), RAIN (a soft bed and droplets, the sides apart), WIND
 *         (noise through two resonant low-passes wandering, gusts), RADIO (a band of static, 500 Hz .. 3 kHz, coarse,
 *         in bursts), ROOM (a low rumble and a little air); or USR1..USR4: a user slot's first zone looped, at its
 *         own pitch
 *   LEVEL (squared: 64 about -30 dBFS rms, 127 about -18), TONE (the DJ filter: left a low-pass, right a high-pass),
 *   MOVE  how much it breathes over RATE (1 .. 16 bars of the tempo: its level and its brightness dip, from the
 *         transport's start), WHEN: PLAY (fades in and out with the transport, 0.1 s) or ALWAYS, DUCK: the kick
 *         pumps it as the synth tracks.
 * Into the dry mix (no sends), before the master's DUST, punch-ins and filter. Off and faded out: nothing runs. */
#include "engine.h"
#include "sample.h"

enum { TX_OFF, TX_VINYL, TX_TAPE, TX_RAIN, TX_WIND, TX_RADIO, TX_ROOM, TX_USR1 };
static struct {
    uint32_t rnd, ph; float g; uint8_t src, play;
    float a[2], b[2], c[2], d[2], lpm[2], t0, t1, t2, wz[2][2], f[4];
    int ticks;
    uint32_t upos; float ufrac, us0, us1, ulast, uoff; int up, ui; uint8_t uwrap, ustart;
} T = { .rnd = 0x2545F491u };

GB_INLINE uint32_t rnd32(void) { uint32_t x = T.rnd; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return T.rnd = x; }
GB_INLINE float n1(void) { return (float)(int32_t)rnd32() * (1.0f / 2147483648.0f); }   /* -1 .. 1 */
static void tx_reset(int src)
{
    uint32_t r = T.rnd, ph = T.ph; float g = T.g; uint8_t pl = T.play;
    memset(&T, 0, sizeof T);
    T.rnd = r ? r : 0x2545F491u; T.ph = ph; T.g = g; T.play = pl; T.src = (uint8_t)src;
    T.t0 = 50.0f; T.t1 = 60.0f; T.ustart = 1;
}
/* a user slot's first zone, looped: its next sample */
static float usr_next(const smp_zone *z, const uint8_t *d)
{
    uint32_t pos = T.upos, b = d[pos >> 1], code = (pos & 1u) ? b >> 4 : b & 15u;
    int step = smp_ima_step[T.ui], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    T.up += (code & 8u) ? -vd : vd; T.up = T.up < -32768 ? -32768 : T.up > 32767 ? 32767 : T.up;
    T.ui += smp_ima_idx[code & 7u]; T.ui = T.ui < 0 ? 0 : T.ui > 88 ? 88 : T.ui;
    if (++pos > (z->looped ? z->le : z->n - 1u)) { pos = z->looped ? z->ls : 0u; T.up = z->looped ? z->pred : 0; T.ui = z->looped ? z->idx : 0; T.uwrap = 1; }
    T.upos = pos;
    return (float)T.up * (1.0f / 32768.0f);
}
/* a block of the source, both sides (about 0.24 rms; SLOOP's levels, as fractions of its full scale) */
static void gen(int src, float *l, float *r)
{
    if (src >= TX_USR1) {
        int k = src - TX_USR1;
        if (!gb_flash_ok || !smp_user_nz(k)) { for (int i = 0; i < GB_BLOCK; i++) l[i] = r[i] = 0.0f; return; }
        uint32_t id = SMP_USR | (uint32_t)k << 5;
        const smp_zone *z = smp_zone_of(id); const uint8_t *d = smp_zone_data(id) + z->off;
        float st = (float)z->rate * (1.0f / 65536.0f);
        if (T.ustart) { T.ustart = 0; T.upos = 0; T.ufrac = 0.0f; T.up = T.ui = 0; T.us0 = 0.0f; T.us1 = usr_next(z, d); T.uwrap = 1; }
        for (int i = 0; i < GB_BLOCK; i++) {
            T.ufrac += st;
            while (T.ufrac >= 1.0f) { T.ufrac -= 1.0f; T.us0 = T.us1; T.us1 = usr_next(z, d); }
            float v = (T.us0 + (T.us1 - T.us0) * T.ufrac) * 0.5f;
            if (T.uwrap) { T.uwrap = 0; T.uoff = T.ulast - v; }     /* (round again: from where it was, no click) */
            T.uoff -= T.uoff * (1.0f / 64.0f);
            l[i] = r[i] = T.ulast = v + T.uoff;
        }
        T.uoff = gb_flush(T.uoff);
        return;
    }
    switch (src) {
    case TX_VINYL:
        for (int i = 0; i < GB_BLOCK; i++) {
            uint32_t z = rnd32(); float w = (float)(int32_t)(z >> 16) * (1.0f / 32768.0f) - 1.0f;
            T.a[0] += w * (1.0f / 32.0f) - T.a[0] * (1.0f / 128.0f);   /* rumble: integrated, leaky */
            if ((z & 0xFFFFu) < 60u) {                                   /* a crackle (~40 a second), anywhere */
                float amp = (6000.0f + (float)((z >> 8) & 0x3FFFu) + ((z & 1u) ? 0.0f : ((z & 0x30u) == 0u ? 12000.0f : 0.0f))) * (1.0f / 32768.0f);
                float pan = (float)((z >> 20) & 0xFFu) * (1.0f / 256.0f);
                if (z & 0x100u) amp = -amp;
                T.b[0] = amp * (1.0f - pan); T.b[1] = amp * pan;
            }
            for (int k = 0; k < 2; k++) {
                float h = k ? n1() : w;
                T.d[k] += (h - T.d[k]) * 0.25f;                          /* the hiss, dark */
                (k ? r : l)[i] = T.d[k] * (1.0f / 16.0f) + T.a[0] * 0.25f + T.b[k];
                T.b[k] -= T.b[k] * 0.25f;
            }
        }
        break;
    case TX_TAPE:
        if (!(T.ticks++ & 15)) T.t2 = 24.0f + (float)((rnd32() >> 20) & 0x7FFu) * (4.0f / 256.0f);
        T.t0 += (T.t2 - T.t0) * (1.0f / 32.0f);
        for (int i = 0; i < GB_BLOCK; i++)
            for (int k = 0; k < 2; k++) {
                float w = n1(); T.a[k] += (w - T.a[k]) * 0.25f;
                (k ? r : l)[i] = (T.a[k] * 0.5f + w * 0.125f) * T.t0 * (1.0f / 32.0f);
            }
        break;
    case TX_RAIN:
        for (int i = 0; i < GB_BLOCK; i++)
            for (int k = 0; k < 2; k++) {
                uint32_t z = rnd32(); float w = (float)(int32_t)(z >> 16) * (1.0f / 32768.0f) - 1.0f;
                T.a[k] += (w - T.a[k]) * (1.0f / 16.0f);                /* the bed, ~450 Hz */
                if ((z & 0xFFFFu) < 420u) T.b[k] = (4000.0f + (float)((z >> 4) & 0x3FFFu)) * (1.0f / 16384.0f);   /* a droplet */
                T.c[k] += (w - T.c[k]) * 0.5f;
                (k ? r : l)[i] = T.a[k] * 0.5f + (w - T.c[k]) * T.b[k];
                T.b[k] -= T.b[k] * (1.0f / 32.0f);
            }
        break;
    case TX_WIND: {                                                      /* two resonant low-passes wandering */
        if (!(rnd32() & 15u)) T.t2 = (float)(rnd32() & 0xFFFFu);
        uint32_t t2 = (uint32_t)T.t2;
        T.t0 += ((34.0f + (float)(t2 & 0xFF) * (44.0f / 256.0f)) - T.t0) * (1.0f / 64.0f);
        T.t1 += ((34.0f + (float)(t2 >> 8) * (44.0f / 256.0f)) - T.t1) * (1.0f / 64.0f);
        for (int k = 0; k < 2; k++) {
            float g = gb_tan_pi(gb_minf(gb_cut_hz(k ? T.t1 : T.t0) * GB_ISR, 0.49f)), q = 0.45f;   /* (resonant) */
            float a1 = 1.0f / (1.0f + g * (g + q)), a2 = g * a1, a3 = g * a2, ic1 = T.wz[k][0], ic2 = T.wz[k][1];
            float *o = k ? r : l;
            for (int i = 0; i < GB_BLOCK; i++) {
                float x = n1() * 0.25f, v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
                o[i] = gb_clampf(v2, -1.0f, 1.0f);
            }
            T.wz[k][0] = gb_flush(ic1); T.wz[k][1] = gb_flush(ic2);
        }
        break; }
    case TX_RADIO:
        if ((T.ticks -= GB_BLOCK) <= 0) {
            uint32_t z = rnd32();
            T.ticks = 1500 + (int)((z >> 8) & 0x3FFFu);                 /* 35 .. 400 ms */
            T.t2 = (z & 3u) == 0u ? 40.0f : 110.0f + (float)((z >> 24) & 0x7Fu);   /* (a gap now and then) */
        }
        T.t0 += (T.t2 - T.t0) * 0.44f;
        for (int i = 0; i < GB_BLOCK; i++) {
            if (!(i & 3)) T.c[0] = n1();                                 /* coarse: held 4 samples */
            T.a[0] += (T.c[0] - T.a[0]) * 0.5f; T.b[0] += (T.a[0] - T.b[0]) * (1.0f / 16.0f);
            float y = (T.a[0] - T.b[0]) * 0.5f * T.t0 * (1.0f / 64.0f);
            l[i] = y; r[i] = y * 0.75f;
        }
        break;
    case TX_ROOM:
        for (int i = 0; i < GB_BLOCK; i++)
            for (int k = 0; k < 2; k++) {
                float w = n1();
                T.a[k] += w * (1.0f / 16.0f) - T.a[k] * (1.0f / 256.0f);
                T.b[k] += (T.a[k] - T.b[k]) * (1.0f / 32.0f);
                (k ? r : l)[i] = T.b[k] * 0.5f + w * (1.0f / 64.0f);
            }
        break;
    default:
        for (int i = 0; i < GB_BLOCK; i++) l[i] = r[i] = 0.0f;
    }
    /* (3.1.8: a crackle or a droplet dies away to nothing, not into the denormals until the next one) */
    for (int k = 0; k < 2; k++) { T.a[k] = gb_flush(T.a[k]); T.b[k] = gb_flush(T.b[k]); T.c[k] = gb_flush(T.c[k]); T.d[k] = gb_flush(T.d[k]); }
}
void texture_reset(void) { tx_reset(0); T.g = 0.0f; T.ph = 0; }
/* the bed into the dry mix; duck0, duck1: the kick's pump at the block's two ends */
void texture_mix(gb_sound *s, float *ml, float *mr, float duck0, float duck1)
{
    static const float SC[TX_USR1 + 1] = { 0.0f, 3.125f, 1.176f, 2.707f, 7.492f, 0.582f, 1.340f, 1.0f };   /* (to ~0.24 rms) */
    static const uint8_t BARS[5] = { 1, 2, 4, 8, 16 };
    int src = s->tx[0] > TX_USR1 + 3 ? 0 : s->tx[0], lvl = s->tx[1];
    if (s->playing && !T.play) T.ph = 0;                                 /* MOVE from the transport's start */
    T.play = s->playing;
    int on = src && src == T.src && (s->playing || s->tx[5]) && lvl;
    if (T.g <= 0.0f && !on) { if (src != T.src) tx_reset(src); return; }
    float g0 = T.g, g1 = T.g = gb_clampf(T.g + (on ? 1.0f : -1.0f) * (476.0f / 32767.0f), 0.0f, 1.0f);   /* (~0.1 s) */
    float beat = s->beat > 1.0f ? s->beat : GB_SR * 0.5f;
    T.ph += (uint32_t)(4294967295.0f / (4.0f * beat * (float)BARS[s->tx[4] % 5]) * (float)GB_BLOCK);
    float u = (float)s->tx[3] * (1.0f / 127.0f) * 0.5f * (1.0f - gb_sin_turn((float)T.ph * (1.0f / 4294967296.0f) + 0.25f));   /* the dip, 0 .. MOVE */
    float mv = 1.0f - u * 0.641f, k0 = 1.0f - u * 0.854f;
    float tl[GB_BLOCK], tr[GB_BLOCK];
    gen(T.src, tl, tr);
    float sc = SC[T.src < TX_USR1 ? T.src : TX_USR1];
    for (int i = 0; i < GB_BLOCK; i++) {                                 /* MOVE's colour */
        T.lpm[0] += (tl[i] * sc - T.lpm[0]) * k0; T.lpm[1] += (tr[i] * sc - T.lpm[1]) * k0;
        tl[i] = T.lpm[0]; tr[i] = T.lpm[1];
    }
    if (s->tx[2]) gb_svf_dj(tl, tr, (float)(int8_t)s->tx[2] * (1.0f / 64.0f), T.f, 0.75f);
    else T.f[0] = T.f[1] = T.f[2] = T.f[3] = 0.0f;
    float k1 = (float)(lvl * lvl) * (0.517f / 16129.0f) * mv, dg = (g1 - g0) * (1.0f / GB_BLOCK), dd = (duck1 - duck0) * (1.0f / GB_BLOCK);
    for (int i = 0; i < GB_BLOCK; i++) {
        float g = (g0 + dg * (float)i) * k1;
        if (s->tx[6]) g *= duck0 + dd * (float)i;
        ml[i] += tl[i] * g; mr[i] += tr[i] * g;
    }
    if (g1 <= 0.0f && src != T.src) tx_reset(src);
}
