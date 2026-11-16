/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP 3's web editor (web/editor.html, built by tools/mkeditor.py from web/src/): the FM-1 over USB MIDI, the
 * protocol of app/edproto.h (SysEx F0 7D 53 33 cmd, 8-to-7 bytes, F7). Written for this project.
 * core: the words, the frames, one request at a time, the device's state, the controls (knobs, faders, pads),
 * MIDI learn, the log. */
const T = { en: {}, fr: {} };
const words = (en, fr) => { Object.assign(T.en, en); Object.assign(T.fr, fr); };
let lang = typeof navigator !== 'undefined' && /^fr/i.test(navigator.language || '') ? 'fr' : 'en';
const L = k => (T[lang][k] !== undefined ? T[lang][k] : T.en[k] !== undefined ? T.en[k] : k);
const $ = id => document.getElementById(id);
const log = (t, c) => { const d = document.createElement('div'); d.textContent = t; if (c) d.className = c; $('log').appendChild(d); $('log').scrollTop = 1e9; };
words({
  editor: 'editor', connect: 'Connect', connected: 'Connected', fw: 'SLOOP', noport: 'No "SLOOP 3" MIDI port. Ports seen: ', nomidi: 'This browser has no Web MIDI: use Chrome or Edge.',
  found: 'Connected: SLOOP ', proto: ', protocol ', oldfw: 'This FM-1 runs an older SLOOP 3 (protocol 1): install 3.1.5 or later for LIVE, the sequencer, the song and the settings.',
  err: ['', 'bad request', 'busy (a save under way): again in a moment', 'nothing there', 'check failed', 'not a SLOOP 3 file', 'flash error'],
  bad: 'Refused: ', timeout: 'no answer', viewAll: 'view: everything', viewSimple: 'view: essentials', full: 'full screen', fullOff: 'exit full screen',
  track: 'Track ', drums: 'DRUMS', pat: 'pattern ', empty: 'empty', none: '-', save: 'Save', load: 'Load', open: 'Open', yes: 'yes', no: 'no',
  unlock: 'Locked: hold to unlock', off: 'OFF', on: 'ON',
  tab_live: 'LIVE', tab_sound: 'Sound', tab_seq: 'Sequencer', tab_mixer: 'Mixer', tab_song: 'Song', tab_library: 'Library', tab_samples: 'Samples',
  tab_kits: 'Drum kit', tab_drumsynth: 'Drum synth', tab_fm6: 'FM6 bank', tab_projects: 'Projects', tab_settings: 'Settings', tab_theme: 'Theme',
  learnHint: 'MIDI learn: tap a control of the page, then move a knob or press a pad on your controller. Right-click a control: forget it.', learned: 'Learned: ', learnWait: 'now move a knob or press a pad on the controller…'
}, {
  editor: 'éditeur', connect: 'Connecter', connected: 'Connecté', fw: 'SLOOP', noport: 'Pas de port MIDI « SLOOP 3 ». Ports vus : ', nomidi: 'Ce navigateur n\'a pas le Web MIDI : prends Chrome ou Edge.',
  found: 'Connecté : SLOOP ', proto: ', protocole ', oldfw: 'Ce FM-1 a un SLOOP 3 plus ancien (protocole 1) : installe la 3.1.5 ou plus récente pour le LIVE, le séquenceur, la song et les réglages.',
  err: ['', 'requête invalide', 'occupé (une sauvegarde) : réessaie dans un instant', 'rien à cet endroit', 'vérification ratée', 'pas un fichier SLOOP 3', 'erreur de flash'],
  bad: 'Refusé : ', timeout: 'pas de réponse', viewAll: 'vue : tout', viewSimple: 'vue : l\'essentiel', full: 'plein écran', fullOff: 'quitter le plein écran',
  track: 'Piste ', drums: 'BATTERIE', pat: 'pattern ', empty: 'vide', none: '-', save: 'Enregistrer', load: 'Charger', open: 'Ouvrir', yes: 'oui', no: 'non',
  unlock: 'Verrouillé : maintiens pour déverrouiller', off: 'OFF', on: 'ON',
  tab_live: 'LIVE', tab_sound: 'Son', tab_seq: 'Séquenceur', tab_mixer: 'Mixage', tab_song: 'Song', tab_library: 'Bibliothèque', tab_samples: 'Échantillons',
  tab_kits: 'Kit batterie', tab_drumsynth: 'Synthé batterie', tab_fm6: 'Banque FM6', tab_projects: 'Projets', tab_settings: 'Réglages', tab_theme: 'Thème',
  learnHint: 'MIDI learn : touche un contrôle de la page, puis bouge un knob ou frappe un pad de ton contrôleur. Clic droit sur un contrôle : l\'oublier.', learned: 'Appris : ', learnWait: 'bouge maintenant un knob ou frappe un pad du contrôleur…'
});
/* ---------------------------------------------------------------- frames and requests */
function frame(cmd, raw) {
  const o = [0xF0, 0x7D, 0x53, 0x33, cmd]; let acc = 0, bits = 0;
  for (const b of raw) { acc |= b << bits; bits += 8; while (bits >= 7) { o.push(acc & 127); acc >>= 7; bits -= 7; } }
  if (bits) o.push(acc & 127); o.push(0xF7); return o;
}
function unpack(d) { const o = []; let acc = 0, bits = 0; for (const b of d) { acc |= (b & 127) << bits; bits += 7; if (bits >= 8) { o.push(acc & 255); acc >>= 8; bits -= 8; } } return o; }
const u16 = (a, i) => a[i] | a[i + 1] << 8, i16 = (a, i) => (u16(a, i) << 16) >> 16, u32 = (a, i) => (a[i] | a[i + 1] << 8 | a[i + 2] << 16 | a[i + 3] << 24) >>> 0;
const w16 = v => [v & 255, v >> 8 & 255], w32 = v => [v & 255, v >> 8 & 255, v >> 16 & 255, v >>> 24];
const str = (a, i, n) => { let s = ''; for (let k = 0; k < n && a[i + k]; k++) s += String.fromCharCode(a[i + k]); return s; };
const clamp = (v, a, b) => v < a ? a : v > b ? b : v;
function crc32(d) { let c = 0xFFFFFFFF; for (const b of d) { c ^= b; for (let k = 0; k < 8; k++) c = c & 1 ? (c >>> 1) ^ 0xEDB88320 : c >>> 1; } return (c ^ 0xFFFFFFFF) >>> 0; }
const ED = { INFO: 1, SOUND: 2, SET: 3, DESC: 4, READ: 5, WBEGIN: 6, WDATA: 7, WEND: 8, LOAD: 9, PATTERN: 10, THEME: 11, NOTES: 12, SAMPLE: 13, BANK: 14,
  LIVE: 15, NOTES2: 16, LOCKS: 17, SONG: 18, HOME: 19, NAMES: 20 };
