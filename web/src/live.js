/* LIVE: the FM-1 played from here, as SLOOP 2.8.5's editor did: the transport, the patterns (and a chain), the
 * scenes (and their morph), the mix, the keys and the pads, the punch-ins, the master, a set list of projects,
 * the set recorded as a WAV. Everything goes through ED_LIVE (fw/main.c): the panel's buttons and keys, the mix. */
words({
  transport: 'Transport', play: 'PLAY', stop: 'STOP', rec: 'REC', fill: 'FILL', fillNext: 'FILL NEXT BAR', tap: 'TAP', swing: 'SWING', bpm: 'BPM',
  songMode: 'SONG', songRec: 'SONG REC', chainDev: 'CHAIN (FM-1)', octD: 'OCT−', octU: 'OCT+',
  transportHelp: 'PLAY / STOP (Space), REC (Shift+R) as on the FM-1. FILL plays the fills while held; FILL NEXT BAR, one bar of them.\nSONG switches the loop and the song (SAVE + key 13 on the FM-1); SONG REC records the patterns you play into the song.\nBPM: drag, the wheel, or TAP four times.',
  patterns: 'Patterns', all: 'ALL', patHelp: 'A pad: that pattern next, on that track (it blinks until it starts). ALL: every track. A track\'s name: that track chosen on the FM-1.\nThe chain: + adds the pattern playing, in turn; while it is on, each pattern change queues the next one.',
  chain: 'Chain', chainOn: 'chain on', chainAdd: '+ pattern playing', chainClear: 'clear',
  scenes: 'Scenes', sceneHelp: 'A scene keeps the mix (levels, pans, sends, filters, mutes) and the master (DJ filter, dust, duck). Click: recall. Shift-click (or STORE then a scene): keep the mix now.\nMORPH: from scene A to scene B with the crossfader. Scenes stay in this browser.', store: 'STORE', morph: 'MORPH', sceneA: 'A', sceneB: 'B',
  mix: 'Mix', mixHelp: 'Each track: level (double-click: 100), pan, filter (centre: off), the sends to chorus, delay, reverb; M mute, S solo. The meter is the track\'s level on the FM-1.',
  keys: 'Keys', keysHelp: 'The FM-1\'s 27 keys (F3-G5) on the track chosen: they play and record as the keys of the FM-1 do (its octave, its scale, its chord).\nComputer keys: Q 2 W 3 E… and Z S X D C V G B. Arrows: ← → the track, ↑ ↓ the octave.',
  pads: 'Drum pads', padsHelp: 'The drum track\'s 16 sounds, played by MIDI (channel 10): low on a pad, softer; high, harder.\nROLL: the pad repeats while held, at the rate chosen. ERASE: a pad held removes its hits from the pattern playing.',
  roll: 'ROLL', erase: 'ERASE', playMode: 'PLAY',
  punch: 'Punch-in effects', punchHelp: 'Held: the effect on the master while held (the keys of FX held on the FM-1). LATCH: a click starts it, another stops it.', latch: 'LATCH',
  master: 'Master', masterHelp: 'VOLUME: the master from here (KNOB: the FM-1\'s MASTER knob again). DJ FILTER: low-pass left, high-pass right. DUST: the vinyl noise. DUCK: the synths duck under the kick.\nTHROW: the delay or the reverb fed fully while held.',
  volume: 'VOLUME', knobBtn: 'KNOB', djf: 'DJ FILT', dust: 'DUST', duck: 'DUCK', thrDly: 'DLY THROW', thrRev: 'REV THROW',
  setlist: 'Set list', setHelp: 'Your project files (.s3p) in the order of the set. CUE: written on the FM-1 in place of its project, opened, at its tempo (if set), ready; GO: then played.\nBack up your project first (Projects). The set list stays here until the page is closed.',
  addFiles: '+ files', cue: 'CUE', go: 'GO', cued: 'Ready: ',
  record: 'Record the set', recHelp: 'The FM-1\'s USB audio (or any input) recorded here, as a WAV file. Choose the input, then ● REC; ■ saves the file.', recStart: '● REC', recStop: '■ SAVE WAV', input: 'input: ',
  tools: 'Tools', learn: 'MIDI LEARN', lock: 'LOCK', panic: 'PANIC', track: 'Track ', locked: 'Locked', recording: 'recording ',
  shortcuts: 'Space PLAY/STOP · Shift+R REC · Shift+1-8 a pattern for every track · ←→ track · ↑↓ octave · Esc PANIC · F fill (held)'
}, {
  transport: 'Transport', play: 'PLAY', stop: 'STOP', rec: 'REC', fill: 'FILL', fillNext: 'FILL MESURE SUIV.', tap: 'TAP', swing: 'SWING', bpm: 'BPM',
  songMode: 'SONG', songRec: 'SONG REC', chainDev: 'CHAIN (FM-1)', octD: 'OCT−', octU: 'OCT+',
  transportHelp: 'PLAY / STOP (Espace), REC (Maj+R) comme sur le FM-1. FILL joue les fills tant qu\'il est tenu ; FILL MESURE SUIV., une mesure.\nSONG passe de la boucle à la song (SAVE + touche 13 sur le FM-1) ; SONG REC enregistre dans la song les patterns que tu joues.\nBPM : glisse, la molette, ou TAP quatre fois.',
  patterns: 'Patterns', all: 'TOUS', patHelp: 'Un pad : ce pattern ensuite, sur cette piste (il clignote jusqu\'à son départ). TOUS : toutes les pistes. Le nom d\'une piste : la piste choisie sur le FM-1.\nLa chaîne : + ajoute le pattern qui joue, à la suite ; quand elle est active, chaque changement de pattern met le suivant en file.',
  chain: 'Chaîne', chainOn: 'chaîne active', chainAdd: '+ pattern qui joue', chainClear: 'vider',
  scenes: 'Scènes', sceneHelp: 'Une scène garde le mix (niveaux, pans, envois, filtres, mutes) et le master (filtre DJ, dust, duck). Clic : la rappeler. Maj-clic (ou STORE puis une scène) : garder le mix actuel.\nMORPH : de la scène A à la scène B avec le crossfader. Les scènes restent dans ce navigateur.', store: 'STORE', morph: 'MORPH', sceneA: 'A', sceneB: 'B',
  mix: 'Mix', mixHelp: 'Chaque piste : niveau (double-clic : 100), pan, filtre (au centre : aucun), envois chorus, delay, réverb ; M mute, S solo. Le vumètre est le niveau de la piste sur le FM-1.',
  keys: 'Clavier', keysHelp: 'Les 27 touches du FM-1 (F3-G5) sur la piste choisie : elles jouent et enregistrent comme celles du FM-1 (son octave, sa gamme, son accord).\nClavier de l\'ordinateur : Q 2 W 3 E… et Z S X D C V G B (en QWERTY). Flèches : ← → la piste, ↑ ↓ l\'octave.',
  pads: 'Pads de batterie', padsHelp: 'Les 16 sons de la piste de batterie, joués en MIDI (canal 10) : en bas du pad, plus doux ; en haut, plus fort.\nROLL : le pad se répète tant qu\'il est tenu, à la vitesse choisie. ERASE : un pad tenu retire ses coups du pattern qui joue.',
  roll: 'ROLL', erase: 'ERASE', playMode: 'JOUER',
  punch: 'Effets punch-in', punchHelp: 'Tenu : l\'effet sur le master tant qu\'il est tenu (les touches de FX tenu sur le FM-1). LATCH : un clic le lance, un autre l\'arrête.', latch: 'LATCH',
  master: 'Master', masterHelp: 'VOLUME : le master depuis ici (KNOB : de nouveau le knob MASTER du FM-1). FILTRE DJ : passe-bas à gauche, passe-haut à droite. DUST : le bruit de vinyle. DUCK : les synthés s\'effacent sous le kick.\nTHROW : le delay ou la réverb à fond tant qu\'il est tenu.',
  volume: 'VOLUME', knobBtn: 'KNOB', djf: 'FILTRE DJ', dust: 'DUST', duck: 'DUCK', thrDly: 'THROW DLY', thrRev: 'THROW REV',
  setlist: 'Set list', setHelp: 'Tes fichiers de projet (.s3p) dans l\'ordre du set. CUE : écrit sur le FM-1 à la place de son projet, ouvert, à son tempo (s\'il est donné), prêt ; GO : puis joué.\nSauvegarde d\'abord ton projet (Projets). La set list reste ici tant que la page est ouverte.',
  addFiles: '+ fichiers', cue: 'CUE', go: 'GO', cued: 'Prêt : ',
  record: 'Enregistrer le set', recHelp: 'L\'audio USB du FM-1 (ou une autre entrée) enregistré ici, en fichier WAV. Choisis l\'entrée, puis ● REC ; ■ enregistre le fichier.', recStart: '● REC', recStop: '■ WAV', input: 'entrée : ',
  tools: 'Outils', learn: 'MIDI LEARN', lock: 'VERROU', panic: 'PANIC', track: 'Piste ', locked: 'Verrouillé', recording: 'enregistrement ',
  shortcuts: 'Espace PLAY/STOP · Maj+R REC · Maj+1-8 un pattern pour toutes les pistes · ←→ piste · ↑↓ octave · Échap PANIC · F fill (tenu)'
});
const LIVE = { strips: [], scenes: [], sceneA: 0, sceneB: 1, xf: 0, storeArm: false, chain: [], chainOn: false, chainPos: 0, lastCur: -1, latch: false, punchOn: -1,
  padMode: 0, rollRate: 4, set: [], taps: [] };
