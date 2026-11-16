/* Samples: audio files into USR1-4 (SLOOP's "FSMP" slots, IMA ADPCM at 22.05 kHz, where SLOOP 2.8 keeps them),
 * heard, trimmed, looped first. Projects: the one project of the FM-1 (3.3.5), file out and in; it and your sounds in one backup. */
words({
  smpTitle: 'Your samples (USR1-4)', smpHelp: 'An instrument (one sound on every key, its root note), a kit (up to 16 files, one per pad, in the pads\' order) or a BREAK cut into slices for SLICE. A slot holds 80 KB: about 7 s at 22 kHz, 10 s at 16 kHz, 14 s at 11 kHz (QUALITY AUTO: the best that fits).\nInstrument: drag START and END (and LOOP) on the sound; ▶ hears the part kept.\nBREAK: START and END around the bars, then AUTO (a slice per hit), GRID (equal slices) or by hand: double-click adds a cut or takes one out, drag one to move it, click a slice (or its number) to hear it; SNAP puts the cuts on the 16ths of the bars. Up to 16 slices. Once sent, on the track you choose: a synth track plays it with SLICE (its SRC this slot; slice 1 on the lowest key), the drum track as one of YOUR KITS, named by the sample (slice 1 on the first pad, up to 16; on the FM-1: SOUNDS > YOUR KITS, to find it again): the break as the pattern playing, the tempo of the break. In SLOOP 2.8 too, the slices are the 16 pads.\n● RECORD takes the computer\'s audio input. The slot is written where SLOOP 2.8 keeps it: it reads it too (SLOOP 3 plays every quality; for SLOOP 2.8, keep 22 kHz).',
  smpChoose: 'Choose audio files…', smpSend: 'Send to the FM-1', smpErase: 'Empty the slot', slot: 'slot', mode: 'as', modes: ['an instrument', 'a kit (16 pads)', 'a BREAK (slices)'], loop: 'loop', loops: ['no loop', 'loop'],
  quality: 'quality', quals: ['AUTO', '22 kHz', '16 kHz', '11 kHz'], smpRec: '● RECORD', smpRecStop: '■ STOP', noMic: 'No audio input: ', rev: 'REVERSE', wav: '⇩ WAV',
  brkAuto: 'AUTO', brkUpTo: 'up to', brkGrid: 'GRID', brkSnap: 'SNAP 1/16', brkClear: 'CLEAR', bars: 'bars', slicesN: ' slices', max16: '16 slices at most.',
  after: 'Once sent:', onTrk: 'put it on', slcTag: ' · SLICE', padTag: ' · pads (kit)', kitWord: 'kit', patOn: 'the break as its pattern', tempoOn: 'its tempo', surePat: 'And replace the notes of the pattern playing on {t}?', brkDone: 'Ready: {t} plays the break ({how}, USR{k}): press PLAY.',
  root: 'root note', sname: 'name', norm: 'normalize', smpSure: 'Replace slot USR{k} on the FM-1?', smpDone: 'Sample sent: USR', tooLong: 'Too long: cut at ', sec: ' s', emptySlot: 'empty',
  zones: ' zone(s)', start: 'START', end: 'END', lstart: 'LOOP', hearIt: '▶ hear', stopIt: '■', files: ' file(s)', noAudio: 'This file could not be read as audio: ',
  projTitle: 'Your project', projHelp: 'The FM-1 keeps one project, at its fullest (8 patterns of up to 128 steps on every track, each its own sound). Keep the others here, as files.\n⇩ FILE: the project saved on the FM-1. ⇧ FILE: a file in its place (what was there is replaced), then opened. OPEN: the saved one again (what is not saved is lost).\nTHE PROJECT NOW: as it is, even unsaved. BACKUP: the project and your sounds in one file; RESTORE writes them back.',
  cur: 'saved', used: 'saved', emptyP: 'nothing saved yet', dl: '⇩ file', up: '⇧ file', openP: 'OPEN', now: '⇩ THE PROJECT NOW', backup: 'BACKUP ALL', restore: 'RESTORE…',
  sureP: 'Replace the project on the FM-1 by this file? Keep it first as a file (⇩) if you want it again.', got: 'Saved: ', wrote: 'Sent: the FM-1 saved it and opened it.', opened: 'Opened: the saved project', bkDone: 'Backup: ', bkBad: 'Not a SLOOP 3 backup (.s3b).',
  sureBk: 'Write this backup on the FM-1: its project and your sounds? What is there now is replaced.', restored: 'Restored: ',
  oldBk: 'A backup of before 3.3.5, with {n} projects: {list}. The FM-1 keeps one now. Which one goes on it? (the others stay in the file)', oneP: 'PROJECT'
}, {
  smpTitle: 'Tes échantillons (USR1-4)', smpHelp: 'Un instrument (un son sur toutes les touches, sa note de base), un kit (jusqu\'à 16 fichiers, un par pad, dans l\'ordre des pads) ou un BREAK coupé en tranches pour SLICE. Une case garde 80 Ko : environ 7 s à 22 kHz, 10 s à 16 kHz, 14 s à 11 kHz (QUALITÉ AUTO : la meilleure qui tient).\nInstrument : glisse START et END (et LOOP) sur le son ; ▶ fait entendre la partie gardée.\nBREAK : START et END autour des mesures, puis AUTO (une tranche par coup), GRILLE (des tranches égales) ou à la main : double-clic ajoute une coupe ou l\'enlève, glisse-la pour la déplacer, clique une tranche (ou son numéro) pour l\'entendre ; CALER met les coupes sur les doubles-croches des mesures. Jusqu\'à 16 tranches. Une fois envoyé, sur la piste que tu choisis : une piste de synthé le joue avec SLICE (son SRC cette case ; la tranche 1 sur la touche la plus grave), la piste de batterie comme un de TES KITS, au nom de l\'échantillon (la tranche 1 sur le premier pad, jusqu\'à 16 ; sur le FM-1 : SOUNDS > YOUR KITS, pour le retrouver) : le break comme pattern en cours, le tempo du break. Dans SLOOP 2.8 aussi, les tranches sont les 16 pads.\n● ENREGISTRER prend l\'entrée audio de l\'ordinateur. La case est écrite là où SLOOP 2.8 la garde : il la lit aussi (SLOOP 3 joue toutes les qualités ; pour SLOOP 2.8, garde 22 kHz).',
  smpChoose: 'Choisir des fichiers audio…', smpSend: 'Envoyer au FM-1', smpErase: 'Vider la case', slot: 'case', mode: 'en', modes: ['un instrument', 'un kit (16 pads)', 'un BREAK (tranches)'], loop: 'boucle', loops: ['sans boucle', 'en boucle'],
  quality: 'qualité', quals: ['AUTO', '22 kHz', '16 kHz', '11 kHz'], smpRec: '● ENREGISTRER', smpRecStop: '■ STOP', noMic: 'Pas d\'entrée audio : ', rev: 'INVERSER', wav: '⇩ WAV',
  brkAuto: 'AUTO', brkUpTo: 'jusqu\'à', brkGrid: 'GRILLE', brkSnap: 'CALER 1/16', brkClear: 'EFFACER', bars: 'mesures', slicesN: ' tranches', max16: '16 tranches au plus.',
  after: 'Une fois envoyé :', onTrk: 'le mettre sur', slcTag: ' · SLICE', padTag: ' · pads (kit)', kitWord: 'kit', patOn: 'le break comme pattern', tempoOn: 'son tempo', surePat: 'Et remplacer les notes du pattern en cours sur {t} ?', brkDone: 'Prêt : {t} joue le break ({how}, USR{k}) : appuie sur PLAY.',
  root: 'note de base', sname: 'nom', norm: 'normaliser', smpSure: 'Remplacer la case USR{k} du FM-1 ?', smpDone: 'Échantillon envoyé : USR', tooLong: 'Trop long : coupé à ', sec: ' s', emptySlot: 'vide',
  zones: ' zone(s)', start: 'START', end: 'END', lstart: 'LOOP', hearIt: '▶ écouter', stopIt: '■', files: ' fichier(s)', noAudio: 'Ce fichier n\'a pas pu être lu comme de l\'audio : ',
  projTitle: 'Ton projet', projHelp: 'Le FM-1 garde un projet, au plus plein (8 patterns jusqu\'à 128 pas sur chaque piste, chacun avec son son). Garde les autres ici, en fichiers.\n⇩ FICHIER : le projet sauvé sur le FM-1. ⇧ FICHIER : un fichier à sa place (ce qui y était est remplacé), puis ouvert. OUVRIR : le projet sauvé à nouveau (ce qui n\'est pas sauvé est perdu).\nLE PROJET MAINTENANT : tel qu\'il est, même non sauvé. SAUVEGARDE : le projet et tes sons dans un fichier ; RESTAURER les réécrit.',
  cur: 'sauvé', used: 'sauvé', emptyP: 'rien de sauvé encore', dl: '⇩ fichier', up: '⇧ fichier', openP: 'OUVRIR', now: '⇩ LE PROJET MAINTENANT', backup: 'TOUT SAUVEGARDER', restore: 'RESTAURER…',
  sureP: 'Remplacer le projet du FM-1 par ce fichier ? Garde-le d\'abord en fichier (⇩) si tu le veux encore.', got: 'Enregistré : ', wrote: 'Envoyé : le FM-1 l\'a sauvé et ouvert.', opened: 'Ouvert : le projet sauvé', bkDone: 'Sauvegarde : ', bkBad: 'Pas une sauvegarde SLOOP 3 (.s3b).',
  sureBk: 'Écrire cette sauvegarde sur le FM-1 : son projet et tes sons ? Ce qui y est maintenant est remplacé.', restored: 'Restauré : ',
  oldBk: 'Une sauvegarde d\'avant 3.3.5, avec {n} projets : {list}. Le FM-1 n\'en garde plus qu\'un. Lequel y va ? (les autres restent dans le fichier)', oneP: 'PROJET'
});
/* ---------------------------------------------------------------- samples */
const SM = { slots: null, slot: 0, mode: 0, loop: 0, root: 60, name: 'SAMPLE', norm: true, files: [], x: null, a: 0, b: 0, ls: 0, src: null,
  cuts: [], bars: 1, auto: 16, grid: 16, quality: 0, tgt: 0, doTrack: true, doPat: true, doTempo: true, playing: -1, ctx: null, rec: null };