let out = null, inp = null, waiting = null, queue = Promise.resolve();
function onMsg(e) {
  const d = e.data; if (d.length < 6 || d[0] !== 0xF0 || d[1] !== 0x7D || d[2] !== 0x53 || d[3] !== 0x33) return;
  const w = waiting; waiting = null; if (w) w(d[4], unpack(d.slice(5, d.length - 1)));
}
/* one request at a time: its answer (the same command), or an error */
let ask = function (cmd, raw = []) {
  const p = queue.then(() => new Promise((res, rej) => {
    if (!out) { rej(new Error(L('timeout'))); return; }
    const t = setTimeout(() => { waiting = null; rej(new Error(L('timeout'))); }, 3000);
    waiting = (c, b) => { clearTimeout(t); if (c === 0) rej(new Error(L('err')[b[0]] || ('code ' + b[0]))); else res(b); };
    out.send(frame(cmd, raw));
  }));
  queue = p.catch(() => {});
  return p;
};
const fail = e => log(L('bad') + (e && e.message || e), 'bad');
/* ---------------------------------------------------------------- the device */
const NTRK = 5, SHOWN = [0, 1, 2, 4, 3], DRUM_T = 3;               /* the tracks as the FM-1 shows them: synths 1-4, the drums 5th */
const TCOL = ['var(--t1)', 'var(--t2)', 'var(--t3)', 'var(--t5)', 'var(--t4)'];   /* (by the track inside) */
const trkName = t => t === DRUM_T ? L('drums') : L('track') + (SHOWN.indexOf(t) + 1);
const ENGINES = ['ANALOG', 'DRUM', 'DIGITAL', 'LOFI', 'PHASE', 'WHEEL', 'NOISE', 'VOICE', 'TRIO', 'PHYS', 'FM6', 'SAMPLE', 'GRAIN', 'SLICE'];
const PADS = ['KICK', 'KICK2', 'SNARE', 'CLAP', 'HAT', 'OPEN', 'PEDAL', 'RIM', 'SNR 2', 'TOM L', 'TOM H', 'CRASH', 'RIDE', 'SHAKE', 'CONGA', 'BELL'];
const GM_PAD = [36, 35, 38, 39, 42, 46, 44, 37, 40, 41, 48, 49, 51, 70, 63, 56];
const BTN = { OCTD: 0, OCTU: 1, FX: 2, SEL: 3, ENV: 4, LFO: 5, EDIT: 6, GLO: 7, HOME: 8, SAVE: 9, ARP: 10, SEQ: 11, PLAY: 12, REC: 13 };
const ENC = { SELECT: 0, ALGORITHM: 1, K1: 2, K2: 3, K3: 4, K4: 5, PRESETS: 6 };
/* the FM-1's 27 keys F3..G5: which are white, the white index, a punch-in each (fw/main.c punch_of_key) */
const KEYS = []; { let w = 0; for (let k = 0; k < 27; k++) { const n = (53 + k) % 12, white = [0, 2, 4, 5, 7, 9, 11].includes(n); KEYS.push({ white, wi: white ? w++ : -1, note: 53 + k }); } }
const WHITE_KEY = KEYS.map((k, i) => k.white ? i : -1).filter(i => i >= 0);
const BLK_FX = [0, 16, 0, 17, 0, 18, 0, 0, 19, 0, 20, 0, 0, 21, 0, 22, 0, 23, 0, 0, 24, 0, 25, 0, 0, 26, 0];
const FX_KEY = []; for (let k = 0; k < 27; k++) FX_KEY[KEYS[k].white ? KEYS[k].wi : BLK_FX[k]] = k;
const PUNCH = ['1/4', '1/8', '1/16', '1/32', 'STUT', 'REV', 'STOP', 'HALF', 'LP', 'HP', 'PHONE', 'CRUSH', 'ALIAS', 'GATE', 'ECHO', 'WOBBL', '-LOW', '-MID', '-HIGH', 'RWND', 'OCT+', 'FLANG', 'PAN', 'FREEZ', '8T', 'MONO', 'FADE'];
const DIVS = ['1/32', '1/16T', '1/16', '1/8T', '1/8', '1/4T', '1/4', '1/2', '1 BAR', '2 BARS'];
let info = null;                                       /* INFO: protocol, tracks, project, places used, version */
let LV = null;                                         /* the LIVE state (ED_LIVE), refreshed ~6 times a second */
let curTrk = 0;                                        /* the track chosen on the FM-1 (its trk) */
const P2 = () => info && info.proto >= 2;
function parseLive(b) {
  const o = b.slice(1), s = { op: b[0], playing: !!(o[0] & 1), rec: !!(o[0] & 2), armed: !!(o[0] & 4), song: !!(o[0] & 8), songRec: !!(o[0] & 16), fill: !!(o[0] & 32), countin: !!(o[0] & 64),
    bpm: u16(o, 1) / 10, bar: u16(o, 3), beat: o[5], trk: o[6], proj: o[7], used: o[8], cpu: o[9], peak: o[10], voices: o[11], master: o[12], punch: o[13],
    dust: o[14], duck: o[15], mfilt: o[16], swing: o[17], thr: o[18], msgGen: o[19], msgKind: o[20], msg: str(o, 21, 24), t: [] };
  for (let t = 0; t < NTRK; t++) { const q = 45 + 22 * t;
    s.t.push({ cur: o[q], next: o[q + 1], mute: o[q + 2], solo: o[q + 3], level: o[q + 4], pan: o[q + 5], cho: o[q + 6], dly: o[q + 7], rev: o[q + 8], filt: o[q + 9],
      eng: o[q + 10], preset: u16(o, q + 11), meter: o[q + 13], used: u16(o, q + 14), step: o[q + 16], steps: o[q + 17], div: o[q + 18], gate: o[q + 19], swing: o[q + 20], quant: o[q + 21] }); }
  s.pads = []; for (let i = 0; i < 16; i++) { const x = o[45 + 22 * NTRK + i]; s.pads.push(x === undefined ? { tune: 0, dec: 0 } : { tune: (x >> 4) - 8, dec: (x & 15) - 4 }); }
  return s;
}
const liveHooks = [];                                  /* the pages drawn from the state */
let liveBusy = 0;                                      /* a transfer under way: no polling */
async function live(op = 0, a = 0, b = 0, c = 0) {
  if (!P2()) return null;
  const r = await ask(ED.LIVE, [op, a & 255, b & 255, c & 255]);
  LV = parseLive(r); curTrk = LV.t[LV.trk] ? LV.trk : curTrk;
  for (const h of liveHooks) try { h(LV); } catch (e) { console.error(e); }
  return LV;
}
const act = (op, a, b, c) => live(op, a, b, c).catch(fail);
/* the FM-1's panel from here: a button or a key down then up; a knob turned */
const btnDown = (b, on) => act(1, b, on ? 1 : 0);
const keyDown = (k, on) => act(2, k, on ? 1 : 0);
const turn = (e, d) => act(3, e, d & 255);
async function press(b, ms = 60) { await live(1, b, 1); await sleep(ms); await live(1, b, 0); }
async function layerKey(b, k) { await live(1, b, 1); await sleep(40); await live(2, k, 1); await sleep(50); await live(2, k, 0); await sleep(30); await live(1, b, 0); }
const sleep = ms => new Promise(r => setTimeout(r, ms));
/* notes to the FM-1 as MIDI: the synths on channels 1-4 (as shown), the drums on 10 (GM) */
function noteOut(t, n, vel, on) {
  if (!out) return; const ch = t === DRUM_T ? 9 : SHOWN.indexOf(t);
  const k = t === DRUM_T ? GM_PAD[n & 15] : n;
  out.send(on ? [0x90 | ch, k & 127, clamp(vel, 1, 127)] : [0x80 | ch, k & 127, 0]);
}
function allNotesOff() { if (!out) return; for (let c = 0; c < 16; c++) out.send([0xB0 | c, 123, 0]); }
/* ---------------------------------------------------------------- the controls */
const learnable = {};                                  /* id -> { el, apply(v 0..127) } */
let learnMode = false, learnSel = null, learnMap = {};
try { learnMap = JSON.parse(localStorage.getItem('sloop3.learn') || '{}'); } catch (e) { learnMap = {}; }
function learnReg(id, el, apply) {
  learnable[id] = { el, apply };
  el.addEventListener('pointerdown', ev => { if (!learnMode) return; ev.stopPropagation(); ev.preventDefault(); for (const k in learnable) learnable[k].el.classList.remove('learn'); learnSel = id; el.classList.add('learn'); log(id + ': ' + L('learnWait')); }, true);
  el.addEventListener('contextmenu', ev => { if (!learnMode) return; ev.preventDefault(); for (const k in learnMap) if (learnMap[k] === id) delete learnMap[k]; saveLearn(); });
}
const saveLearn = () => { try { localStorage.setItem('sloop3.learn', JSON.stringify(learnMap)); } catch (e) {} };
function ctlIn(e) {                                    /* a controller's message: learnt, or played */
  const d = e.data; if (d.length < 3 || d[0] >= 0xF0) return;
  const ty = d[0] & 0xF0, key = (ty === 0xB0 ? 'cc' : 'n') + (d[0] & 15) + ':' + d[1];
  if (ty !== 0xB0 && ty !== 0x90 && ty !== 0x80) return;
  if (learnMode && learnSel && (ty === 0xB0 || (ty === 0x90 && d[2]))) { learnMap[key] = learnSel; saveLearn(); learnable[learnSel].el.classList.remove('learn'); log(L('learned') + learnSel + ' ← ' + key, 'ok'); learnSel = null; return; }
  const id = learnMap[key]; if (!id || !learnable[id]) return;
  learnable[id].apply(ty === 0x80 ? 0 : d[2], ty);
}
/* a knob: drag up / down (Shift: fine), wheel, double-click: its default */
function knob(o) {
  const el = document.createElement('div'); el.className = 'knob';
  const min = o.min ?? 0, max = o.max ?? 127, span = max - min || 1; let v = o.value ?? min;
  el.innerHTML = '<svg viewBox="0 0 42 42"><circle cx="21" cy="21" r="16" fill="none" stroke="#2f2940" stroke-width="5" stroke-dasharray="75 26" transform="rotate(135 21 21)"/><path fill="none" stroke-width="5" stroke-linecap="round"/><line x1="21" y1="21" x2="21" y2="8" stroke="#efeaf7" stroke-width="2.5" stroke-linecap="round"/></svg><div class="v"></div><div class="l"></div>';
  const arc = el.querySelector('path'), ln = el.querySelector('line'), vt = el.querySelector('.v'), lt = el.querySelector('.l');
  arc.setAttribute('stroke', o.color || 'var(--acc)'); lt.textContent = o.label || '';
  if (o.tip) el.title = o.tip;
  const draw = () => {
    const f = (v - min) / span, a0 = -135, a = a0 + 270 * f, c = (x, y) => [21 + 16 * Math.sin(x * Math.PI / 180), 21 - 16 * Math.cos(x * Math.PI / 180)];
    const from = o.bip ? 0 : a0, [x0, y0] = c(Math.min(from, a)), [x1, y1] = c(Math.max(from, a)), big = Math.abs(a - from) > 180 ? 1 : 0;
    arc.setAttribute('d', Math.abs(a - from) < 0.5 ? '' : `M${x0} ${y0} A16 16 0 ${big} 1 ${x1} ${y1}`);
    ln.setAttribute('transform', `rotate(${a} 21 21)`); vt.textContent = o.fmt ? o.fmt(v) : Math.round(v);
  };
  const setV = (x, send) => { x = clamp(Math.round(x), min, max); if (x === v) return; v = x; draw(); if (send && o.onChange) o.onChange(v); };
  el.set = x => { if (dragging) return; v = clamp(x, min, max); draw(); }; el.get = () => v;
  let dragging = false, y0 = 0, v0 = 0;
  el.addEventListener('pointerdown', ev => { if (learnMode) return; dragging = true; y0 = ev.clientY; v0 = v; el.setPointerCapture(ev.pointerId); });
  el.addEventListener('pointermove', ev => { if (!dragging) return; const d = (y0 - ev.clientY) * span / (ev.shiftKey ? 600 : 150); setV(v0 + d, true); });
  el.addEventListener('pointerup', () => { dragging = false; if (o.onRelease) o.onRelease(v); });
  el.addEventListener('wheel', ev => { ev.preventDefault(); setV(v + (ev.deltaY < 0 ? 1 : -1) * Math.max(1, Math.round(span / 64)), true); }, { passive: false });
  el.addEventListener('dblclick', () => { if (o.def !== undefined) setV(o.def, true); });
  const mid = min + span / 2;                          /* (a bipolar knob: CC 64 its centre) */
  if (o.id) learnReg(o.id, el, x => setV(o.bip ? (x <= 64 ? min + x * (mid - min) / 64 : mid + (x - 64) * (max - mid) / 63) : min + x * span / 127, true));
  draw(); return el;
}
/* a fader: drag (Shift: fine), double-click: its default; a meter beside it */
function fader(o) {
  const el = document.createElement('div'); el.className = 'fader'; el.innerHTML = '<div class="fill"></div><div class="cap"></div>';
  const fill = el.children[0], cap = el.children[1], max = o.max ?? 127; let v = o.value ?? 0, dragging = false;
  fill.style.background = o.color || 'var(--acc)';
  const draw = () => { const f = v / max, h = 150 - 14; cap.style.bottom = (f * h) + 'px'; fill.style.height = (f * 100) + '%'; };
  const setV = (x, send) => { x = clamp(Math.round(x), 0, max); if (x === v) return; v = x; draw(); if (send && o.onChange) o.onChange(v); };
  el.set = x => { if (!dragging) { v = x; draw(); } };
  let y0 = 0, v0 = 0;
  el.addEventListener('pointerdown', ev => { if (learnMode) return; dragging = true; y0 = ev.clientY; v0 = v; el.setPointerCapture(ev.pointerId); });
  el.addEventListener('pointermove', ev => { if (dragging) setV(v0 + (y0 - ev.clientY) * max / (ev.shiftKey ? 500 : 136), true); });
  el.addEventListener('pointerup', () => { dragging = false; });
  el.addEventListener('dblclick', () => { if (o.def !== undefined) setV(o.def, true); });
  if (o.tip) el.title = o.tip;
  if (o.id) learnReg(o.id, el, x => setV(x * max / 127, true));
  draw(); return el;
}
function button(text, onClick, cls = '', tip = '', id = '') {
  const b = document.createElement('button'); b.className = 'btn ' + cls; b.textContent = text; if (tip) b.title = tip;
  if (onClick) b.onclick = ev => { if (!learnMode) onClick(ev); };
  if (id) learnReg(id, b, (x, ty) => { if (x >= 64 || ty === 0x90) onClick(); });
  return b;
}
/* a button that acts while held (pointer, or a controller's note / CC held) */
function holdButton(text, down, up, cls = '', tip = '', id = '') {
  const b = document.createElement('button'); b.className = 'btn ' + cls; b.textContent = text; if (tip) b.title = tip; let on = false;
  const d = () => { if (on) return; on = true; b.classList.add('on'); down(); }, u = () => { if (!on) return; on = false; b.classList.remove('on'); up(); };
  b.addEventListener('pointerdown', ev => { if (learnMode) return; b.setPointerCapture(ev.pointerId); d(); });
  b.addEventListener('pointerup', u); b.addEventListener('pointercancel', u);
  if (id) learnReg(id, b, x => x >= 64 ? d() : u());
  b.hold = { d, u }; return b;
}
function el(tag, cls, text) { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; }
function card(parent, title, sub, help, cls = '') {
  const c = el('div', 'card ' + cls), h = el('h3'); h.append(title);
  if (help) { const hb = el('button', 'help', '?'), box = el('div', 'helpbox', help); hb.onclick = () => box.classList.toggle('on'); h.appendChild(hb); c.append(h, box); }
  else c.appendChild(h);
  if (sub) c.appendChild(el('div', 'sub', sub));
  parent.appendChild(c); return c;
}
function select(items, value, onChange) {
  const s = el('select', 'sel'); for (const [v, t] of items) { const o = document.createElement('option'); o.value = v; o.textContent = t; s.appendChild(o); }
  if (value !== undefined) s.value = value; if (onChange) s.onchange = () => onChange(s.value); return s;
}
function fill(sel, items) { const keep = sel.value; sel.innerHTML = ''; items.forEach(([v, t]) => { const o = document.createElement('option'); o.value = v; o.textContent = t; sel.appendChild(o); }); if (keep !== '') sel.value = keep; if (sel.selectedIndex < 0) sel.selectedIndex = 0; }
function save(bytes, name, type = 'application/octet-stream') {
  const a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([bytes], { type })); a.download = name;
  document.body.appendChild(a); a.click(); a.remove();
}
function pick(then, accept) { const f = $('file'); f.value = ''; f.accept = accept || ''; f.onchange = async () => { if (f.files[0]) then(new Uint8Array(await f.files[0].arrayBuffer()), f.files[0].name); }; f.click(); }
/* a debounced sender per key: the last value goes, ~25 a second at most */
const pend = {};
function soon(key, fn, ms = 40) { clearTimeout(pend[key]); pend[key] = setTimeout(fn, ms); }
/* the names the FM-1 knows: its sounds (kind, engine, name), its kits, yours, the kinds */
const NAMES = { presets: [], kits: [], user: [], kinds: [] };
async function readNames() {
  for (const [kind, key] of [[0, 'presets'], [1, 'kits'], [2, 'user'], [3, 'kinds']]) {
    const list = []; let first = 0, total = 1;
    while (first < total) { const b = await ask(ED.NAMES, [kind, first & 255, first >> 8]); total = u16(b, 3); const n = b[5]; if (!n) break;
      for (let i = 0; i < n; i++) { const o = 6 + 15 * i; list.push({ i: first + i, kind: b[o], eng: b[o + 1], used: b[o], name: str(b, o + 2, 13) }); } first += n; }
    NAMES[key] = list;
  }
}
/* ---------------------------------------------------------------- the device's files and patterns */
async function fileRead(kind, slot, onPart) {             /* kind 0 a project's place, 1 your sounds, 2 the project now */
  let off = 0, total = 1, crc = 0, buf = null; liveBusy++;
  try {
    while (off < total) {
      const b = await ask(ED.READ, [kind, slot, ...w32(off), ...w16(384)]);
      total = u32(b, 6); crc = u32(b, 10); if (!buf) buf = new Uint8Array(total);
      const data = b.slice(14); if (!data.length && off < total) throw new Error('short');
      buf.set(data, off); off += data.length; if (onPart) onPart(off / total);
    }
  } finally { liveBusy--; }
  if (crc32(buf) !== crc) throw new Error(L('err')[4]);
  return buf;
}
async function fileWrite(kind, slot, data, onPart) {
  liveBusy++;
  try {
    /* (3.3.5) each request asked again while the FM-1 answers busy: a project goes to the flash as it comes */
    await retryBusy(() => ask(ED.WBEGIN, [kind, slot, ...w32(data.length)]), 80);
    for (let off = 0; off < data.length; off += 384) { await retryBusy(() => ask(ED.WDATA, [...w32(off), ...data.slice(off, off + 384)]), 80); if (onPart) onPart(off / data.length); }
    await retryBusy(() => ask(ED.WEND, w32(crc32(data))), 80);
  } finally { liveBusy--; }
}
/* the FM-1 busy saving what was just sent: again until it is done */
async function retryBusy(fn, tries = 40) {
  for (let i = 0; ; i++) { try { return await fn(); } catch (e) { if (i >= tries || !/busy|occupé/i.test(e.message)) throw e; await sleep(250); } }
}
/* a pattern: its notes (tick, len, note, vel, cond, ratchet) and, protocol 2, its locks */
async function patRead(t, pt = 255) {
  const P = { t, pt, steps: 16, bpm: 120, notes: [], locks: [] }; let first = 0, total = 1;
  const wide = P2() ? 1 : 0, w = wide ? 8 : 6;
  while (first < total) {
    const b = await ask(ED.PATTERN, [t, pt, first & 255, first >> 8, wide]);
    P.pt = b[1]; P.steps = u16(b, 2); P.bpm = u16(b, 4) || P.bpm; total = u16(b, 6); const cnt = b[10];
    for (let i = 0; i < cnt; i++) { const o = 11 + w * i; P.notes.push({ at: u16(b, o), len: u16(b, o + 2), note: b[o + 4], vel: b[o + 5], cond: wide ? b[o + 6] : 0, rat: wide ? b[o + 7] : 1 }); }
    first += cnt;
    if (wide && first >= total) { let o = 11 + w * cnt; const nl = b[o++] || 0; for (let i = 0; i < nl; i++, o += 3) P.locks.push({ step: b[o], param: b[o + 1], value: b[o + 2] }); }
    if (!cnt) break;
  }
  return P;
}
async function patWrite(t, pt, steps, notes) {
  const ns = notes.filter(n => n.at < steps * 96).sort((a, b) => a.at - b.at || a.note - b.note).slice(0, 128); let i = 0, tot = 0;
  const wide = P2(), w = wide ? 8 : 6, per = wide ? 60 : 80;
  do {
    const part = ns.slice(i, i + per), raw = [t, pt, i & 255, i >> 8, steps & 255, steps >> 8, part.length];
    for (const n of part) { raw.push(n.at & 255, n.at >> 8, n.len & 255, n.len >> 8, n.note & 127, clamp(n.vel, 1, 127)); if (wide) raw.push(n.cond || 0, n.rat || 1); }
    const b = await ask(wide ? ED.NOTES2 : ED.NOTES, raw); tot = u16(b, 2); i += per;
  } while (i < ns.length);
  return tot;
}
async function locksWrite(t, pt, locks) {
  const raw = [t, pt, Math.min(32, locks.length)]; for (const l of locks.slice(0, 32)) raw.push(l.step, l.param, l.value);
  return ask(ED.LOCKS, raw);
}
const SEQ_SLIDE = 0x10, SEQ_ACCENT = 0x20;
const LK_MIX = ['LEVEL', 'PAN', 'CHO', 'DLY', 'REV', 'FILT'];                 /* locks 48..53 */
/* a step's condition, as words */
function condName(c) {
  if (!c) return L('always'); if (c === 1) return 'FILL'; if (c === 2) return '!FILL';
  if ((c & 0xC0) === 0x40) return (c & 15) * 10 + '%';
  if (c & 0x80) { const x = c & 0x7F, B = x >> 3, A = (x & 7) + 1; return A + ':' + B; }
  return '?';
}
const CONDS = [0, 1, 2, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x80 + 16, 0x80 + 17, 0x80 + 24, 0x80 + 25, 0x80 + 26, 0x80 + 32, 0x80 + 33, 0x80 + 34, 0x80 + 35];
words({ always: 'always' }, { always: 'toujours' });
