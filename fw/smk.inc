/* SPDX-License-Identifier: GPL-3.0-only
 * fw/main.c's SMK-37 part (included there with BOARD_SMK37): what the SMK-37 has beyond the FM-1, played the SLOOP way.
 * The panel reads as an FM-1 (hal/smk37/panel.c: the same buttons, keys and knobs); here the rest:
 *   - KEYS: velocity from the unit's factory curves (the dynamic curve; no setting yet); the ten outer keys (C3..E3 below,
 *     G#5..C6 above) play on as keys 27..36 (their notes through the track's scale, chords and arpeggiator; on the drum
 *     track: KICK, SNARE, HAT below, BELL, OPEN, CRASH, RIDE above)
 *   - PADS: the FM-1's 16 white keys (steps, patterns, the mix, SOUNDS, notes, drums), lit in the track's colour;
 *     KNOB BANK: DRUM PADS, the pads play the drum kit whatever track the keys play (as SLOOP Pro)
 *   - FADERS, five banks (FADER BANK): A level, B filter, C reverb, D delay of the synth tracks 1-4; E the drums'
 *     level, filter, reverb, delay. A fader takes over once it reaches the value (no jump)
 *   - WHEELS: pitch bend on the selected synth track (GLO > MIDI 3 BEND, +-1..12), the mod wheel a vibrato
 *   - PEDAL: sustain (the keys' notes held while it is down), CC 64 out
 *   - < >: the previous / next track (SEQ held, or on STEPS: the step page)
 *   - TRS jack: MIDI OUT, what goes out over USB
 *   - KEYS (KNOB BANK held): K3 the LAYER, a second synth track every key plays too (a piano and a pad); K4 the SPLIT's
 *     track, what the keys below the split point play (a bass, the drums); a key pressed: the split point there (the
 *     same key again: no split). KNOB BANK tapped alone: DRUM PADS, as before */
