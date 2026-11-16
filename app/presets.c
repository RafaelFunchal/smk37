/* SPDX-License-Identifier: GPL-3.0-only
 * Browsing the factory sounds (presets.h). Written for this project. */
#include "presets.h"

static int is_kit(int i) { return gb_presets[i].engine == ENG_DRUM; }
/* the order PRESETS goes through them: by kind, then as written */
static int before(int a, int b) { return gb_presets[a].kind != gb_presets[b].kind ? gb_presets[a].kind < gb_presets[b].kind : a < b; }
int preset_step(int cur, int drum, int d)
{
    int i = cur;
    if (i < 0 || i >= gb_npresets || is_kit(i) != (drum != 0)) {          /* none yet: the first of that side */
        i = -1;
        for (int k = 0; k < gb_npresets; k++) if (is_kit(k) == (drum != 0) && (i < 0 || before(k, i))) i = k;
        if (d > 0) d--; else if (d < 0) d++;
        if (i < 0) return cur;
    }
    for (int steps = d > 0 ? d : -d; steps > 0; steps--) {
        int best = -1, wrap = -1;
        for (int k = 0; k < gb_npresets; k++) {
            if (is_kit(k) != (drum != 0) || k == i) continue;
            if (d > 0) {
                if (before(i, k) && (best < 0 || before(k, best))) best = k;
                if (wrap < 0 || before(k, wrap)) wrap = k;
            } else {
                if (before(k, i) && (best < 0 || before(best, k))) best = k;
                if (wrap < 0 || before(wrap, k)) wrap = k;
            }
        }
        i = best >= 0 ? best : wrap >= 0 ? wrap : i;
    }
    return i;
}
int preset_rank(int i, int *count)
{
    int r = 0, n = 0;
    if (i < 0 || i >= gb_npresets) { if (count) *count = 0; return 0; }
    for (int k = 0; k < gb_npresets; k++) if (is_kit(k) == is_kit(i)) { n++; if (k == i || before(k, i)) r++; }
    if (count) *count = n;
    return r;
}
void preset_apply(gb_sound *s, int trk, int i)
{
    if (i < 0 || i >= gb_npresets || trk < 0 || trk >= GB_NTRACK) return;
    const gb_preset *x = &gb_presets[i];
    gb_track *k = &s->trk[trk];
    if ((k->patch.engine == ENG_DRUM) != (x->engine == ENG_DRUM)) return;   /* the drum track stays drums */
    gb_patch *p = &k->patch;
    gb_patch_default(p, x->engine);
    for (int e = 0; e < 8; e++) gb_patch_set(p, SP_E0 + e, x->e[e]);
    if (x->engine != ENG_DRUM) {
        gb_patch_set(p, SP_ATK, x->env[0]); gb_patch_set(p, SP_DEC, x->env[1]); gb_patch_set(p, SP_SUS, x->env[2]); gb_patch_set(p, SP_REL, x->env[3]);
        gb_patch_set(p, SP_EFLT, x->fenv); gb_patch_set(p, SP_VOICE, x->mono ? VM_MONO : VM_POLY);
        gb_patch_set(p, SP_DIST, x->fx[0]);
        for (int j = 0; j + 1 < 8 && (uint8_t)x->x[j] != 0xFF; j += 2) if (x->x[j] < SP_N) gb_patch_set(p, x->x[j], x->x[j + 1]);
        p->protect = (uint8_t)(x->kind == K_BASSE);
    } else p->protect = 1;
    p->gain = x->gain ? (float)x->gain * 0.001f : 1.0f;
    k->mono_n = 0; k->mono_cur = -1;                      /* (notes sounding go on with their own engine) */
    if (x->engine == ENG_DRUM) return;                    /* (a kit, as on SLOOP: the track's sends stay) */
    sm_set(&k->send_cho, (float)x->fx[1] * (1.0f / 127.0f)); sm_set(&k->send_dly, (float)x->fx[2] * (1.0f / 127.0f));
    sm_set(&k->send_rev, (float)x->fx[3] * (1.0f / 127.0f));
}
float preset_gain(int i) { return i >= 0 && i < gb_npresets && gb_presets[i].gain ? (float)gb_presets[i].gain * 0.001f : 1.0f; }
int preset_trans(int i)
{
    if (i < 0 || i >= gb_npresets) return 0;
    const gb_preset *x = &gb_presets[i];
    for (int j = 0; j + 1 < 8 && (uint8_t)x->x[j] != 0xFF; j += 2) if (x->x[j] == PX_TRANS) return x->x[j + 1];
    return 0;
}
const uint8_t *preset_arp(int i) { return i >= 0 && i < gb_npresets && gb_presets[i].arp[0] ? gb_presets[i].arp : 0; }
int preset_of_sloop(int e, int k)
{
    if (e < 0 || e >= 12 || k < 0 || k >= gb_sloop_n[e]) return -1;
    return gb_sloop_map[e][k] == 0xFF ? -1 : gb_sloop_map[e][k];
}