try { LIVE.scenes = JSON.parse(localStorage.getItem('sloop3.scenes') || '[]'); } catch (e) { LIVE.scenes = []; }
const saveScenes = () => { try { localStorage.setItem('sloop3.scenes', JSON.stringify(LIVE.scenes)); } catch (e) {} };
const fmtPan = v => v === 64 ? 'C' : v < 64 ? 'L' + (64 - v) : 'R' + (v - 64);
const fmtFilt = v => v === 64 ? 'OFF' : v < 64 ? 'LP' + (64 - v) : 'HP' + (v - 64);
function soundName(tk) {
  if (tk.preset === 0xFFFF) return 'OWN SOUND';
  const k = tk.preset & 0x7FFF, ed = tk.preset & 0x8000 ? ' *' : '';
  if (k >= 2000) return dxName(k - 2000) + ed;
  if (k >= 1000) { const u = NAMES.user[k - 1000]; return (u ? u.name : 'USER ' + (k - 999)) + ed; }
  const p = NAMES.presets[k]; return (p ? p.name : '#' + k) + ed;
}
/* a mixer strip (the LIVE page's, or the Mixer's, fuller) */
function mkStrip(t, full) {
  const s = el('div', 'strip'); s.style.setProperty('--c', TCOL[t]);
  const nm = el('div', 'nm'), en = el('div', 'eng'), id = 'mix' + (SHOWN.indexOf(t) + 1);
  nm.onclick = () => act(9, t); nm.style.cursor = 'pointer';
  const send = (what, v) => soon('mx' + t + what, () => act(4, t, what, v));
  const lv = fader({ color: TCOL[t], def: 100, id: id + '.level', onChange: v => send(0, v) });
  const mt = el('div', 'meter'), mi = el('i'); mt.appendChild(mi);
  const fr = el('div', 'row'); fr.style.alignItems = 'flex-end'; fr.style.gap = '6px'; fr.append(lv, mt);
  const pan = knob({ label: 'PAN', def: 64, bip: true, fmt: fmtPan, id: id + '.pan', color: TCOL[t], onChange: v => send(1, v) });
  const flt = knob({ label: 'FILT', def: 64, bip: true, fmt: fmtFilt, id: id + '.filt', color: TCOL[t], onChange: v => send(5, v) });
  const cho = knob({ label: 'CHO', id: id + '.cho', color: TCOL[t], onChange: v => send(2, v) });
  const dly = knob({ label: 'DLY', id: id + '.dly', color: TCOL[t], onChange: v => send(3, v) });
  const rev = knob({ label: 'REV', id: id + '.rev', color: TCOL[t], onChange: v => send(4, v) });
  const k1 = el('div', 'knobs'); k1.append(pan, flt); const k2 = el('div', 'knobs'); k2.append(cho, dly, rev);
  let drv = null; if (full && t !== DRUM_T) { drv = knob({ label: 'DRIVE', id: id + '.drive', color: TCOL[t], onChange: v => send(8, v) }); k1.appendChild(drv); }
  const m = button('M', () => act(4, t, 6, LV && LV.t[t].mute ? 0 : 1), 'sm red', 'mute', id + '.mute');
  const so = button('S', () => act(4, t, 7, LV && LV.t[t].solo ? 0 : 1), 'sm grn', 'solo', id + '.solo');
  const ms = el('div', 'row'); ms.append(m, so);
  const info = el('div', 'dim'); info.style.fontSize = '11px';
  s.append(nm, en, fr, ms, k1, k2, info); if (!full) k2.classList.add('adv');
  s.upd = st => {
    const tk = st.t[t]; nm.textContent = trkName(t) + ' · ' + soundName(tk); nm.title = nm.textContent; en.textContent = ENGINES[tk.eng] || '?';
    lv.set(tk.level); pan.set(tk.pan); flt.set(tk.filt); cho.set(tk.cho); dly.set(tk.dly); rev.set(tk.rev);
    mi.style.height = (tk.meter * 100 / 127) + '%'; m.classList.toggle('on', !!tk.mute); so.classList.toggle('on', !!tk.solo);
    s.style.outline = st.trk === t ? '1px solid ' + TCOL[t] : '';
    info.textContent = L('pat') + (tk.cur + 1) + (tk.step !== 255 ? ' · ' + (tk.step + 1) + '/' + tk.steps : ' · ' + tk.steps);
  };
  return s;
}
/* the keys of the FM-1 (27, F3-G5) */
function mkKeys(onDown, onUp) {
  const kb = el('div'); kb.id = 'kb'; const W = 100 / 16, keys = [];
  KEYS.forEach((k, i) => {
    const e = el('div', k.white ? 'w' : 'b');
    if (k.white) { e.style.left = (k.wi * W) + '%'; e.style.width = W + '%'; } else { const wl = KEYS[i - 1].wi; e.style.left = ((wl + 1) * W - W * 0.32) + '%'; e.style.width = (W * 0.64) + '%'; }
    if (k.white && (k.note % 12 === 0 || i === 0)) { const s = el('span', '', ['C', '', 'D', '', 'E', 'F'][k.note % 12] + (Math.floor(k.note / 12) - 1)); e.appendChild(s); }
    let on = false; const d = () => { if (!on) { on = true; e.classList.add('down'); onDown(i); } }, u = () => { if (on) { on = false; e.classList.remove('down'); onUp(i); } };
    e.addEventListener('pointerdown', ev => { if (learnMode) return; e.setPointerCapture(ev.pointerId); d(); });
    e.addEventListener('pointerup', u); e.addEventListener('pointercancel', u);
    e.hold = { d, u }; keys.push(e); kb.appendChild(e);
  });
  kb.keys = keys; return kb;
}
/* the computer's keys on the FM-1's: Q 2 W 3 E 4 R T 6 Y 7 U I 9 O 0 P - [ ], then Z S X D C V G B */
const PC_KEYS = ['KeyQ', 'Digit2', 'KeyW', 'Digit3', 'KeyE', 'Digit4', 'KeyR', 'KeyT', 'Digit6', 'KeyY', 'Digit7', 'KeyU', 'KeyI', 'Digit9', 'KeyO', 'Digit0', 'KeyP', 'Minus', 'BracketLeft',
  'BracketRight', 'KeyZ', 'KeyS', 'KeyX', 'KeyD', 'KeyC', 'KeyV', 'KeyG'];