async function readSlots() { try { const b = await ask(ED.SAMPLE, [0]); SM.slots = []; for (let i = 0; i < b[1]; i++) { const o = 3 + 11 * i; SM.slots.push({ nz: b[o], name: str(b, o + 1, 8), kb: u16(b, o + 9) }); } drawSlots(); } catch (e) { fail(e); } }
function drawSlots() {
  const E = SM.slotsEl; if (!E) return; E.innerHTML = '';
  for (let k = 0; k < 4; k++) { const s = SM.slots && SM.slots[k], d = el('div', 'slot' + (k === SM.slot ? ' on' : '')); d.style.cursor = 'pointer';
    d.append(el('b', '', 'USR' + (k + 1) + ' '), el('span', 'dim', s && s.nz ? s.name + ' · ' + s.nz + L('zones') + ' · ' + s.kb + ' KB' : L('emptySlot'))); d.onclick = () => { SM.slot = k; drawSlots(); }; E.appendChild(d); }
}
const STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767];
const IDX = [-1, -1, -1, -1, 2, 4, 6, 8];
function ima(x, ls) {                                          /* as tools/mksamples.py: the state kept at the loop start */
  let pred = 0, idx = 0, st = [0, 0]; const nib = [];
  for (let n = 0; n < x.length; n++) {
    if (n === ls) st = [pred, idx];
    const v = clamp(Math.round(x[n] * 32767), -32768, 32767), step = STEP[idx]; let d = v - pred, code = 0;
    if (d < 0) { code = 8; d = -d; }
    let vd = step >> 3;
    if (d >= step) { code |= 4; d -= step; vd += step; }
    if (d >= step >> 1) { code |= 2; d -= step >> 1; vd += step >> 1; }
    if (d >= step >> 2) { code |= 1; vd += step >> 2; }
    pred = clamp(code & 8 ? pred - vd : pred + vd, -32768, 32767); idx = clamp(idx + IDX[code & 7], 0, 88); nib.push(code);
  }
  if (nib.length & 1) nib.push(0);
  const o = new Uint8Array(nib.length / 2); for (let i = 0; i < o.length; i++) o[i] = nib[2 * i] | nib[2 * i + 1] << 4;
  return { data: o, st };
}
async function decodeTo(file, rate, norm) {
  const ab = await file.arrayBuffer(), ac = new (window.OfflineAudioContext || window.webkitOfflineAudioContext)(1, 1, rate);
  const buf = await ac.decodeAudioData(ab), n = Math.ceil(buf.duration * rate), oc = new OfflineAudioContext(1, Math.max(1, n), rate);
  const src = oc.createBufferSource(); src.buffer = buf; src.connect(oc.destination); src.start();
  const r = await oc.startRendering(); const x = r.getChannelData(0); let pk = 0; for (const v of x) pk = Math.max(pk, Math.abs(v));
  return pk > 1 || (norm && pk > 0.001) ? x.map(v => v / pk * (norm ? 0.98 : 1)) : x;
}
const GM_IN = [36, 35, 38, 39, 42, 46, 44, 37, 40, 43, 48, 49, 51, 70, 63, 56];
/* 3.3.6: the samples, more complete: a break cut into slices for SLICE (AUTO on its hits, a GRID, or by hand), its bars
 * and tempo, a quality that fits long sounds (22, 16 or 11 kHz), the audio input recorded, REVERSE, a WAV out; once
 * sent, SLICE on a track, the break as its pattern, the tempo. A break is sent as a kit: a zone a slice, each its own
 * sound (SLOOP 2.8 and the drum track play them on the pads; SLICE plays zone k as slice k). */
