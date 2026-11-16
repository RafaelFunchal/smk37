/* SPDX-License-Identifier: GPL-3.0-only
 * Your sounds (usersnd.h). Written for this project. The bank's bytes: "S3US", a version (1), the number of places
 * (32), a spare byte, then the places (US_REC bytes each, usersnd.h). */
#include "usersnd.h"
#include "presets.h"

static uint32_t used;
uint16_t us_fav[US_NFAV] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
uint8_t us_theme[40];
void us_fav_put(uint8_t *b)
{
    for (int i = 0; i < 40; i++) b[US_FILE_FV + i] = us_theme[i];
    b[US_LEN] = 'F'; b[US_LEN + 1] = 'V';
    for (int i = 0; i < US_NFAV; i++) { b[US_LEN + 2 + 2 * i] = (uint8_t)us_fav[i]; b[US_LEN + 3 + 2 * i] = (uint8_t)(us_fav[i] >> 8); }
}
static char names[US_N][13];

static int ok_head(const uint8_t *b, int n)
{
    return n >= US_LEN && b[0] == 'S' && b[1] == '3' && b[2] == 'U' && b[3] == 'S' && b[4] == 1 && b[5] == US_N;
}
int us_get(const uint8_t *b, int k, us_sound *s)
{
    const uint8_t *r = b + 8 + k * US_REC;
    if (k < 0 || k >= US_N || r[0] != 0xA5 || r[1] >= ENG_N || r[1] == ENG_DRUM) return 0;
    s->used = r[0]; s->engine = r[1]; s->kind = r[2]; s->trans = (int8_t)r[3];
    for (int i = 0; i < 12; i++) s->name[i] = (char)r[4 + i];
    for (int i = 0; i < 4; i++) { s->fx[i] = r[16 + i]; s->arp[i] = r[20 + i]; }
    s->gain = (uint16_t)(r[24] | r[25] << 8); s->poly = r[26]; s->unison = r[27];
    for (int i = 0; i < GB_NPARAM; i++) s->p[i] = r[28 + i];
    return 1;
}
void us_put(uint8_t *b, int k, const us_sound *s)
{
    b[0] = 'S'; b[1] = '3'; b[2] = 'U'; b[3] = 'S'; b[4] = 1; b[5] = US_N; b[6] = b[7] = 0;
    uint8_t *r = b + 8 + k * US_REC;
    for (int i = 0; i < US_REC; i++) r[i] = 0;
    if (!s) return;
    r[0] = 0xA5; r[1] = s->engine; r[2] = s->kind; r[3] = (uint8_t)s->trans;
    for (int i = 0; i < 12; i++) r[4 + i] = (uint8_t)s->name[i];
    for (int i = 0; i < 4; i++) { r[16 + i] = s->fx[i]; r[20 + i] = s->arp[i]; }
    r[24] = (uint8_t)(s->gain & 0xFF); r[25] = (uint8_t)(s->gain >> 8); r[26] = s->poly; r[27] = s->unison;
    for (int i = 0; i < GB_NPARAM; i++) r[28 + i] = s->p[i];
}
void us_from_bytes(const uint8_t *b, int n)
{
    used = 0;
    int fv = n >= US_FILE_FV && b[US_LEN] == 'F' && b[US_LEN + 1] == 'V';
    for (int i = 0; i < 40; i++) us_theme[i] = n >= US_FILE ? b[US_FILE_FV + i] : 0;
    for (int i = 0; i < US_NFAV; i++) us_fav[i] = fv ? (uint16_t)(b[US_LEN + 2 + 2 * i] | b[US_LEN + 3 + 2 * i] << 8) : 0xFFFFu;
    for (int k = 0; k < US_N; k++) {
        us_sound s; names[k][0] = 0;
        if (!ok_head(b, n) || !us_get(b, k, &s)) continue;
        used |= 1u << k;
        int i = 0; for (; i < 12 && s.name[i] >= 32 && s.name[i] < 127; i++) names[k][i] = s.name[i] >= 'a' && s.name[i] <= 'z' ? (char)(s.name[i] - 32) : s.name[i];   /* (shown in capitals: the screen's letters; kept as written) */
        names[k][i] = 0;
        if (!i) { names[k][0] = 'U'; names[k][1] = 'S'; names[k][2] = 'E'; names[k][3] = 'R'; names[k][4] = 0; }
    }
}
int us_used(int k) { return k >= 0 && k < US_N && ((used >> k) & 1u); }
const char *us_name(int k) { return k >= 0 && k < US_N ? names[k] : ""; }