function drawLive() {
  const P = $('tab_live'); P.innerHTML = ''; LIVE.strips = [];
  const cards = el('div', 'cards'); P.appendChild(cards);
  /* transport */
  const tc = card(cards, L('transport'), '', L('transportHelp'));
  const r1 = el('div', 'row');
  const bPlay = button('▶ ' + L('play'), () => act(5, 2), 'big grn', '', 'live.play');
  const bRec = button('● ' + L('rec'), () => press(BTN.REC), 'big red', '', 'live.rec');
  const bFill = holdButton(L('fill'), () => act(15, 1), () => act(15, 0), '', '', 'live.fill');
  const bFillN = button(L('fillNext'), () => act(15, 2), 'sm adv');
  r1.append(bPlay, bRec, bFill, bFillN);
  const r2 = el('div', 'row'); r2.style.marginTop = '8px';
  const kB = knob({ label: L('bpm'), min: 300, max: 3000, value: 1200, def: 1200, fmt: v => (v / 10).toFixed(1), id: 'live.bpm', onChange: v => soon('bpm', () => act(13, v & 255, v >> 8, 255)) });
  const kS = knob({ label: L('swing'), min: 0, max: 100, def: 0, id: 'live.swing', onChange: v => soon('sw', () => act(13, 0, 0, v)) });
  const bTap = button(L('tap'), () => {
    const now = performance.now(); LIVE.taps = LIVE.taps.filter(x => now - x < 2500); LIVE.taps.push(now);
    if (LIVE.taps.length >= 3) { const d = (LIVE.taps[LIVE.taps.length - 1] - LIVE.taps[0]) / (LIVE.taps.length - 1), bp = Math.round(600000 / d); if (bp >= 300 && bp <= 3000) act(13, bp & 255, bp >> 8, 255); }
  }, '', '', 'live.tap');
  const bSong = button(L('songMode'), () => layerKey(BTN.SAVE, WHITE_KEY[12]).catch(fail), 'adv', '', 'live.song');
  const bSongR = button(L('songRec'), () => layerKey(BTN.SAVE, WHITE_KEY[13]).catch(fail), 'red adv');
  const bOd = holdButton(L('octD'), () => btnDown(BTN.OCTD, 1), () => btnDown(BTN.OCTD, 0), 'sm'), bOu = holdButton(L('octU'), () => btnDown(BTN.OCTU, 1), () => btnDown(BTN.OCTU, 0), 'sm');
  r2.append(kB, kS, bTap, bSong, bSongR, bOd, bOu); tc.append(r1, r2);
  /* the FM-1's panel buttons, held as on the FM-1 (everything: the layers from here) */
  const r3 = el('div', 'row adv'); r3.style.marginTop = '8px';
  for (const [n, b] of [['FX', BTN.FX], ['SEL', BTN.SEL], ['ENV', BTN.ENV], ['LFO', BTN.LFO], ['EDIT', BTN.EDIT], ['GLO', BTN.GLO], ['HOME', BTN.HOME], ['SAVE', BTN.SAVE], ['ARP', BTN.ARP], ['SEQ', BTN.SEQ]])
    r3.appendChild(holdButton(n, () => btnDown(b, 1), () => btnDown(b, 0), 'sm', '', 'btn.' + n));
  tc.appendChild(r3);
  /* patterns */
  const pc = card(cards, L('patterns'), '', L('patHelp'), 'wide');
  const g = el('div', 'pgrid'); pc.appendChild(g);
  g.appendChild(el('div', 'dim', ''));
  for (let p = 0; p < 8; p++) { const a = button(L('all') + ' ' + (p + 1), () => act(6, 255, p), 'sm', '', 'pat.all' + (p + 1)); g.appendChild(a); }
  const pads = [];
  for (const t of SHOWN) {
    const h = el('div', 'nm', trkName(t)); h.style.cssText = 'cursor:pointer;font-weight:700;font-size:12px;color:' + TCOL[t]; h.onclick = () => act(9, t); g.appendChild(h);
    pads[t] = [];
    for (let p = 0; p < 8; p++) { const d = el('div', 'pad', String(p + 1)); d.style.setProperty('--c', TCOL[t]); d.onclick = ev => { if (!learnMode) act(6, ev.shiftKey ? 255 : t, p); }; learnReg('pat.' + (SHOWN.indexOf(t) + 1) + '.' + (p + 1), d, x => { if (x) act(6, t, p); }); g.appendChild(d); pads[t].push(d); }
  }
  const cr = el('div', 'row'); cr.style.marginTop = '8px';
  const cl = el('span', 'dim'); const bCh = button(L('chainOn'), () => { LIVE.chainOn = !LIVE.chainOn; LIVE.chainPos = 0; LIVE.lastCur = -1; drawChain(); }, 'sm');
  const drawChain = () => { cl.textContent = L('chain') + ': ' + (LIVE.chain.length ? LIVE.chain.map((p, i) => (LIVE.chainOn && i === LIVE.chainPos ? '[' + (p + 1) + ']' : p + 1)).join(' → ') : '-'); bCh.classList.toggle('on', LIVE.chainOn); };
  cr.append(button(L('chainAdd'), () => { if (LV && LIVE.chain.length < 16) { LIVE.chain.push(LV.t[LV.trk].cur); drawChain(); } }, 'sm'), button(L('chainClear'), () => { LIVE.chain = []; LIVE.chainOn = false; drawChain(); }, 'sm'), bCh,
    button(L('chainDev'), () => layerKey(BTN.SAVE, WHITE_KEY[14]).catch(fail), 'sm adv'), cl);
  pc.appendChild(cr); drawChain();
  /* scenes */
  const sc = card(cards, L('scenes'), '', L('sceneHelp'));
  const sg = el('div', 'scenes'), scs = [];
  const bStore = button(L('store'), () => { LIVE.storeArm = !LIVE.storeArm; bStore.classList.toggle('on', LIVE.storeArm); }, 'sm');
  for (let i = 0; i < 8; i++) { const d = el('div', 'scene', 'SCENE ' + (i + 1)); d.onclick = ev => { if (learnMode) return; if (ev.shiftKey || LIVE.storeArm) { sceneStore(i); LIVE.storeArm = false; bStore.classList.remove('on'); } else sceneRecall(i); drawScenes(); };
    learnReg('scene' + (i + 1), d, x => { if (x) sceneRecall(i); }); sg.appendChild(d); scs.push(d); }
  const drawScenes = () => scs.forEach((d, i) => { d.classList.toggle('has', !!LIVE.scenes[i]); d.classList.toggle('on', LIVE.sceneOn === i); });
  const xr = el('div', 'row'); xr.style.marginTop = '8px';
  const sA = select([...Array(8)].map((_, i) => [i, 'A: ' + (i + 1)]), LIVE.sceneA, v => LIVE.sceneA = +v), sB = select([...Array(8)].map((_, i) => [i, 'B: ' + (i + 1)]), LIVE.sceneB, v => LIVE.sceneB = +v);
  const xf = el('input'); xf.type = 'range'; xf.min = 0; xf.max = 127; xf.value = 0; xf.style.cssText = 'flex:1;min-width:60px'; xf.oninput = () => { LIVE.xf = +xf.value; soon('morph', morph, 60); };
  learnReg('morph', xf, x => { xf.value = x; LIVE.xf = x; soon('morph', morph, 60); });
  xr.style.flexWrap = 'nowrap'; sA.style.width = sB.style.width = '64px'; xr.append(bStore, el('b', 'dim', L('morph')), sA, xf, sB);
  sc.append(sg, xr); drawScenes();
  /* master */
  const mc = card(cards, L('master'), '', L('masterHelp'));
  const kV = knob({ label: L('volume'), max: 126, value: 100, id: 'master.vol', onChange: v => soon('mst', () => act(8, v)) });
  const bK = button(L('knobBtn'), () => act(8, 127), 'sm');
  const kF = knob({ label: L('djf'), max: 200, value: 100, def: 100, bip: true, fmt: v => v === 100 ? 'OFF' : v < 100 ? 'LP' + (100 - v) : 'HP' + (v - 100), id: 'master.djf', onChange: v => soon('mf', () => act(14, 255, 255, v)) });
  const kD = knob({ label: L('dust'), max: 100, id: 'master.dust', onChange: v => soon('du', () => act(14, v, 255, 255)) });
  const kU = knob({ label: L('duck'), max: 100, id: 'master.duck', onChange: v => soon('dk', () => act(14, 255, v, 255)) });
  const thr = { v: 0 }; const thrB = (bit, on) => { thr.v = on ? thr.v | bit : thr.v & ~bit; act(12, thr.v); };
  const bTd = holdButton(L('thrDly'), () => thrB(2, 1), () => thrB(2, 0), '', '', 'master.thrdly'), bTr = holdButton(L('thrRev'), () => thrB(1, 1), () => thrB(1, 0), '', '', 'master.thrrev');
  const mk = el('div', 'knobs'); mk.append(kV, kF, kD, kU); const mr = el('div', 'row'); mr.append(bK, bTd, bTr);
  mc.append(mk, mr);
  /* the mix */
  const xc = card(cards, L('mix'), '', L('mixHelp'), 'wide'); const strips = el('div', 'strips'); xc.appendChild(strips);
  for (const t of SHOWN) { const s = mkStrip(t, false); strips.appendChild(s); LIVE.strips.push(s); }
  /* the punch-ins */
  const fc = card(cards, L('punch'), '', L('punchHelp'), 'wide');
  const bL = button(L('latch'), () => { LIVE.latch = !LIVE.latch; bL.classList.toggle('on', LIVE.latch); if (!LIVE.latch && LIVE.punchOn >= 0) { act(7, 255); LIVE.punchOn = -1; } }, 'sm');
  fc.querySelector('h3').insertBefore(bL, fc.querySelector('h3 .help'));
  const fg = el('div', 'fxpads'), fpads = [];
  PUNCH.forEach((n, fx) => {
    const k = FX_KEY[fx], d = el('div', 'pad', n);
    const dn = () => { if (LIVE.latch) { if (LIVE.punchOn === k) { act(7, 255); LIVE.punchOn = -1; } else { act(7, k); LIVE.punchOn = k; } } else { act(7, k); LIVE.punchOn = k; } };
    const up = () => { if (!LIVE.latch && LIVE.punchOn === k) { act(7, 255); LIVE.punchOn = -1; } };
    d.addEventListener('pointerdown', ev => { if (learnMode) return; d.setPointerCapture(ev.pointerId); dn(); }); d.addEventListener('pointerup', up); d.addEventListener('pointercancel', up);
    learnReg('punch.' + n, d, x => x ? dn() : up()); fg.appendChild(d); fpads[k] = d;
  });
  fc.appendChild(fg);
  /* the keys */
  const kc = card(cards, L('keys'), '', L('keysHelp'), 'wide');
  const kb = mkKeys(k => keyDown(k, 1), k => keyDown(k, 0)); kc.appendChild(kb); LIVE.kb = kb;
  /* the drum pads */
  const dc = card(cards, L('pads'), '', L('padsHelp'));
  const mr2 = el('div', 'row'); const modes = [L('playMode'), L('roll'), L('erase')].map((n, i) => button(n, () => { LIVE.padMode = i; modes.forEach((b, j) => b.classList.toggle('on', j === i)); }, 'sm'));
  modes[0].classList.add('on');
  const rr = select([[2, '1/8'], [3, '1/8T'], [4, '1/16'], [6, '1/16T'], [8, '1/32']], LIVE.rollRate, v => LIVE.rollRate = +v);
  mr2.append(...modes, rr); dc.appendChild(mr2);
  const dg = el('div', 'drumpads'); dg.style.marginTop = '8px';
  PADS.forEach((n, p) => {
    const d = el('div', 'pad', n); d.style.setProperty('--c', TCOL[DRUM_T]); let roll = 0;
    const dn = vel => {
      d.classList.add('cur');
      if (LIVE.padMode === 2) { padErase(p); return; }
      noteOut(DRUM_T, p, vel, 1); setTimeout(() => noteOut(DRUM_T, p, 0, 0), 80);
      if (LIVE.padMode === 1) { const every = () => 60000 / ((LV ? LV.bpm : 120) * LIVE.rollRate); const go = () => { noteOut(DRUM_T, p, vel, 1); setTimeout(() => noteOut(DRUM_T, p, 0, 0), 40); roll = setTimeout(go, every()); }; roll = setTimeout(go, every()); }
    };
    const up = () => { d.classList.remove('cur'); clearTimeout(roll); roll = 0; };
    d.addEventListener('pointerdown', ev => { if (learnMode) return; d.setPointerCapture(ev.pointerId); const rc = d.getBoundingClientRect(); dn(clamp(Math.round(127 - (ev.clientY - rc.top) / rc.height * 90), 30, 127)); });
    d.addEventListener('pointerup', up); d.addEventListener('pointercancel', up);
    learnReg('pad.' + n, d, (x, ty) => x ? dn(ty === 0x90 ? x : 110) : up()); dg.appendChild(d);
  });
  dc.appendChild(dg);
  /* the set list */
  const lc = card(cards, L('setlist'), '', L('setHelp'));
  const lr = el('div', 'row');
  const list = el('div', 'list'); list.style.maxHeight = '220px';
  lr.append(button(L('addFiles'), () => { const f = $('file'); f.value = ''; f.multiple = true; f.accept = '.s3p'; f.onchange = async () => { for (const x of f.files) LIVE.set.push({ name: x.name.replace(/\.s3p$/i, ''), data: new Uint8Array(await x.arrayBuffer()), bpm: '' }); f.multiple = false; drawSet(); }; f.click(); }, 'sm'));
  const drawSet = () => {
    list.innerHTML = '';
    LIVE.set.forEach((s, i) => {
      const d = el('div'); const n = el('span', '', (i + 1) + '. ' + s.name); n.style.flex = '1';
      const bp = el('input', 'num'); bp.placeholder = 'BPM'; bp.value = s.bpm; bp.style.width = '58px'; bp.oninput = () => s.bpm = bp.value;
      const up = button('↑', () => { if (i) { [LIVE.set[i - 1], LIVE.set[i]] = [LIVE.set[i], LIVE.set[i - 1]]; drawSet(); } }, 'sm');
      const rm = button('×', () => { LIVE.set.splice(i, 1); drawSet(); }, 'sm');
      d.append(n, bp, button(L('cue'), () => setCue(s, false), 'sm'), button(L('go'), () => setCue(s, true), 'sm pri'), up, rm); list.appendChild(d);
    });
  };
  lc.append(lr, list); drawSet();
  /* the recording, the tools */
  const rc = card(cards, L('record'), '', L('recHelp'));
  const ins = el('select', 'sel'), bR = button(L('recStart'), () => recToggle(ins.value, bR, rinfo), 'red'), rinfo = el('span', 'dim');
  const rrow = el('div', 'row'); rrow.append(ins, bR, rinfo); rc.appendChild(rrow);
  listInputs(ins);
  const oc = card(cards, L('tools'), L('shortcuts'));
  const bLearn = button(L('learn'), () => { learnMode = !learnMode; bLearn.classList.toggle('on', learnMode); document.body.classList.toggle('learning', learnMode); if (learnMode) log(L('learnHint')); else { learnSel = null; for (const k in learnable) learnable[k].el.classList.remove('learn'); } }, '');
  const orow = el('div', 'row'); orow.append(bLearn, button(L('lock'), () => $('locked').classList.add('on')), button(L('panic'), panic, 'red')); oc.appendChild(orow);
  cards.append(tc, sc, mc, pc, xc, fc, kc, dc, lc, rc, oc);         /* (the small cards first, side by side) */
  /* drawn from the state */
  LIVE.upd = st => {
    bPlay.classList.toggle('on', st.playing); bPlay.textContent = st.playing ? '■ ' + L('stop') : '▶ ' + L('play');
    bRec.classList.toggle('on', st.rec || st.armed || st.countin); bFill.classList.toggle('on', st.fill);
    bSong.classList.toggle('on', st.song); bSongR.classList.toggle('on', st.songRec);
    kB.set(Math.round(st.bpm * 10)); kS.set(st.swing);
    if (st.master !== 255) kV.set(st.master); bK.classList.toggle('on', st.master === 255);
    kF.set(st.mfilt); kD.set(st.dust); kU.set(st.duck);
    for (const t of SHOWN) { const tk = st.t[t]; pads[t].forEach((d, p) => { d.classList.toggle('has', !!(tk.used >> p & 1)); d.classList.toggle('cur', tk.cur === p); d.classList.toggle('next', tk.next === p && tk.next !== tk.cur); }); }
    fpads.forEach((d, k) => d && d.classList.toggle('lit', st.punch === k));
    for (const s of LIVE.strips) s.upd(st);
    if (LIVE.chainOn && LIVE.chain.length > 1) {          /* the chain: each change, the next one queued */
      const c = st.t[st.trk].cur;
      if (c !== LIVE.lastCur) { LIVE.lastCur = c; const i = LIVE.chain.indexOf(c, LIVE.chainPos); LIVE.chainPos = i >= 0 ? i : LIVE.chainPos; const nx = LIVE.chain[(LIVE.chainPos + 1) % LIVE.chain.length]; LIVE.chainPos = (LIVE.chainPos + 1) % LIVE.chain.length; act(6, 255, nx); drawChain(); }
    }
  };
  if (LV) LIVE.upd(LV);
}
liveHooks.push(st => { if (LIVE.upd) LIVE.upd(st); });
/* the scenes */
function sceneStore(i) {
  if (!LV) return;
  LIVE.scenes[i] = { t: LV.t.map(k => ({ level: k.level, pan: k.pan, cho: k.cho, dly: k.dly, rev: k.rev, filt: k.filt, mute: k.mute })), mfilt: LV.mfilt, dust: LV.dust, duck: LV.duck };
  saveScenes(); log('SCENE ' + (i + 1) + ' ✓', 'ok');
}
async function sceneApply(s, prev) {
  try {
    for (let t = 0; t < NTRK; t++) { const a = s.t[t], b = prev && prev.t[t];
      for (const [w, key] of [[1, 'pan'], [2, 'cho'], [3, 'dly'], [4, 'rev'], [5, 'filt'], [0, 'level']]) if (!b || b[key] !== a[key]) await live(4, t, w, a[key]);
      if (!b || b.mute !== a.mute) await live(4, t, 6, a.mute); }
    if (!prev || prev.mfilt !== s.mfilt || prev.dust !== s.dust || prev.duck !== s.duck) await live(14, s.dust, s.duck, s.mfilt);
  } catch (e) { fail(e); }
}
function sceneRecall(i) { const s = LIVE.scenes[i]; if (!s) return; LIVE.sceneOn = i; sceneApply(s, LV && { t: LV.t, mfilt: LV.mfilt, dust: LV.dust, duck: LV.duck }); }
let morphLast = null;
function morph() {
  const A = LIVE.scenes[LIVE.sceneA], B = LIVE.scenes[LIVE.sceneB]; if (!A || !B) return;
  const f = LIVE.xf / 127, m = (a, b) => Math.round(a + (b - a) * f);
  const s = { t: A.t.map((a, t) => { const b = B.t[t]; return { level: m(a.level, b.level), pan: m(a.pan, b.pan), cho: m(a.cho, b.cho), dly: m(a.dly, b.dly), rev: m(a.rev, b.rev), filt: m(a.filt, b.filt), mute: f < 0.5 ? a.mute : b.mute }; }),
    mfilt: m(A.mfilt, B.mfilt), dust: m(A.dust, B.dust), duck: m(A.duck, B.duck) };
  sceneApply(s, morphLast); morphLast = s;
}
/* a pad held in ERASE: its hits out of the drum pattern playing */
async function padErase(p) {
  try { const P = await patRead(DRUM_T, 255); const n = P.notes.length; P.notes = P.notes.filter(x => (x.note & 15) !== p);
    if (P.notes.length !== n) { await patWrite(DRUM_T, P.pt, P.steps, P.notes); log(PADS[p] + ': −' + (n - P.notes.length), 'ok'); } } catch (e) { fail(e); }
}
/* the set list: a project into its place, opened, at its tempo */
async function setCue(s, go) {
  try {
    if (LV && LV.playing && !go) await live(5, 0);
    await fileWrite(0, 0, s.data);                        /* (3.3.5: the one project; received, the FM-1 opens it) */
    await retryBusy(() => ask(ED.LOAD, [0]), 80);
    const bp = Math.round(parseFloat(s.bpm) * 10); if (bp >= 300 && bp <= 3000) await live(13, bp & 255, bp >> 8, 255);
    if (go) await live(5, 1);
    log(L('cued') + s.name, 'ok'); refreshInfo().catch(() => {});
  } catch (e) { fail(e); }
}
/* the set recorded: an audio input, as a 16-bit stereo WAV */
const REC = { on: false };
async function listInputs(sel) {
  try {
    if (!navigator.mediaDevices || !navigator.mediaDevices.enumerateDevices) return;
    const ds = (await navigator.mediaDevices.enumerateDevices()).filter(d => d.kind === 'audioinput');
    fill(sel, ds.length ? ds.map((d, i) => [d.deviceId, d.label || (L('input') + (i + 1))]) : [['', L('input') + '-']]);
    const fm = ds.find(d => /fm-?1|sloop/i.test(d.label)); if (fm) sel.value = fm.deviceId;
  } catch (e) { /* no devices listed */ }
}
async function recToggle(dev, b, info) {
  if (REC.on) {
    REC.on = false; clearInterval(REC.tick); REC.node.disconnect(); REC.src.disconnect(); REC.stream.getTracks().forEach(t => t.stop()); REC.ac.close();
    const n = REC.len, d = new DataView(new ArrayBuffer(44 + n * 4)), ws = (o, s) => { for (let i = 0; i < 4; i++) d.setUint8(o + i, s.charCodeAt(i)); };
    ws(0, 'RIFF'); d.setUint32(4, 36 + n * 4, true); ws(8, 'WAVE'); ws(12, 'fmt '); d.setUint32(16, 16, true); d.setUint16(20, 1, true); d.setUint16(22, 2, true);
    d.setUint32(24, REC.rate, true); d.setUint32(28, REC.rate * 4, true); d.setUint16(32, 4, true); d.setUint16(34, 16, true); ws(36, 'data'); d.setUint32(40, n * 4, true);
    let o = 44; for (const [l, r] of REC.bufs) for (let i = 0; i < l.length; i++, o += 4) { d.setInt16(o, clamp(Math.round(l[i] * 32767), -32768, 32767), true); d.setInt16(o + 2, clamp(Math.round(r[i] * 32767), -32768, 32767), true); }
    const name = 'sloop3-set-' + new Date().toISOString().slice(0, 19).replace(/[:T]/g, '-') + '.wav';
    save(new Uint8Array(d.buffer), name, 'audio/wav'); log(name + ' (' + (n / REC.rate).toFixed(1) + ' s)', 'ok');
    b.textContent = L('recStart'); b.classList.remove('on'); info.textContent = ''; return;
  }
  try {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: { deviceId: dev ? { exact: dev } : undefined, echoCancellation: false, noiseSuppression: false, autoGainControl: false, channelCount: 2 } });
    const ac = new AudioContext(), src = ac.createMediaStreamSource(stream), node = ac.createScriptProcessor(4096, 2, 2);
    Object.assign(REC, { on: true, stream, ac, src, node, bufs: [], len: 0, rate: ac.sampleRate });
    node.onaudioprocess = e => { const ib = e.inputBuffer, l = ib.getChannelData(0), r = ib.numberOfChannels > 1 ? ib.getChannelData(1) : l; REC.bufs.push([l.slice(), r.slice()]); REC.len += l.length; };
    src.connect(node); node.connect(ac.destination);
    b.textContent = L('recStop'); b.classList.add('on');
    REC.tick = setInterval(() => { const s = REC.len / REC.rate; info.textContent = L('recording') + Math.floor(s / 60) + ':' + String(Math.floor(s % 60)).padStart(2, '0'); }, 500);
    const sel = b.parentNode.querySelector('select'); if (sel) listInputs(sel);
  } catch (e) { fail(e); }
}
function panic() { allNotesOff(); act(7, 255); LIVE.punchOn = -1; act(12, 0); act(15, 0); for (let k = 0; k < 27; k++) live(2, k, 0).catch(() => {}); for (let b = 0; b < 14; b++) live(1, b, 0).catch(() => {}); }
/* the computer's keyboard on the LIVE page */
const pcHeld = new Set();
function liveKey(ev, down) {
  if (!$('tab_live').classList.contains('on') || /INPUT|SELECT|TEXTAREA/.test((ev.target && ev.target.tagName) || '') || !out) return;
  if (ev.ctrlKey || ev.metaKey || ev.altKey) return;
  const c = ev.code;
  if (down && ev.repeat) { if (PC_KEYS.includes(c) || c === 'Space' || c === 'KeyF') ev.preventDefault(); return; }
  if (c === 'Space') { ev.preventDefault(); if (down) act(5, 2); return; }
  if (c === 'Escape') { if (down) panic(); return; }
  if (ev.shiftKey && down) {
    if (c === 'KeyR') { press(BTN.REC).catch(fail); return; }
    const m = /^Digit([1-8])$/.exec(c); if (m) { act(6, 255, +m[1] - 1); ev.preventDefault(); return; }
  }
  if (c === 'KeyF') { act(15, down ? 1 : 0); return; }
  if (down && (c === 'ArrowLeft' || c === 'ArrowRight')) { ev.preventDefault(); const i = SHOWN.indexOf(curTrk), j = clamp(i + (c === 'ArrowLeft' ? -1 : 1), 0, 4); act(9, SHOWN[j]); return; }
  if (c === 'ArrowUp' || c === 'ArrowDown') { ev.preventDefault(); btnDown(c === 'ArrowUp' ? BTN.OCTU : BTN.OCTD, down ? 1 : 0); return; }
  const k = PC_KEYS.indexOf(c);
  if (k >= 0 && !ev.shiftKey) { ev.preventDefault(); if (down) { if (pcHeld.has(k)) return; pcHeld.add(k); } else pcHeld.delete(k); const key = LIVE.kb && LIVE.kb.keys[k]; if (key) (down ? key.hold.d : key.hold.u)(); }
}
if (typeof window !== 'undefined') { window.addEventListener('keydown', e => liveKey(e, true)); window.addEventListener('keyup', e => liveKey(e, false)); }