const RATES = [22050, 16000, 11025], SMAX = (0x14000 - 512) * 2 - 64, CUTMIN = 256;
const bounds = () => [SM.a, ...SM.cuts, SM.b];
const partDur = () => (SM.b - SM.a) / 22050;
const bpmOf = () => SM.bars * 240 / partDur();
function barsFor(dur) { let best = 1, bd = 1e9; for (const n of [1, 2, 4, 8]) { const bpm = n * 240 / dur; if (bpm < 60 || bpm > 200) continue; const d = Math.abs(Math.log(bpm / 112)); if (d < bd) { bd = d; best = n; } } return best; }
function normCuts() {
  const c = SM.cuts.map(v => Math.round(v)).filter(v => v >= SM.a + CUTMIN && v <= SM.b - CUTMIN).sort((p, q) => p - q), o = [];
  for (const v of c) if (!o.length || v - o[o.length - 1] >= CUTMIN) o.push(v);
  SM.cuts = o.slice(0, 15);
}
/* AUTO: the hits (a rise of the bright part, or of the whole), the strongest first, 70 ms apart at least */
function autoCuts(max) {
  const x = SM.x, a = SM.a, b = SM.b, hop = 128, W = 512, nf = Math.floor((b - a - W) / hop);
  SM.cuts = []; if (nf < 8) return;
  const hf = new Float32Array(nf), lo = new Float32Array(nf), odf = new Float32Array(nf);
  for (let f = 0; f < nf; f++) { let eh = 0, el2 = 0; const s = a + f * hop; for (let i = s + 1; i < s + W; i++) { const d = x[i] - x[i - 1]; eh += d * d; el2 += x[i] * x[i]; } hf[f] = Math.log(eh + 1e-7); lo[f] = Math.log(el2 + 1e-7); }
  let mx = 0;
  for (let f = 3; f < nf; f++) { odf[f] = Math.max(0, hf[f] - (hf[f - 1] + hf[f - 2] + hf[f - 3]) / 3) + 0.5 * Math.max(0, lo[f] - (lo[f - 1] + lo[f - 2] + lo[f - 3]) / 3); mx = Math.max(mx, odf[f]); }
  const gap = Math.round(0.07 * 22050 / hop), cand = [];
  for (let f = 3; f < nf; f++) { const v = odf[f]; if (v < mx * 0.15) continue; let pk = true; for (let k = -3; k <= 3; k++) if (k && f + k >= 0 && f + k < nf && odf[f + k] > v) pk = false; if (pk) cand.push([f, v]); }
  cand.sort((p, q) => q[1] - p[1]); const keep = [];
  for (const [f] of cand) { if (keep.length >= max - 1) break; if (keep.some(g => Math.abs(g - f) < gap)) continue; keep.push(f); }
  SM.cuts = keep.map(f => onsetIn(a + f * hop, a + f * hop + W)); normCuts();
}
function onsetIn(s, e) {                                   /* the hit in a window: its first sharp step, a little before, at a zero */
  const x = SM.x; let pk = 0; for (let i = s + 1; i < e; i++) pk = Math.max(pk, Math.abs(x[i] - x[i - 1]));
  let i = s + 1; while (i < e && Math.abs(x[i] - x[i - 1]) < pk * 0.5) i++;
  i = Math.max(SM.a, i - 44); for (let k = 0; k < 66 && i > SM.a + 1; k++, i--) if (x[i] * x[i - 1] <= 0) break;
  return i;
}
function gridCuts(n) { SM.cuts = []; for (let i = 1; i < n; i++) SM.cuts.push(SM.a + (SM.b - SM.a) * i / n); normCuts(); }
function snapCuts() { const st = (SM.b - SM.a) / (SM.bars * 16); SM.cuts = SM.cuts.map(c => SM.a + Math.round((c - SM.a) / st) * st); normCuts(); }
function rateFor(n) { if (SM.quality) return RATES[SM.quality - 1]; for (const r of RATES) if (n * r / 22050 <= SMAX) return r; return RATES[2]; }
function showInfo() {
  if (!SM.info) return; if (!SM.x) { SM.info.textContent = ''; return; }
  const n = SM.mode === 1 ? SM.x.length : SM.b - SM.a, r = rateFor(n), kb = Math.ceil(n * r / 22050 / 2 / 1024);
  let t = (SM.files[0] ? SM.files[0].name + ' · ' : '') + (n / 22050).toFixed(2) + L('sec') + ' · ' + (r / 1000).toFixed(r % 1000 ? 2 : 0) + ' kHz · ' + Math.min(999, kb) + ' / 80 KB';
  if (n * r / 22050 > SMAX) t += ' · ' + L('tooLong') + (SMAX / r).toFixed(1) + L('sec');
  if (SM.mode === 2) t += ' · ' + (SM.cuts.length + 1) + L('slicesN') + ' · ' + bpmOf().toFixed(1) + ' BPM';
  SM.info.textContent = t; if (SM.bpmEl) SM.bpmEl.textContent = '≈ ' + bpmOf().toFixed(1) + ' BPM';
}
function redraw() { drawWave(); drawPads(); showInfo(); }
function drawSamples() {
  const P = $('tab_samples'); P.innerHTML = '';
  const c = card(P, L('smpTitle'), '', L('smpHelp'), 'wide');
  SM.slotsEl = el('div', 'slots'); c.appendChild(SM.slotsEl); drawSlots();
  const r = el('div', 'tools');
  const md = select(L('modes').map((t, i) => [i, t]), SM.mode, v => { SM.mode = +v; if (SM.mode === 2 && SM.x && !SM.cuts.length) { SM.bars = SM.bars || barsFor(partDur()); autoCuts(SM.auto); } drawSamples(); });
  const lp = select(L('loops').map((t, i) => [i, t]), SM.loop, v => { SM.loop = +v; drawWave(); });
  const rt = el('input', 'num'); rt.type = 'number'; rt.min = 0; rt.max = 127; rt.value = SM.root; rt.onchange = () => SM.root = clamp(+rt.value || 60, 0, 127);
  const nm = el('input', 'txt'); nm.maxLength = 8; nm.value = SM.name; nm.style.width = '100px'; nm.oninput = () => SM.name = nm.value;
  const nl = el('label', 'dim'); const nc = el('input'); nc.type = 'checkbox'; nc.checked = SM.norm; nc.onchange = () => { SM.norm = nc.checked; if (SM.files.length) loadFiles(SM.files); }; nl.append(nc, ' ' + L('norm'));
  const q = select(L('quals').map((t, i) => [i, t]), SM.quality, v => { SM.quality = +v; showInfo(); });
  const rb = button(SM.rec ? L('smpRecStop') : L('smpRec'), () => recSample(rb), SM.rec ? 'red on' : 'red');
  r.append(button(L('smpChoose'), () => { const f = $('files'); f.value = ''; f.onchange = () => loadFiles([...f.files]); f.click(); }, 'pri'), rb, el('span', 'dim', L('mode')), md);
  if (SM.mode === 0) r.append(lp, el('span', 'dim', L('root')), rt);
  r.append(el('span', 'dim', L('sname')), nm, nl, el('span', 'dim', L('quality')), q);
  c.appendChild(r);
  SM.info = el('div', 'dim'); c.appendChild(SM.info);
  if (SM.mode === 2) {                                    /* the break's tools */
    const bt = el('div', 'tools'); bt.style.marginTop = '6px';
    const au = select([8, 12, 16].map(n => [n, L('brkUpTo') + ' ' + n]), SM.auto, v => SM.auto = +v);
    const gr = select([4, 8, 16].map(n => [n, String(n)]), SM.grid, v => SM.grid = +v);
    const ba = select([1, 2, 4, 8].map(n => [n, n + ' ' + L('bars')]), SM.bars || 1, v => { SM.bars = +v; showInfo(); });
    SM.bpmEl = el('span', 'dim');
    bt.append(button(L('brkAuto'), () => { if (SM.x) { autoCuts(SM.auto); redraw(); } }, 'sm'), au, button(L('brkGrid'), () => { if (SM.x) { gridCuts(SM.grid); redraw(); } }, 'sm'), gr,
      button(L('brkSnap'), () => { if (SM.x) { snapCuts(); redraw(); } }, 'sm'), button(L('brkClear'), () => { SM.cuts = []; redraw(); }, 'sm'), el('span', 'dim', '·'), ba, SM.bpmEl);
    c.appendChild(bt);
  }
  const cv = el('canvas', 'roll'); cv.style.height = '150px'; cv.style.marginTop = '8px'; c.appendChild(cv); SM.cv = cv; waveEvents(cv);
  SM.padsEl = el('div', 'row'); SM.padsEl.style.cssText = 'flex-wrap:wrap;gap:4px;margin-top:6px'; c.appendChild(SM.padsEl);
  const r2 = el('div', 'tools'); SM.prog = el('span', 'dim');
  r2.append(button(L('hearIt'), hearPart, 'sm'), button(L('stopIt'), stopSample, 'sm'), button(L('rev'), reversePart, 'sm'), button(L('wav'), saveWav, 'sm'),
    button(L('smpSend'), uploadSamples, 'pri'), button(L('smpErase'), eraseSlot, 'red'), SM.prog);
  c.appendChild(r2);
  if (SM.mode === 2) {                                    /* once sent */
    const r3 = el('div', 'tools'); const ck = (k, t) => { const l = el('label', 'dim'), i = el('input'); i.type = 'checkbox'; i.checked = SM[k]; i.onchange = () => SM[k] = i.checked; l.append(i, ' ' + t); return l; };
    const ts = select(SHOWN.map(t => [t, trkName(t) + (t === DRUM_T ? L('padTag') : L('slcTag'))]), SM.tgt, v => SM.tgt = +v);   /* (3.3.6: a synth track: SLICE; the drums: a kit, a slice a pad) */
    r3.append(el('span', 'dim', L('after')), ck('doTrack', L('onTrk')), ts, ck('doPat', L('patOn')), ck('doTempo', L('tempoOn')));
    c.appendChild(r3);
  }
  if (info) readSlots(); redraw();
}
async function loadFiles(files) {
  SM.files = files; SM.x = null; SM.cuts = []; if (!files.length) return;
  try { SM.x = await decodeTo(files[0], 22050, SM.norm); SM.a = 0; SM.b = SM.x.length; SM.ls = Math.floor(SM.x.length * 0.1);
    if (SM.name === 'SAMPLE' || !SM.name) SM.name = (files[0].name || 'SAMPLE').replace(/\.[^.]*$/, '').toUpperCase().replace(/[^A-Z0-9 ]/g, '').slice(0, 8) || 'SAMPLE';
    SM.bars = barsFor(partDur()); if (SM.mode === 2) autoCuts(SM.auto);
    drawSamples();
  } catch (e) { log(L('noAudio') + (files[0].name || ''), 'bad'); }
}
function drawWave() {
  const cv = SM.cv; if (!cv) return; const W = cv.clientWidth || 800, H = 150, dpr = (typeof devicePixelRatio !== 'undefined' && devicePixelRatio) || 1;
  cv.width = W * dpr; cv.height = H * dpr; const g = cv.getContext('2d'); g.setTransform(dpr, 0, 0, dpr, 0, 0);
  g.fillStyle = '#09080e'; g.fillRect(0, 0, W, H); const x = SM.x; if (!x) return;
  const n = x.length, px = i => i / n * W;
  if (SM.mode === 2) { const bd = bounds(); for (let k = 0; k + 1 < bd.length; k++) { g.fillStyle = k === SM.playing ? '#ffd23c30' : k & 1 ? '#ffffff08' : '#ffffff14'; g.fillRect(px(bd[k]), 0, px(bd[k + 1]) - px(bd[k]), H); } }
  else if (!SM.mode) { g.fillStyle = '#ffffff10'; g.fillRect(px(SM.a), 0, px(SM.b) - px(SM.a), H); }
  g.strokeStyle = '#50a0ff'; g.beginPath();
  for (let i = 0; i < W; i++) { let mn = 1, mx = -1; const s0 = Math.floor(i / W * n), s1 = Math.max(s0 + 1, Math.floor((i + 1) / W * n)); for (let j = s0; j < s1; j++) { if (x[j] < mn) mn = x[j]; if (x[j] > mx) mx = x[j]; } g.moveTo(i + 0.5, H / 2 - mx * H / 2); g.lineTo(i + 0.5, H / 2 - mn * H / 2); }
  g.stroke();
  const mark = (i, c, t) => { g.fillStyle = c; g.fillRect(px(i) - 1, 0, 2, H); g.font = '11px system-ui'; g.fillText(t, px(i) + 4, 12); };
  if (SM.mode !== 1) { mark(SM.a, '#5ef08a', L('start')); mark(SM.b, '#ff6b6b', L('end')); }
  if (!SM.mode && SM.loop) mark(SM.ls, '#ffd23c', L('lstart'));
  if (SM.mode === 2) { SM.cuts.forEach(c => { g.fillStyle = '#ffd23c'; g.fillRect(px(c) - 1, 0, 2, H); }); g.font = 'bold 11px system-ui'; g.fillStyle = '#ffd23c';
    const bd = bounds(); for (let k = 0; k + 1 < bd.length; k++) g.fillText(String(k + 1), px(bd[k]) + 4, H - 6); }
}
function sliceAt(i) { const bd = bounds(); for (let k = 0; k + 1 < bd.length; k++) if (i >= bd[k] && i < bd[k + 1]) return k; return -1; }
function waveEvents(cv) {
  let which = null;
  const at = ev => { const rc = cv.getBoundingClientRect(); return { i: clamp(Math.round((ev.clientX - rc.left) / rc.width * SM.x.length), 0, SM.x.length), pxOf: v => v / SM.x.length * rc.width, x: ev.clientX - rc.left }; };
  cv.addEventListener('pointerdown', ev => { if (!SM.x || SM.mode === 1) return; const p = at(ev);
    const hs = [['a', SM.a], ['b', SM.b]]; if (!SM.mode && SM.loop) hs.push(['ls', SM.ls]); if (SM.mode === 2) SM.cuts.forEach((c, k) => hs.push([k, c]));
    const d = hs.map(([k, v]) => [k, Math.abs(p.pxOf(v) - p.x)]).sort((u, w) => u[1] - w[1]);
    if (SM.mode === 2 && d[0][1] > 8) { const k = sliceAt(p.i); if (k >= 0) playSlice(k); return; }   /* (between the marks: hear that slice) */
    which = d[0][0]; cv.setPointerCapture(ev.pointerId); move(ev); });
  const move = ev => { if (which === null) return; const i = at(ev).i;
    if (which === 'a') SM.a = Math.min(i, SM.b - 64); else if (which === 'b') SM.b = Math.max(i, SM.a + 64); else if (which === 'ls') SM.ls = i;
    else { const lo = which ? SM.cuts[which - 1] : SM.a, hi = which + 1 < SM.cuts.length ? SM.cuts[which + 1] : SM.b; SM.cuts[which] = clamp(i, lo + CUTMIN, hi - CUTMIN); }
    SM.ls = clamp(SM.ls, SM.a, SM.b - 32); drawWave(); showInfo(); };
  cv.addEventListener('pointermove', move);
  cv.addEventListener('pointerup', () => { if (which === 'a' || which === 'b') { normCuts(); redraw(); } which = null; });
  cv.addEventListener('dblclick', ev => { if (!SM.x || SM.mode !== 2) return; const p = at(ev);   /* double-click: a cut added there, or the one there taken out */
    const k = SM.cuts.findIndex(c => Math.abs(p.pxOf(c) - p.x) <= 8);
    if (k >= 0) SM.cuts.splice(k, 1); else if (SM.cuts.length < 15) { SM.cuts.push(p.i); normCuts(); } else log(L('max16'), 'bad');
    redraw(); });
}
function drawPads() {
  const E = SM.padsEl; if (!E) return; E.innerHTML = ''; if (SM.mode !== 2 || !SM.x) return;
  const bd = bounds(); for (let k = 0; k + 1 < bd.length; k++) { const b = button(String(k + 1), () => playSlice(k), 'sm' + (k === SM.playing ? ' on' : '')); b.title = ((bd[k + 1] - bd[k]) / 22050 * 1000).toFixed(0) + ' ms'; E.appendChild(b); }
}
function playBuf(part, loop, ls) {
  stopSample(); if (!SM.ctx) SM.ctx = new AudioContext(); const ac = SM.ctx, b = ac.createBuffer(1, Math.max(1, part.length), 22050); b.copyToChannel(part, 0);
  const s = ac.createBufferSource(); s.buffer = b; if (loop) { s.loop = true; s.loopStart = ls / 22050; s.loopEnd = part.length / 22050; }
  s.connect(ac.destination); s.start(); SM.src = s; return s;
}
function stopSample() { if (SM.src) try { SM.src.stop(); } catch (e) {} SM.src = null; }
function playSlice(k) { const bd = bounds(); if (k < 0 || k + 1 >= bd.length) return; SM.playing = k; const s = playBuf(SM.x.slice(bd[k], bd[k + 1])); s.onended = () => { if (SM.playing === k) { SM.playing = -1; drawWave(); drawPads(); } }; drawWave(); drawPads(); }
function hearPart() {
  if (!SM.x) return;
  if (SM.mode === 1) { playBuf(SM.x); return; }
  playBuf(SM.x.slice(SM.a, SM.b), !SM.mode && SM.loop, SM.ls - SM.a);
}
function reversePart() {
  if (!SM.x) return; const a = SM.mode === 1 ? 0 : SM.a, b = SM.mode === 1 ? SM.x.length : SM.b; SM.x.subarray(a, b).reverse();
  SM.cuts = SM.cuts.map(c => a + b - c); normCuts(); redraw();
}
function saveWav() {
  if (!SM.x) return; const p = SM.mode === 1 ? SM.x : SM.x.slice(SM.a, SM.b), n = p.length, f = new Uint8Array(44 + 2 * n), dv = new DataView(f.buffer);
  const s = (o, t) => { for (let i = 0; i < t.length; i++) f[o + i] = t.charCodeAt(i); };
  s(0, 'RIFF'); dv.setUint32(4, 36 + 2 * n, true); s(8, 'WAVEfmt '); dv.setUint32(16, 16, true); dv.setUint16(20, 1, true); dv.setUint16(22, 1, true);
  dv.setUint32(24, 22050, true); dv.setUint32(28, 44100, true); dv.setUint16(32, 2, true); dv.setUint16(34, 16, true); s(36, 'data'); dv.setUint32(40, 2 * n, true);
  for (let i = 0; i < n; i++) dv.setInt16(44 + 2 * i, clamp(Math.round(p[i] * 32767), -32768, 32767), true);
  save(f, (SM.name || 'SAMPLE').toLowerCase().replace(/\s+/g, '-') + '.wav', 'audio/wav');
}
async function recSample(btn) {                           /* the computer's audio input (a microphone, a line in), then as a file */
  if (SM.rec) { SM.rec.stop(); return; }
  try {
    const st = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } });
    const mr = new MediaRecorder(st), parts = [];
    mr.ondataavailable = e => { if (e.data && e.data.size) parts.push(e.data); };
    mr.onstop = () => { st.getTracks().forEach(t => t.stop()); SM.rec = null; SM.name = 'SAMPLE'; loadFiles([new File(parts, 'REC.webm', { type: mr.mimeType || 'audio/webm' })]); };
    mr.start(); SM.rec = mr; btn.textContent = L('smpRecStop'); btn.classList.add('on');
  } catch (e) { SM.rec = null; log(L('noMic') + (e.message || e), 'bad'); }
}
async function resample(x, r) {
  if (r === 22050 || !x.length) return x;
  const n = Math.max(1, Math.ceil(x.length * r / 22050)), oc = new OfflineAudioContext(1, n, r), b = oc.createBuffer(1, x.length, 22050); b.copyToChannel(x, 0);
  const s = oc.createBufferSource(); s.buffer = b; s.connect(oc.destination); s.start(); return (await oc.startRendering()).getChannelData(0);
}
async function uploadSamples() {
  const k = SM.slot, mode = SM.mode, lp = SM.loop, root = SM.root;
  if (!SM.files.length || !SM.x) return;
  if (!confirm(L('smpSure').replace('{k}', k + 1))) return;
  if (mode === 2 && SM.doPat && !confirm(L('surePat').replace('{t}', trkName(SM.tgt)))) return;
  const MAX = 0x14000 - 512, zones = [], blob = []; let used = 0; liveBusy++;
  try {
    /* what goes: each sound at 22.05 kHz first, then all at the rate that fits */
    let parts = [];
    if (mode === 1) { for (const f of SM.files.slice(0, 16)) parts.push(await decodeTo(f, 22050, SM.norm)); }
    else if (mode === 0) parts = [SM.x.slice(SM.a, SM.b)];
    else { const bd = bounds(); for (let j = 0; j + 1 < bd.length; j++) parts.push(SM.x.slice(bd[j], bd[j + 1])); }
    const total = parts.reduce((s, p) => s + p.length, 0), R = rateFor(total), rate16 = Math.round(R / 44100 * 65536);
    for (let i = 0; i < parts.length; i++) {
      let x = await resample(parts[i], R);
      const room = (MAX - used) * 2 - 2; if (room < 64) { log(L('tooLong') + (used * 2 / R).toFixed(1) + L('sec'), 'bad'); break; }
      if (x.length > room) { x = x.slice(0, room); log(L('tooLong') + ((used * 2 + room) / R).toFixed(1) + L('sec'), 'bad'); }
      const n = x.length, looped = !mode && lp ? 1 : 0, ls = looped ? clamp(Math.round((SM.ls - SM.a) * R / 22050), 0, n - 2) : n - 1, le = n - 1;
      const { data, st } = ima(x, ls), off = used;
      blob.push(data); used += data.length; if (used & 1) { blob.push(new Uint8Array(1)); used++; }
      const nt = mode ? GM_IN[i] : root;
      zones.push({ off, n, ls, le, rate: rate16, root16: nt * 16, pred: st[0], idx: st[1], lo: mode ? nt : 0, hi: mode ? nt : 127, looped });
    }
    const data = new Uint8Array(used); let o = 0; for (const b of blob) { data.set(b, o); o += b.length; }
    const h = new Uint8Array(480), dv = new DataView(h.buffer);
    h.set([0x46, 0x53, 0x4D, 0x50]); dv.setUint16(4, 1, true); h[6] = zones.length;
    const nm = (SM.name || 'SAMPLE').toUpperCase().slice(0, 8); for (let i = 0; i < nm.length; i++) h[8 + i] = nm.charCodeAt(i) & 127;
    dv.setUint32(16, data.length, true); dv.setUint32(20, crc32(data), true);
    zones.forEach((z, i) => { const b = 32 + 28 * i; dv.setUint32(b, z.off, true); dv.setUint32(b + 4, z.n, true); dv.setUint32(b + 8, z.ls, true); dv.setUint32(b + 12, z.le, true);
      dv.setUint32(b + 16, z.rate, true); dv.setInt16(b + 20, z.root16, true); dv.setInt16(b + 22, z.pred, true); h[b + 24] = z.idx; h[b + 25] = z.lo; h[b + 26] = z.hi; h[b + 27] = z.looped; });
    await retryBusy(() => ask(ED.SAMPLE, [1, k]), 80);
    for (let off = 0; off < data.length; off += 384) { await ask(ED.SAMPLE, [2, k, ...w32(off), ...data.slice(off, off + 384)]); SM.prog.textContent = Math.floor(off * 100 / data.length) + ' %'; }
    await ask(ED.SAMPLE, [3, k, ...h]);
    SM.prog.textContent = ''; log(L('smpDone') + (k + 1) + ' (' + zones.length + L('zones') + ', ' + Math.ceil(data.length / 1024) + ' KB, ' + (R / 1000).toFixed(R % 1000 ? 2 : 0) + ' kHz)', 'ok'); readSlots();
    if (mode === 2) await breakOnTrack(k, zones.length);
  } catch (e) { SM.prog.textContent = ''; fail(e); } finally { liveBusy--; }
}
/* the break sent: on a synth track, SLICE (USR SLICE, its SRC this slot); on the drum track, the slot as its kit (USR1-4:
 * slice k on pad k). Its tempo, its pattern (slice k where it was: key k for SLICE, pad k for the drums) */