COLD static int smk_key_pos(int k) { return k < 32 ? k : k - 37; }    /* slots 27..31 G#5..C6, 32..36 C3..E3 (as -5..-1) */
COLD static int smk_drum_outer(int pos)                               /* the drum lane an outer key plays */
{
    static const int8_t LO[5] = { 0, 0, 2, 2, 4 }, HI[5] = { 15, 5, 5, 11, 12 };   /* C3 C#3 D3 D#3 E3 | G#5 A5 A#5 B5 C6 */
    return pos < 0 ? LO[pos + 5] : HI[pos - 27];
}
/* ---- the sustain pedal: the keys let go while it is down end when it comes up (if not pressed again) */
static uint8_t smk_sus;
static uint64_t smk_sus_keys;
COLD static void smk_release(int k)
{
    if (smk_sus && key_nn[k]) { smk_sus_keys |= 1ull << k; return; }
    if (k < 27) roll[k].on = 0;
    key_off(k, latency_frames());
}
COLD static void smk_pedal(void)
{
    int p = smk_pedal_take();
    if (p < 0) return;
    static int8_t ch = -1;                                    /* (the channel its CC 64 went to: its release goes there too) */
    smk_sus = (uint8_t)p;
    if (p && !is_drum(trk)) { ch = (int8_t)midi_ch_of_track(trk); midi_out((uint8_t)(0xB0 | ch), 64, 127, 3); }
    else if (!p && ch >= 0) { midi_out((uint8_t)(0xB0 | ch), 64, 0, 3); ch = -1; }
    if (p) return;
    uint32_t held = panel_keys() | ek_held;
    for (int k = 0; k < PANEL_KEYS; k++)
        if ((smk_sus_keys >> k) & 1u) { if (!(k < 27 && ((held >> k) & 1u))) { if (k < 27) roll[k].on = 0; key_off(k, latency_frames()); } }
    smk_sus_keys = 0;
}
/* ---- the outer keys and the drum pads: notes only (in a layer they do nothing) */
COLD static void smk_play(int k, int t, const uint8_t *n, int nn, float vel)
{
    uint32_t lat = latency_frames();
    if (recording_armed && !Q.playing && !countin && !lay()) {   /* armed: the first note starts it (as the keys) */
        if (project_empty() && !rec_mode) { if (!ft_on) { ft_on = 1; ft_n = 0; ft_t0 = time_ticks(); ft_said = 0; } }
        else if (!rec_count) { record_now(t); lat = 0; }
    }
    if (t == trk) smk_key_on(k, n, nn, vel, lat); else key_on(k, t, n, nn, vel, lat);
}
COLD static void smk_outer(void)
{
    for (int e; (e = smk_ext_take()) >= 0;) {
        int kb = e >> 8, vel = e & 255;
        if (kb >= 0x40) {                                       /* DRUM PADS: pad (kb - 0x40) on the drum track */
            int pad = kb - 0x40, t = -1;
            for (int i = 0; i < NTRK; i++) if (is_drum(i)) t = i;
            if (t < 0) continue;
            static uint8_t pn[16];
            if (vel) { uint8_t n = (uint8_t)pad; play_on(GB_OWNER(OWN_KEY, 1), t, n, (float)vel / 127.0f); pn[pad] = 1;
                       if (Q.recording && Q.rec_trk == t) seq_key(&Q, n, vel, 1, latency_frames()); t_input = time_ticks(); }
            else if (pn[pad]) { pn[pad] = 0; play_off(GB_OWNER(OWN_KEY, 1), t, pad); if (Q.recording && Q.rec_trk == t) seq_key(&Q, pad, 0, 0, latency_frames()); }
            continue;
        }
        int k = kb < SMK_KEY_F3 ? 32 + kb : kb - SMK_KEY_F3;   /* keybed 0..4 -> 32..36, 32..36 -> 27..31 */
        if (k < 27 || k >= PANEL_KEYS) continue;
        t_input = time_ticks();
        if (!vel) { smk_release(k); continue; }
        smk_sus_keys &= ~(1ull << k);
        key_off(k, latency_frames());
        if (smk_keys_setup_key(k)) continue;
        if (lay()) continue;
        uint8_t n[5]; int nn;
        if (is_drum(trk)) { n[0] = (uint8_t)smk_drum_outer(smk_key_pos(k)); nn = 1; }
        else nn = notes_for_key(k, n);
        if (!nn) continue;
        for (int i = 0; i < nn; i++) last_chord[trk][i] = n[i];
        last_chord_n[trk] = (uint8_t)nn; last_note = n[0];
        smk_play(k, trk, n, nn, (float)vel / 127.0f);
    }
}
/* ---- the faders: five banks, soft takeover */
static uint8_t smk_fbank;
static int8_t smk_fpick[4] = { -1, -1, -1, -1 };              /* -1: waiting to reach the value; else its last */
static int8_t smk_fside[4];                                  /* (waiting: the side of the value the fader is on) */
COLD static int smk_trk_shown(int pos) { for (int t = 0; t < NTRK; t++) if (POS[t] == pos) return t; return 0; }
COLD static gb_smooth *smk_fader_target(int f, int *t)
{
    static const char *const P[4] = { "LEVEL", "FILTER", "REVERB", "DELAY" };
    (void)P;
    int what = smk_fbank < 4 ? smk_fbank : f;
    *t = smk_fbank < 4 ? smk_trk_shown(f) : smk_trk_shown(4);
    gb_track *k = &S.trk[*t];
    return what == 0 ? &k->level : what == 1 ? &k->filt : what == 2 ? &k->send_rev : &k->send_dly;
}
COLD static float smk_fval(int f, int v) { int t; gb_smooth *s = smk_fader_target(f, &t); return s == &S.trk[t].filt ? (float)v / 63.5f - 1.0f : (float)v / 127.0f; }
COLD static void smk_faders(void)
{
    static const char *const B[5] = { "A LEVEL 1-4", "B FILTER 1-4", "C REVERB 1-4", "D DELAY 1-4", "E DRUMS" };
    static const char *const P[4] = { "LEVEL", "FILTER", "REVERB", "DELAY" };
    for (int f = 0; f < 4; f++) {
        int v = smk_fader(f), t;
        if (v < 0) continue;
        gb_smooth *s = smk_fader_target(f, &t);
        float x = smk_fval(f, v), cur = s->tgt;
        if (smk_fpick[f] < 0) {                                 /* takes over once it crosses the value */
            int sd = x < cur - 0.02f ? -1 : x > cur + 0.02f ? 1 : 0;
            if (sd && (!smk_fside[f] || smk_fside[f] == sd)) { smk_fside[f] = (int8_t)sd; continue; }
            smk_fside[f] = 0; smk_fpick[f] = (int8_t)v;
        }
        if (v == smk_fpick[f]) continue;
        smk_fpick[f] = (int8_t)v; sm_set(s, x); t_input = time_ticks();
        char b[32], *p = put_s(b, smk_fbank < 4 ? P[smk_fbank] : P[f]);
        p = put_s(p, " "); p = put_s(p, is_drum(t) ? "DRUMS" : ""); if (!is_drum(t)) p = put_u(p, (uint32_t)POS[t] + 1u);
        p = put_s(p, " "); p = put_u(p, (uint32_t)v); *p = 0; say(b);
    }
    (void)B;
}
COLD static void smk_fader_bank(int d)
{
    static const char *const B[5] = { "FADERS: A · LEVEL 1-4", "FADERS: B · FILTER 1-4", "FADERS: C · REVERB 1-4", "FADERS: D · DELAY 1-4", "FADERS: E · DRUMS" };
    smk_fbank = (uint8_t)((smk_fbank + 5 + d) % 5);
    for (int f = 0; f < 4; f++) { smk_fpick[f] = -1; smk_fside[f] = 0; }
    say(B[smk_fbank]);
}
/* ---- the wheels: bend and vibrato, once a block, on the selected synth track */
static int8_t smk_bend_trk = -1;
static uint32_t smk_vib_ph;
static void smk_block(void)
{
    float b = smk_bend(), m = smk_mod();
    int t = is_drum(trk) ? -1 : trk;
    if (smk_bend_trk >= 0 && smk_bend_trk != t) S.trk[smk_bend_trk].bend = 0.0f;   /* (the old track back in tune) */
    if (t < 0) { smk_bend_trk = -1; return; }
    static float was;
    float v = 0.0f;
    if (m > 0.0f) { smk_vib_ph += (uint32_t)(5.5f * GB_BLOCK * GB_ISR * 4294967296.0f); v = m * 0.5f * gb_sin_u32(smk_vib_ph); }
    float x = b * (float)m_bend + v;
    if (x != was || smk_bend_trk != t) { S.trk[t].bend = x; was = x; }   /* (a MIDI bend is kept while the wheels rest) */
    smk_bend_trk = (int8_t)t;
}
/* ---- KEYS: the layer and the split (KNOB BANK held: K3, K4, a key) */
static int8_t smk_layer = -1, smk_lower = -1;               /* tracks (-1 none) */
static int8_t smk_split = -100;                              /* the split point: keys at positions below it play smk_lower */
static uint8_t smk_kb_used;                                  /* (KNOB BANK held and used: no DRUM PADS on its release) */
static int8_t smk_lay_trk[PANEL_KEYS] GB_BOOTRAM; static uint8_t smk_lay_n[PANEL_KEYS] GB_BOOTRAM, smk_lay_notes[PANEL_KEYS][5] GB_BOOTRAM;   /* (smk_keys_init) */
COLD static void smk_keys_init(void) { for (int k = 0; k < PANEL_KEYS; k++) { smk_lay_trk[k] = -1; smk_lay_n[k] = 0; } }
COLD static int smk_kb_held(void) { return (smk_xbuttons() >> SB_KNOB_BANK) & 1u; }
COLD static const char *smk_trk_name(int t, char *b)              /* "2 PAD SOFT" / "DRUMS" / "OFF" */
{
    if (t < 0) return "OFF";
    if (is_drum(t)) return "DRUMS";
    char *p = put_u(b, (uint32_t)POS[t] + 1u); p = put_s(p, " ");
    int pi = preset_of[t]; p = put_s(p, pi >= 0 && pi < gb_npresets ? gb_presets[pi].name : gb_edesc[S.trk[t].patch.engine].name); *p = 0;
    return b;
}
COLD static void smk_keys_say(void)
{
    char a[24], c[24], b[64], *p = put_s(b, "LAYER "); p = put_s(p, smk_trk_name(smk_layer, a));
    p = put_s(p, " · SPLIT "); p = put_s(p, smk_split > -100 && smk_lower >= 0 ? smk_trk_name(smk_lower, c) : "OFF");
    if (smk_split > -100 && smk_lower >= 0) { p = put_s(p, " < "); p = put_s(p, harm_note_name[((53 + smk_split) % 12 + 12) % 12]); p = put_u(p, (uint32_t)((53 + smk_split) / 12 - 1)); }
    *p = 0; say(b);
}
COLD static int smk_next_trk(int cur, int d, int drums)           /* OFF, then the tracks in the order shown (synths; the drums if asked) */
{
    int order[NTRK + 1], n = 0; order[n++] = -1;
    for (int pos = 0; pos < NTRK; pos++) { int t = smk_trk_shown(pos); if (!is_drum(t) || drums) order[n++] = t; }
    int i = 0; while (i < n && order[i] != cur) i++;
    if (i >= n) i = 0;
    return order[((i + d) % n + n) % n];
}
COLD static void smk_keys_setup_knobs(void)
{
    int dl = panel_enc_take(ENC_ALGORITHM), ds = panel_enc_take(ENC_PRESETS);
    for (int e = 0; e < 7; e++) if (e != ENC_ALGORITHM && e != ENC_PRESETS) (void)panel_enc_take(e);   /* (the others: nothing now) */
    if (dl) { smk_layer = (int8_t)smk_next_trk(smk_layer, dl > 0 ? 1 : -1, 0); smk_kb_used = 1; smk_keys_say(); }
    if (ds) { smk_lower = (int8_t)smk_next_trk(smk_lower, ds > 0 ? 1 : -1, 1); if (smk_lower >= 0 && smk_split == -100) smk_split = 7; smk_kb_used = 1; smk_keys_say(); }
}
COLD static int smk_keys_setup_key(int k)                    /* 1: the key was the split point's (KNOB BANK held) */
{
    if (!smk_kb_held()) return 0;
    int pos = smk_key_pos(k);
    smk_kb_used = 1;
    if (smk_split == pos) smk_split = -100;
    else { smk_split = (int8_t)pos; if (smk_lower < 0) smk_lower = (int8_t)smk_next_trk(-1, 1, 0); }
    smk_keys_say();
    return 1;
}
static void smk_layer_off(int k)
{
    int L = smk_lay_trk[k];
    if (L < 0 || L >= NTRK) { smk_lay_trk[k] = -1; return; }
    for (int i = 0; i < smk_lay_n[k]; i++) { if (arp_on(L)) arp_remove(L, smk_lay_notes[k][i]); else play_off(GB_OWNER(OWN_KEY, 0), L, smk_lay_notes[k][i]); }
    smk_lay_trk[k] = -1; smk_lay_n[k] = 0;
}
/* a key's notes: below the split point on the split's track (its own scale and chords), else on the track selected and
 * the layer (the same notes, the same velocity) */