static uint8_t q8(float x) { x = x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; return (uint8_t)(x * 255.0f + 0.5f); }
void us_apply(gb_sound *snd, int t, const us_sound *s)
{
    gb_track *k = &snd->trk[t];
    if (k->patch.engine == ENG_DRUM) return;                   /* (the drum track stays drums) */
    gb_patch *p = &k->patch;
    gb_patch_default(p, s->engine);
    for (int i = 0; i < GB_NPARAM; i++) p->p[i] = (float)s->p[i] * (1.0f / 255.0f);
    p->poly = s->poly ? s->poly : p->poly; p->unison = s->unison && s->unison <= 4 ? s->unison : 1;
    p->gain = s->gain ? (float)s->gain * 0.001f : 1.0f;
    p->protect = (uint8_t)(s->kind == K_BASSE);
    k->mono_n = 0; k->mono_cur = -1;
    sm_set(&k->send_cho, (float)s->fx[1] * (1.0f / 127.0f)); sm_set(&k->send_dly, (float)s->fx[2] * (1.0f / 127.0f));
    sm_set(&k->send_rev, (float)s->fx[3] * (1.0f / 127.0f));
}
void us_take(const gb_sound *snd, int t, const float *own, const char *name, us_sound *s)
{
    const gb_track *k = &snd->trk[t];
    s->used = 0xA5; s->engine = k->patch.engine; s->kind = K_FX; s->trans = 0;
    int i = 0; for (; i < 12 && name && name[i]; i++) s->name[i] = name[i];
    for (; i < 12; i++) s->name[i] = 0;
    s->fx[0] = (uint8_t)gb_pv(k->patch.p, SP_DIST);
    s->fx[1] = (uint8_t)(k->send_cho.tgt * 127.0f + 0.5f); s->fx[2] = (uint8_t)(k->send_dly.tgt * 127.0f + 0.5f); s->fx[3] = (uint8_t)(k->send_rev.tgt * 127.0f + 0.5f);
    for (int j = 0; j < 4; j++) s->arp[j] = 0;
    float g = k->patch.gain * 1000.0f + 0.5f; s->gain = (uint16_t)(g < 1.0f ? 1.0f : g > 65535.0f ? 65535.0f : g);
    s->poly = k->patch.poly; s->unison = k->patch.unison;
    for (int j = 0; j < GB_NPARAM; j++) s->p[j] = q8(own[j]);
}
int us_from_sloop(sloop_rd rd, uint8_t *scratch, uint8_t *bank)
{
    int n = 0;
    for (int k = 0; k < US_N; k++) us_put(bank, k, 0);
    for (int b = 0; b < 2; b++) {
        int len = sloop_obj(rd, 5 + b, scratch, 3840u);
        const uint8_t *s = scratch;
        if (len < 8 + 16 * 192 || s[0] != 'U' || s[1] != 'P' || s[2] != 'B' || s[3] != '1' || (s[4] | s[5] << 8) != 192 || (s[6] | s[7] << 8) != 16) continue;
        for (int i = 0; i < 16; i++) {
            gb_patch p; char nm[13]; uint8_t fx[4], arp[4]; int tr;
            if (!sloop_upreset(s + 8 + 192 * i, &p, nm, fx, arp, &tr)) continue;
            us_sound u; u.used = 0xA5; u.engine = p.engine; u.kind = K_FX; u.trans = (int8_t)tr;
            int j = 0; for (; j < 12 && nm[j]; j++) u.name[j] = nm[j];
            for (; j < 12; j++) u.name[j] = 0;
            for (j = 0; j < 4; j++) { u.fx[j] = fx[j]; u.arp[j] = arp[j]; }
            u.gain = 1000; u.poly = p.poly; u.unison = p.unison ? p.unison : 1;
            gb_patch_set(&p, SP_DIST, fx[0]);
            for (j = 0; j < GB_NPARAM; j++) u.p[j] = q8(p.p[j]);
            us_put(bank, b * 16 + i, &u); n++;
        }
    }
    return n;
}
