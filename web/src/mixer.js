/* Mixer: the five strips in full (DRIVE too) and the effects shared by the tracks (delay, reverb, chorus: HOME ids
 * 41-45), the master. Settings: everything of HOME + SELECT and of the MIDI pages (ED_HOME). */
words({
  fxShared: 'Shared effects', fxHelp: 'The delay, the reverb and the chorus the tracks send to (FX tapped on the FM-1: DELAY, REV · CHO). TIME follows the tempo.',
  dTime: 'DLY TIME', dColr: 'DLY COLR', dMix: 'DLY MIX', rDamp: 'REV DAMP', cRate: 'CHO RATE',
  setTitle: 'Settings of the FM-1', setHelp: 'As in HOME held + SELECT and GLO (MIDI). Each change is made at once and kept with the FM-1\'s settings.', reread: '↻',
  sScreen: 'Screen', sLights: 'Lights and keys', sAudio: 'Audio', sMidi: 'MIDI', sRec: 'Recording', sInfo: 'The FM-1',
  color: 'COLOR', yours: 'YOURS', calOk: 'calibration: ', learnt: 'MIDI LEARN: ', ccs: ' CC(s) learnt',
  hsHelp: ['The screen\'s movement eased (ACCEL).', 'How bright the lights are.', 'Which keys light: OFF, the C keys, the white keys, all.', 'The notes\' names on the keys.', 'SPEAKER: for the FM-1\'s own speaker (the drums\' lows made audible). OFF on headphones.',
    'USB audio: the master only, or FULL (each track).', 'USB audio rate: the computer\'s, or this one only.', 'The filter\'s resonance limit (OFF: none).', 'PREVIEW: a sound heard when chosen.', 'CORE 2: the second core plays half the voices.'],
  mIn: 'MIDI IN', mSync: 'SYNC', mOut: 'MIDI OUT', mClk: 'CLOCK OUT', mDmap: 'DRUM MAP', mRmch: 'REMOTE CH', mFxch: 'FX CH', mBend: 'BEND', rMode: 'REC MODE', rBars: 'REC BARS', rCount: 'COUNT-IN', rKeep: 'Track changed while recording', rKeepHelp: 'STOP: REC stops (the default). KEEP: the take goes on, on the new track. On the FM-1: GLO, REC 6/6.',
  mHelp: 'IN: the notes received play the tracks on their channels (TRACKS) or the track chosen (SEL). SYNC: the FM-1\'s tempo, or an external MIDI clock. OUT: the keys, or the sequencer too. DRUM MAP: the drums\' notes, GM or the pads in order. REMOTE CH / FX CH: a channel that remote-controls the FM-1 (0: off). BEND: in semitones.',
  rHelp: 'REC MODE: FREE (the first take sets the tempo) or at the TEMPO set. REC BARS: the take\'s length. COUNT-IN: a bar of clicks before it.'
}, {
  fxShared: 'Effets partagés', fxHelp: 'Le delay, la réverb et le chorus où envoient les pistes (FX tapé sur le FM-1 : DELAY, REV · CHO). TIME suit le tempo.',
  dTime: 'DLY TIME', dColr: 'DLY COLR', dMix: 'DLY MIX', rDamp: 'REV DAMP', cRate: 'CHO RATE',
  setTitle: 'Réglages du FM-1', setHelp: 'Comme dans HOME tenu + SELECT et GLO (MIDI). Chaque changement est fait tout de suite et gardé avec les réglages du FM-1.', reread: '↻',
  sScreen: 'Écran', sLights: 'Lumières et touches', sAudio: 'Audio', sMidi: 'MIDI', sRec: 'Enregistrement', sInfo: 'Le FM-1',
  color: 'COULEUR', yours: 'LE TIEN', calOk: 'calibrage : ', learnt: 'MIDI LEARN : ', ccs: ' CC appris',
  hsHelp: ['Le mouvement de l\'écran adouci (ACCEL).', 'La luminosité des lumières.', 'Quelles touches s\'allument : OFF, les do, les blanches, toutes.', 'Le nom des notes sur les touches.', 'SPEAKER : pour le haut-parleur du FM-1 (les graves de la batterie rendus audibles). OFF au casque.',
    'Audio USB : le master seul, ou FULL (chaque piste).', 'Fréquence de l\'audio USB : celle de l\'ordinateur, ou celle-ci seulement.', 'La limite de résonance du filtre (OFF : aucune).', 'PREVIEW : un son entendu quand il est choisi.', 'CORE 2 : le second cœur joue la moitié des voix.'],
  mIn: 'MIDI IN', mSync: 'SYNC', mOut: 'MIDI OUT', mClk: 'CLOCK OUT', mDmap: 'DRUM MAP', mRmch: 'CANAL REMOTE', mFxch: 'CANAL FX', mBend: 'BEND', rMode: 'MODE REC', rBars: 'MESURES REC', rCount: 'DÉCOMPTE', rKeep: 'Piste changée en enregistrant', rKeepHelp: 'STOP : le REC s\'arrête (par défaut). KEEP : la prise continue, sur la nouvelle piste. Sur le FM-1 : GLO, REC 6/6.',
  mHelp: 'IN : les notes reçues jouent les pistes sur leurs canaux (TRACKS) ou la piste choisie (SEL). SYNC : le tempo du FM-1, ou une horloge MIDI externe. OUT : les touches, ou le séquenceur aussi. DRUM MAP : les notes de la batterie, GM ou les pads dans l\'ordre. CANAL REMOTE / FX : un canal qui pilote le FM-1 (0 : aucun). BEND : en demi-tons.',
  rHelp: 'MODE REC : FREE (la première prise donne le tempo) ou au TEMPO réglé. MESURES REC : la longueur de la prise. DÉCOMPTE : une mesure de clics avant.'
});
const HOME = { r: null };
async function homeRead() { const b = await ask(ED.HOME, []); HOME.r = b; return b; }
async function homeSet(id, v) { try { HOME.r = await ask(ED.HOME, [id, v]); drawHomeVals(); } catch (e) { fail(e); } }
const DLY_DIV = ['1/32', '1/16', '1/8T', '1/16.', '1/8', '1/4T', '1/8.', '1/4', '1/4.'];
function drawMixer() {
  const P = $('tab_mixer'); P.innerHTML = '';
  const c = card(P, L('mix'), '', L('mixHelp'), 'wide'); const st = el('div', 'strips'); c.appendChild(st);
  MIX.strips = SHOWN.map(t => { const s = mkStrip(t, true); st.appendChild(s); return s; });
  const cards = el('div', 'cards'); cards.style.marginTop = '12px'; P.appendChild(cards);
  const fc = card(cards, L('fxShared'), '', L('fxHelp')); const ks = el('div', 'knobs'); fc.appendChild(ks);
  MIX.fx = [knob({ label: L('dTime'), min: 0, max: 8, fmt: v => DLY_DIV[v], id: 'fx.dtime', onChange: v => soon('h41', () => homeSet(41, v)) }),
    ...[[L('dColr'), 42], [L('dMix'), 43], [L('rDamp'), 44], [L('cRate'), 45]].map(([n, id]) => knob({ label: n, id: 'fx.' + id, onChange: v => soon('h' + id, () => homeSet(id, v)) }))];
  ks.append(...MIX.fx);
  const mc = card(cards, L('master'), '', L('masterHelp')); const mk = el('div', 'knobs'); mc.appendChild(mk);
  MIX.m = [knob({ label: L('volume'), max: 126, value: 100, onChange: v => soon('mst', () => act(8, v)) }),
    knob({ label: L('djf'), max: 200, value: 100, def: 100, bip: true, fmt: v => v === 100 ? 'OFF' : v < 100 ? 'LP' + (100 - v) : 'HP' + (v - 100), onChange: v => soon('mf', () => act(14, 255, 255, v)) }),
    knob({ label: L('dust'), max: 100, onChange: v => soon('du', () => act(14, v, 255, 255)) }), knob({ label: L('duck'), max: 100, onChange: v => soon('dk', () => act(14, 255, v, 255)) })];
  const kb2 = button(L('knobBtn'), () => act(8, 127), 'sm'); kb2.style.alignSelf = 'center'; mk.append(...MIX.m, kb2);
  if (P2()) homeRead().then(drawHomeVals).catch(fail);
  if (LV) MIX.upd(LV);
}
const MIX = { strips: [], upd(st) { if (!$('tab_mixer').classList.contains('on')) return; for (const s of MIX.strips) s.upd(st); if (MIX.m) { if (st.master !== 255) MIX.m[0].set(st.master); MIX.m[1].set(st.mfilt); MIX.m[2].set(st.dust); MIX.m[3].set(st.duck); } } };
liveHooks.push(st => MIX.upd(st));
/* the settings */
const HS_NAMES = ['ACCEL', 'LIGHTS', 'KEYS', 'NOTES', 'SPEAKER', 'USB', 'RATE', 'RESO', 'PREVIEW', 'CORE 2'];
const HS_VALS = [null, ['OFF', 'LOW', 'MID', 'HIGH'], ['OFF', 'C KEYS', 'WHITE', 'ALL'], null, null, ['MASTER', 'FULL'], ['AUTO', '44.1k', '48k'], null, null, null];
const THEMES = ['NIGHT', 'GREEN', 'AMBER', 'CYAN', 'RED', 'MONO'];
const SET = { ctl: [] };
function setCtl(parent, name, id, vals, help) {           /* a setting: a select of its values */
  const w = el('div', 'pgroup'); w.style.minWidth = '150px';
  const s = el('select', 'sel'); s.onchange = () => homeSet(id, +s.value);
  w.append(el('h4', '', name), s); if (help) { const h = el('div', 'dim', help); h.style.cssText = 'font-size:11.5px;margin-top:4px'; w.appendChild(h); }
  parent.appendChild(w); SET.ctl.push({ s, id, vals }); return s;
}
function drawSettings() {
  const P = $('tab_settings'); P.innerHTML = ''; SET.ctl = [];
  const h = el('div', 'row'); h.append(el('h2', '', L('setTitle')), button(L('reread'), () => homeRead().then(drawHomeVals).catch(fail), 'sm')); P.appendChild(h);
  P.appendChild(el('div', 'sub', L('setHelp')));
  const cards = el('div', 'cards'); P.appendChild(cards);
  const g = (title, help) => { const c = card(cards, title, '', help); const r = el('div', 'row'); r.style.alignItems = 'flex-start'; c.appendChild(r); return r; };
  const onoff = ['OFF', 'ON'];
  let r = g(L('sScreen'));
  setCtl(r, L('color'), 40, () => [...THEMES, L('yours')]); setCtl(r, HS_NAMES[0], 0, () => onoff, L('hsHelp')[0]);
  r = g(L('sLights')); for (const i of [1, 2, 3]) setCtl(r, HS_NAMES[i], i, () => HS_VALS[i] || onoff, L('hsHelp')[i]);
  r = g(L('sAudio')); for (const i of [4, 5, 6, 7, 8, 9]) setCtl(r, HS_NAMES[i], i, () => i === 7 ? ['OFF', ...[...Array(10)].map((_, k) => String(k + 1))] : HS_VALS[i] || onoff, L('hsHelp')[i]);
  r = g(L('sMidi'), L('mHelp'));
  setCtl(r, L('mIn'), 32, () => ['OFF', 'TRACKS', 'SEL']); setCtl(r, L('mSync'), 33, () => ['INT', 'EXT']); setCtl(r, L('mOut'), 34, () => ['OFF', 'KEYS', 'SEQ']);
  setCtl(r, L('mClk'), 35, () => onoff); setCtl(r, L('mDmap'), 36, () => ['GM', 'PADS']);
  setCtl(r, L('mRmch'), 37, () => ['OFF', ...[...Array(16)].map((_, k) => String(k + 1))]); setCtl(r, L('mFxch'), 38, () => ['OFF', ...[...Array(16)].map((_, k) => String(k + 1))]);
  setCtl(r, L('mBend'), 39, () => [...Array(13)].map((_, k) => k ? '±' + k : '-'));
  r = g(L('sRec'), L('rHelp'));
  setCtl(r, L('rMode'), 46, () => ['FREE', 'TEMPO']); setCtl(r, L('rBars'), 47, () => ['', '1', '2', '', '4']); setCtl(r, L('rCount'), 48, () => onoff); setCtl(r, L('rKeep'), 49, () => ['STOP', 'KEEP'], L('rKeepHelp'));
  const ic = card(cards, L('sInfo')); SET.info = el('div', 'dim'); ic.appendChild(SET.info);
  if (P2()) homeRead().then(drawHomeVals).catch(fail);
}
function drawHomeVals() {
  const b = HOME.r; if (!b) return; const o = 32;
  const val = id => id < 32 ? b[id] : id <= 40 ? b[o + id - 32] : id === 41 ? b[o + 9] : id <= 45 ? b[o + 10 + id - 42] : id === 49 ? (b[o + 20 + b[o + 19]] || 0) : b[o + 14 + id - 46];   /* (49: 3.3.6, after the maxima) */
  for (const c of SET.ctl) { const vs = c.vals(); fill(c.s, vs.map((t, i) => [i, t]).filter(([, t]) => t !== '')); c.s.value = val(c.id); }
  if (SET.info) SET.info.textContent = L('calOk') + (b[o + 17] ? 'OK' : '-') + ' · ' + L('learnt') + b[o + 18] + L('ccs') + (info ? ' · SLOOP ' + info.ver : '');
  if (MIX.fx) { MIX.fx[0].set(val(41)); for (let i = 0; i < 4; i++) MIX.fx[1 + i].set(val(42 + i)); }
}