async function breakOnTrack(k, nz) {
  const t = SM.tgt, bpm = bpmOf(), drums = t === DRUM_T;
  if (SM.doTrack && drums) {                               /* (one of YOUR KITS: named by the sample, found again in SOUNDS) */
    const v = UK_BASE + KIT_USR + k; await retryBusy(() => live(10, t, v & 255, v >> 8), 80);
  } else if (SM.doTrack) {
    if (!NAMES.presets.length) await retryBusy(() => readNames(), 80);
    const p = NAMES.presets.find(p => p.name === 'USR SLICE') || NAMES.presets.find(p => p.eng === ENGINES.indexOf('SLICE'));
    if (p) await retryBusy(() => live(10, t, p.i & 255, p.i >> 8), 80);
    await retryBusy(() => ask(ED.SET, [t, 0, ...w16(k + 1)]), 80);
  }
  if (SM.doTempo) { const bp = Math.round(bpm * 10); if (bp >= 300 && bp <= 3000) await retryBusy(() => live(13, bp & 255, bp >> 8, 255), 80); }
  if (SM.doPat) {
    const bd = bounds().slice(0, nz + 1), steps = Math.min(128, SM.bars * 16), tps = steps * 96 / (SM.b - SM.a), notes = [];
    for (let j = 0; j < nz; j++) { const at = Math.round((bd[j] - SM.a) * tps), nx = j + 1 < nz ? Math.round((bd[j + 1] - SM.a) * tps) : steps * 96;
      if (at < steps * 96) notes.push({ at, len: Math.max(6, nx - at - 2), note: drums ? j : 60 + j, vel: 100, cond: 0, rat: 1 }); }
    const P = await retryBusy(() => patRead(t, 255), 80); await retryBusy(() => patWrite(t, P.pt, steps, notes), 80);
  }
  if (SM.doTrack || SM.doPat) log(L('brkDone').replace('{t}', trkName(t)).replace('{how}', drums ? L('kitWord') : 'SLICE').replace('{k}', k + 1), 'ok');
}
async function eraseSlot() { const k = SM.slot; if (!confirm(L('smpSure').replace('{k}', k + 1))) return; try { await ask(ED.SAMPLE, [4, k]); readSlots(); } catch (e) { fail(e); } }
/* ---------------------------------------------------------------- projects */
async function refreshInfo() {
  const b = await ask(ED.INFO);
  info = { proto: b[0], ntrk: b[1], proj: b[3], used: u16(b, 4), us: u32(b, 6), ver: str(b, 10, 32) };
  $('s_ver').textContent = info.ver; $('s_proj').textContent = info.proj + 1;
  if ($('tab_projects').classList.contains('on')) drawProjects();
  return info;
}
async function dlFile(kind, slot, name) { try { log(name + '…'); const d = await fileRead(kind, slot); save(d, name); log(L('got') + name + ' (' + d.length + ' B)', 'ok'); } catch (e) { fail(e); } }
function drawProjects() {
  const P = $('tab_projects'); P.innerHTML = '';
  const c = card(P, L('projTitle'), '', L('projHelp'), 'wide');
  const r = el('div', 'tools');
  r.append(button(L('now'), () => dlFile(2, 0, 'sloop3-project-now.s3p')), button(L('backup'), backupAll, 'pri'), button(L('restore'), () => pick(restoreAll, '.s3b')));
  c.appendChild(r);
  const g = el('div', 'slots'); c.appendChild(g);
  {                                                       /* (3.3.5) one project, at its fullest */
    const used = info && info.used & 1;
    const d = el('div', 'slot on'); d.append(el('b', '', L('oneP') + ' '), el('span', 'dim', used ? L('used') : L('emptyP')), el('br'));
    const a = button(L('dl'), () => dlFile(0, 0, 'sloop3-project-' + new Date().toISOString().slice(0, 10) + '.s3p'), 'sm'); a.disabled = !info || !used;
    const b = button(L('up'), () => pick(async f => { if (!confirm(L('sureP'))) return; try { await fileWrite(0, 0, f); await retryBusy(() => refreshInfo(), 80); log(L('wrote'), 'ok'); } catch (e) { fail(e); } }, '.s3p'), 'sm'); b.disabled = !info;
    const o = button(L('openP'), async () => { try { await retryBusy(() => ask(ED.LOAD, [0]), 80); log(L('opened'), 'ok'); await refreshInfo(); } catch (e) { fail(e); } }, 'sm'); o.disabled = !info || !used;
    d.append(a, b, o); g.appendChild(d);
  }
}
/* a backup: "S3BK", 1, its count; then each: kind (0 a project, 1 your sounds), place, length (u32), the bytes */
async function backupAll() {
  try {
    const parts = [];
    if (info.used & 1) { log(L('oneP') + '…'); parts.push([0, 0, await retryBusy(() => fileRead(0, 0), 80)]); }
    try { parts.push([1, 0, await fileRead(1, 0)]); } catch (e) { /* no sounds of yours yet */ }
    let n = 8; for (const p of parts) n += 6 + p[2].length;
    const f = new Uint8Array(n); f.set([0x53, 0x33, 0x42, 0x4B, 1, parts.length, 0, 0]); let o = 8;
    for (const [k, s, d] of parts) { f.set([k, s, ...w32(d.length)], o); f.set(d, o + 6); o += 6 + d.length; }
    const name = 'sloop3-backup-' + new Date().toISOString().slice(0, 10) + '.s3b'; save(f, name); log(L('bkDone') + name + ' (' + parts.length + ')', 'ok');
  } catch (e) { fail(e); }
}
async function restoreAll(f) {
  if (f.length < 8 || f[0] !== 0x53 || f[1] !== 0x33 || f[2] !== 0x42 || f[3] !== 0x4B) { log(L('bkBad'), 'bad'); return; }
  const parts = []; let o = 8; for (let i = 0; i < f[5] && o + 6 <= f.length; i++) { const len = u32(f, o + 2); parts.push([f[o], f[o + 1], f.slice(o + 6, o + 6 + len)]); o += 6 + len; }
  const projs = parts.filter(p => p[0] === 0);
  if (projs.length > 1) {                                 /* (3.3.5) a backup of the 8 places: one of them goes on the FM-1 */
    const list = projs.map(p => p[1] + 1).join(', '), a = prompt(L('oldBk').replace('{n}', projs.length).replace('{list}', list), String(projs[projs.length - 1][1] + 1));
    const keep = projs.find(p => String(p[1] + 1) === String(a || '').trim()); if (!keep) return;
    for (let i = parts.length - 1; i >= 0; i--) if (parts[i][0] === 0 && parts[i] !== keep) parts.splice(i, 1);
  }
  if (!confirm(L('sureBk'))) return;
  try {
    for (const [k, s, d] of parts) { await fileWrite(k, k ? s : 0, d); log(L('restored') + (k ? L('libTitle') : L('oneP')), 'ok'); }
    await retryBusy(() => refreshInfo()); await retryBusy(() => readNames());
  } catch (e) { fail(e); }
}