COLD static void smk_key_on(int k, const uint8_t *n, int nn, float vel, uint32_t lat)
{
    smk_layer_off(k);
    if (smk_split > -100 && smk_lower >= 0 && smk_lower != trk && smk_key_pos(k) < smk_split) {
        uint8_t m[5]; int saved = trk; trk = smk_lower;
        int mn = is_drum(trk) && k >= 27 ? (m[0] = (uint8_t)smk_drum_outer(smk_key_pos(k)), 1) : notes_for_key(k, m);
        trk = saved;
        if (mn) key_on(k, smk_lower, m, mn, vel, lat);
        return;
    }
    key_on(k, trk, n, nn, vel, lat);
    int L = smk_layer;
    if (L < 0 || L >= NTRK || L == trk || is_drum(L) || is_drum(trk)) return;
    smk_lay_trk[k] = (int8_t)L; smk_lay_n[k] = (uint8_t)nn;
    for (int i = 0; i < nn; i++) {
        smk_lay_notes[k][i] = n[i];
        if (arp_on(L)) arp_add(L, n[i], (int)(vel * 127.0f)); else play_on(GB_OWNER(OWN_KEY, 0), L, n[i], vel);
    }
}
/* ---- the extra buttons */
static uint8_t smk_drumpads;
COLD static void smk_xbtns(void)
{
    uint32_t rel, pr = smk_xbutton_edges(&rel);
    if (!(pr | rel)) return;
    t_input = time_ticks();
    if (pr & (1u << SB_FADER_BANK)) smk_fader_bank((held_btn & (1u << BTN_OCT_DOWN)) ? -1 : 1);
    if (pr & (1u << SB_KNOB_BANK)) smk_kb_used = 0;
    if ((rel & (1u << SB_KNOB_BANK)) && !smk_kb_used) { smk_drumpads = (uint8_t)!smk_drumpads; say(smk_drumpads ? "PADS: THE DRUM KIT" : "PADS: THE WHITE KEYS"); }
    int d = (pr & (1u << SB_LEFT)) ? -1 : (pr & (1u << SB_RIGHT)) ? 1 : 0;
    if (!d) return;
    if (lay_on(BTN_SEQ) || sticky == PG_STEPS || notes_on()) {   /* the step page */
        int np = page + d, steps = Q.t[trk].pat[Q.t[trk].cur].steps;
        if (np >= 0 && np * 16 < steps) page = np;
        char b[16]; *put_u(put_s(b, "PAGE "), (uint32_t)page + 1u) = 0; say(b);
        return;
    }
    int pos = POS[trk] + d; pos = (pos + NTRK) % NTRK;
    select_track(smk_trk_shown(pos));
}
/* ---- the lights: the pads in the track's colour; DRUM PADS in the drums' (lit as they are hit) */
COLD static uint32_t smk_rgb(uint16_t c) { return (uint32_t)((c >> 11) & 31) * 255u / 31u << 16 | (uint32_t)((c >> 5) & 63) * 255u / 63u << 8 | (uint32_t)(c & 31) * 255u / 31u; }
COLD static int smk_drum_mode(void) { return smk_drumpads && !lay() && !is_drum(trk) && sticky != PG_STEPS && sticky != PG_SND && !notes_on(); }
COLD static void smk_lights(void)
{
    int dm = smk_drum_mode(), dt = 0;
    for (int i = 0; i < NTRK; i++) if (is_drum(i)) dt = i;
    smk_pad_mode(dm);
    smk_pad_tint(smk_rgb(TRK_COL[dm ? dt : trk]));
    if (dm) for (int w = 0; w < 16; w++) panel_level(14 + white_key[w], smk_pad_held(w) ? 4 : 1);
    smk_xled(SB_FADER_BANK, smk_fbank != 0);
    smk_xled(SB_KNOB_BANK, smk_drumpads);
    smk_xled(SB_LEFT, 0); smk_xled(SB_RIGHT, 0);
    smk_pads_flush();
}
/* once a round of the interface (ui_round) */
COLD static void smk_round(void)
{
    smk_xbtns(); smk_outer(); smk_pedal(); smk_faders(); smk_lights();
}
