/* Your FM6 bank (B1-B27: DX7 voices) and your SYN kits (the drum synth's 4 kits), app/xbank.h, ED_BANK; your
 * colour theme (ED_THEME). As in the editor of 3.1.4, laid out again. */
words({
  fm6: 'Your FM6 bank (B1-B27)', syn: 'Your SYN kits (SYN1-4)', bread: 'Read from the FM-1', bload: 'Load DX7 voices (.syx)', fsave: 'Save the bank to the FM-1',
  fsyx: 'Save as .syx', put: 'Into the slot selected', fillall: 'Fill the slots from the one selected', bempty: 'empty', bclear: 'Empty this slot',
  bhear: 'Hear it', bsrc: ['nothing yet', 'SLOOP 2.8\'s', 'yours'], ysrc: ['the defaults', 'SLOOP 2.8\'s', 'yours'], fsaved: 'FM6 bank saved on the FM-1. There: turn PRESETS to the DX7 category (or EDIT 6/6, SOUNDS): the voices by name.',
  bplaced: ' voices put in B1 and on. Now: Save the bank to the FM-1.', bmore: ' (The file has more voices than the 27 slots: the rest is in the list, to put by hand.)', bunsaved: '● not saved on the FM-1 yet',
  ysaved: 'SYN kits saved on the FM-1.', ysave: 'Save the kits to the FM-1', crush: 'crush', kitN: 'SYN', btracks: ' track(s) play it',
  fhelp: 'Load a .syx: into an empty bank its voices go to B1-B27 at once; then Save. On the FM-1 they are in SOUNDS (turn PRESETS, or EDIT 6/6), category DX7, by name. Changes are heard at once on the tracks that play that slot (FM6, PTCH B1-B27); they stay only once saved. SLOOP 2.8\'s bank is never written: saving makes one of SLOOP 3\'s own.',
  yhelp: 'Each change is heard at once (the drum track on that SYN kit); ▶ plays the sound. They stay only once saved. SLOOP 2.8\'s kits are never written.',
  nosyx: 'Not DX7 voices: a 32-voice bank (4104 bytes) or a single voice (163 bytes).', bgot: ' voices in the file', op: 'OP', voices: 'voices of the file',
  theme: 'Your colour theme', themeHelp: 'Five shades, the background, the accent, a colour for each synth track: then HOME > SCREEN > COLOR (YOURS) on the FM-1, or COLOR in Settings.\nPREVIEW shows it here as the FM-1 would.',
  tget: 'Read from the FM-1', tsend: 'Send to the FM-1', tok: 'Theme sent: COLOR > YOURS.', tname: 'name', tlab: ['shade 1', 'shade 2', 'shade 3', 'shade 4', 'text', 'background', 'accent', 'track 1', 'track 2', 'track 3', 'track 4']
}, {
  fm6: 'Ta banque FM6 (B1-B27)', syn: 'Tes kits SYN (SYN1-4)', bread: 'Lire depuis le FM-1', bload: 'Charger des voix DX7 (.syx)', fsave: 'Enregistrer la banque sur le FM-1',
  fsyx: 'Enregistrer en .syx', put: 'Dans la case choisie', fillall: 'Remplir les cases depuis celle choisie', bempty: 'vide', bclear: 'Vider cette case',
  bhear: 'Écouter', bsrc: ['rien encore', 'celle de SLOOP 2.8', 'la tienne'], ysrc: ['ceux d\'origine', 'ceux de SLOOP 2.8', 'les tiens'], fsaved: 'Banque FM6 enregistrée sur le FM-1. Là-bas : tourne PRESETS jusqu\'à la catégorie DX7 (ou EDIT 6/6, SOUNDS) : les voix par leur nom.',
  bplaced: ' voix mises en B1 et suivantes. Maintenant : Enregistrer la banque sur le FM-1.', bmore: ' (Le fichier a plus de voix que les 27 cases : le reste est dans la liste, à placer à la main.)', bunsaved: '● pas encore enregistrée sur le FM-1',
  ysaved: 'Kits SYN enregistrés sur le FM-1.', ysave: 'Enregistrer les kits sur le FM-1', crush: 'crush', kitN: 'SYN', btracks: ' piste(s) la jouent',
  fhelp: 'Charge un .syx : dans une banque vide ses voix vont en B1-B27 d\'un coup ; puis Enregistrer. Sur le FM-1 elles sont dans SOUNDS (tourne PRESETS, ou EDIT 6/6), catégorie DX7, par leur nom. Les changements s\'entendent tout de suite sur les pistes qui jouent cette case (FM6, PTCH B1-B27) ; ils restent une fois enregistrés. La banque de SLOOP 2.8 n\'est jamais écrite : enregistrer crée celle de SLOOP 3.',
  yhelp: 'Chaque changement s\'entend tout de suite (la piste de batterie sur ce kit SYN) ; ▶ joue le son. Ils restent une fois enregistrés. Les kits de SLOOP 2.8 ne sont jamais écrits.',
  nosyx: 'Pas des voix DX7 : une banque de 32 voix (4104 octets) ou une voix seule (163 octets).', bgot: ' voix dans le fichier', op: 'OP', voices: 'voix du fichier',
  theme: 'Ton thème de couleurs', themeHelp: 'Cinq teintes, le fond, l\'accent, une couleur par piste de synthé : puis HOME > SCREEN > COLOR (YOURS) sur le FM-1, ou COULEUR dans Réglages.\nL\'APERÇU le montre ici comme le FM-1.',
  tget: 'Lire depuis le FM-1', tsend: 'Envoyer au FM-1', tok: 'Thème envoyé : COLOR > YOURS.', tname: 'nom', tlab: ['teinte 1', 'teinte 2', 'teinte 3', 'teinte 4', 'texte', 'fond', 'accent', 'piste 1', 'piste 2', 'piste 3', 'piste 4']
});
const L2 = L;
const XB = { FM6: 16 + 27 * 128, KIT: 4 + 16 * 22, SYN: 16 + 4 * (4 + 16 * 22) };
async function bankRead(which) {
  const len = which ? XB.SYN : XB.FM6, img = new Uint8Array(len);
  for (let off = 0; off < len;) { const b = await ask(14, [1, which, ...w16(off), ...w16(384)]); const m = b.length - 6; img.set(b.slice(6), off); off += m; if (!m) break; }
  return img;
}
/* an image with its header (app/xbank.h), written: begin, the bytes, end (the FM-1 answers once its sector is written) */
async function bankWrite(which, img) {
  img.set([0x53, 0x33, which ? 0x53 : 0x46, which ? 0x59 : 0x36, 1, 0, which ? 4 : 27, 0], 0);
  if (which) img.set([0, 0, 0, 0], 8);
  img.set(w32(crc32(img.slice(16))), 12);
  await ask(14, [2, which, ...w16(img.length)]);
  for (let off = 0; off < img.length; off += 380) await ask(14, [3, which, ...w16(off), ...img.slice(off, off + 380)]);
  await ask(14, [4, which]);
}
/* ---- DX7 voices: the packed 128 bytes (VMEM) <-> their values */
const OPF = [['R1', 99], ['R2', 99], ['R3', 99], ['R4', 99], ['L1', 99], ['L2', 99], ['L3', 99], ['L4', 99], ['BP', 99], ['LD', 99], ['RD', 99], ['LC', 3], ['RC', 3],
  ['RS', 7], ['AMS', 3], ['KVS', 7], ['OUT', 99], ['MODE', 1], ['CRS', 31], ['FINE', 99], ['DET', 14]];
const GLF = [['ALG', 31], ['FB', 7], ['SYNC', 1], ['TRSP', 48], ['PR1', 99], ['PR2', 99], ['PR3', 99], ['PR4', 99], ['PL1', 99], ['PL2', 99], ['PL3', 99], ['PL4', 99],
  ['LSPD', 99], ['LDLY', 99], ['LPMD', 99], ['LAMD', 99], ['LSYNC', 1], ['LWAVE', 5], ['PMS', 7]];
function vUnpack(b) {   /* op 0 = OP1 (stored last) */
  const v = { op: [], g: {} };
  for (let k = 0; k < 6; k++) { const o = (5 - k) * 17, x = {};
    for (let i = 0; i < 4; i++) { x['R' + (i + 1)] = b[o + i]; x['L' + (i + 1)] = b[o + 4 + i]; }
    x.BP = b[o + 8]; x.LD = b[o + 9]; x.RD = b[o + 10]; x.LC = b[o + 11] & 3; x.RC = b[o + 11] >> 2 & 3; x.RS = b[o + 12] & 7; x.DET = b[o + 12] >> 3 & 15;
    x.AMS = b[o + 13] & 3; x.KVS = b[o + 13] >> 2 & 7; x.OUT = b[o + 14]; x.MODE = b[o + 15] & 1; x.CRS = b[o + 15] >> 1 & 31; x.FINE = b[o + 16]; v.op.push(x); }
  const g = v.g; for (let i = 0; i < 4; i++) { g['PR' + (i + 1)] = b[102 + i]; g['PL' + (i + 1)] = b[106 + i]; }
  g.ALG = b[110] & 31; g.FB = b[111] & 7; g.SYNC = b[111] >> 3 & 1; g.LSPD = b[112]; g.LDLY = b[113]; g.LPMD = b[114]; g.LAMD = b[115];
  g.LSYNC = b[116] & 1; g.LWAVE = b[116] >> 1 & 7; g.PMS = b[116] >> 4 & 7; g.TRSP = b[117];
  v.name = String.fromCharCode(...b.slice(118, 128)).replace(/[\x00-\x1f\x7f-\xff]/g, ' ').trimEnd();
  return v;
}
function vPack(v) {
  const b = new Uint8Array(128), c = (x, m) => Math.max(0, Math.min(m, x | 0));
  for (let k = 0; k < 6; k++) { const o = (5 - k) * 17, x = v.op[k];
    for (let i = 0; i < 4; i++) { b[o + i] = c(x['R' + (i + 1)], 99); b[o + 4 + i] = c(x['L' + (i + 1)], 99); }
    b[o + 8] = c(x.BP, 99); b[o + 9] = c(x.LD, 99); b[o + 10] = c(x.RD, 99); b[o + 11] = c(x.LC, 3) | c(x.RC, 3) << 2; b[o + 12] = c(x.RS, 7) | c(x.DET, 14) << 3;
    b[o + 13] = c(x.AMS, 3) | c(x.KVS, 7) << 2; b[o + 14] = c(x.OUT, 99); b[o + 15] = c(x.MODE, 1) | c(x.CRS, 31) << 1; b[o + 16] = c(x.FINE, 99); }
  const g = v.g; for (let i = 0; i < 4; i++) { b[102 + i] = c(g['PR' + (i + 1)], 99); b[106 + i] = c(g['PL' + (i + 1)], 99); }
  b[110] = c(g.ALG, 31); b[111] = c(g.FB, 7) | c(g.SYNC, 1) << 3; b[112] = c(g.LSPD, 99); b[113] = c(g.LDLY, 99); b[114] = c(g.LPMD, 99); b[115] = c(g.LAMD, 99);
  b[116] = c(g.LSYNC, 1) | c(g.LWAVE, 5) << 1 | c(g.PMS, 7) << 4; b[117] = c(g.TRSP, 48);
  const nm = (v.name || '').toUpperCase().padEnd(10).slice(0, 10); for (let i = 0; i < 10; i++) b[118 + i] = nm.charCodeAt(i) & 127;
  return b;
}
/* a single voice's 155 values (DX7 format 0) -> packed */
function vFrom155(u) {
  const v = { op: [], g: {} };
  for (let k = 0; k < 6; k++) { const o = (5 - k) * 21, x = {}; OPF.forEach(([n], i) => x[n] = u[o + i]); v.op.push(x); }
  ['PR1', 'PR2', 'PR3', 'PR4', 'PL1', 'PL2', 'PL3', 'PL4', 'ALG', 'FB', 'SYNC', 'LSPD', 'LDLY', 'LPMD', 'LAMD', 'LSYNC', 'LWAVE', 'PMS', 'TRSP'].forEach((n, i) => v.g[n] = u[126 + i]);
  v.name = String.fromCharCode(...u.slice(145, 155)); return vPack(v);
}
function syxVoices(d) {
  const out = [];
  for (let i = 0; i + 6 < d.length; i++) {
    if (d[i] !== 0xF0 || d[i + 1] !== 0x43) continue;
    if (d[i + 3] === 9 && d[i + 4] === 0x20 && d[i + 5] === 0 && i + 6 + 4096 <= d.length) { for (let k = 0; k < 32; k++) out.push(Uint8Array.from(d.slice(i + 6 + 128 * k, i + 6 + 128 * k + 128))); i += 4100; }
    else if (d[i + 3] === 0 && d[i + 4] === 1 && d[i + 5] === 0x1B && i + 6 + 155 <= d.length) { out.push(vFrom155(d.slice(i + 6, i + 161))); i += 160; }
  }
  return out;
}
function numIn(val, max, min, on) { const i = document.createElement('input'); i.type = 'number'; i.className = 'cell'; i.min = min; i.max = max; i.value = val; i.oninput = () => on(+i.value); return i; }
/* ---- the FM6 bank */
const F = { img: null, used: 0, sel: 0, file: [], src: 0, dirty: false };
const fVoice = k => F.img.subarray(16 + 128 * k, 144 + 128 * k);
function fName(k) { return (F.used >>> k) & 1 ? vUnpack(fVoice(k)).name || '?' : L('bempty'); }
let fTry = 0;
function dxName(k) { return F.img && (F.used >>> k) & 1 ? fName(k) : 'DX7 ' + (k + 1); }   /* (a sound numbered 2000 + k: the FM-1's SOUNDS, 3.2) */
async function fRead() { const i = await ask(14, [0]); F.src = i[1]; F.img = await bankRead(0); F.used = u32(F.img, 8) & 0x7FFFFFF; F.dirty = false; }
function fChanged() { F.dirty = true; if (F.info) fInfo(); F.used |= 1 << F.sel; clearTimeout(fTry); fTry = setTimeout(() => ask(14, [6, F.sel, ...fVoice(F.sel)]).catch(fail), 120); drawFm6Slots(); }
function fInfo() { F.info.textContent = !F.img ? '' : F.dirty ? L('bunsaved') : L('bsrc')[F.src] || ''; F.info.style.color = F.dirty ? 'var(--warn, #ffb84d)' : ''; }
function drawFm6() {
  const P = $('tab_fm6'); P.innerHTML = '';
  const c = card(P, L('fm6'), '', L('fhelp'), 'wide');
  const r = el('div', 'tools'); F.info = el('span', 'dim', ''); fInfo();
  const bs = button(L('fsave'), async () => { try { F.img.set(w32(F.used), 8); await bankWrite(0, F.img); F.src = 2; F.dirty = false; fInfo(); log(L('fsaved'), 'ok'); } catch (e) { fail(e); } }, 'pri');
  const bx = button(L('fsyx'), () => save(fm6Syx(), 'sloop3-fm6-bank.syx')); bs.disabled = bx.disabled = !F.img;
  r.append(button(L('bread'), async () => { try { await fRead(); drawFm6(); } catch (e) { fail(e); } }),
    button(L('bload'), () => pick(async d => {
      const v = syxVoices(d); if (!v.length) { log(L('nosyx'), 'bad'); return; }
      F.file = v;
      try { if (!F.img) await fRead(); } catch (e) { fail(e); return; }   /* (3.2: the bank as the FM-1 has it, read first: until then the file's voices had nowhere to go) */
      if (!F.used) {                                                      /* an empty bank: the file's voices in B1 and on, at once */
        const n = Math.min(27, v.length); for (let k = 0; k < n; k++) { fVoice(k).set(v[k]); F.used |= 1 << k; }
        F.dirty = true; log(n + L('bplaced') + (v.length > 27 ? L('bmore') : ''), 'ok');
      } else log(v.length + L('bgot'), 'ok');
      drawFm6();
    }, '.syx')), bs, bx, F.info);
  c.appendChild(r);
  if (F.file.length) {
    const fr = el('div', 'tools'); const fs = select(F.file.map((b, i) => [i, (i + 1) + ' ' + vUnpack(b).name]), 0);
    fr.append(el('span', 'dim', L('voices')), fs, button(L('put'), () => { if (!F.img) return; fVoice(F.sel).set(F.file[+fs.value]); fChanged(); drawFm6Edit(); }, 'sm'),
      button(L('fillall'), () => { if (!F.img) return; let i = +fs.value; for (let k = F.sel; k < 27 && i < F.file.length; k++, i++) { fVoice(k).set(F.file[i]); F.used |= 1 << k; } F.dirty = true; fInfo(); drawFm6Slots(); drawFm6Edit(); }, 'sm'));
    c.appendChild(fr);
  }
  F.slots = el('div', 'slots'); F.edit = el('div'); F.edit.style.marginTop = '10px'; F.edit.style.overflowX = 'auto'; c.append(F.slots, F.edit);
  drawFm6Slots(); drawFm6Edit();
}
function drawFm6Slots() {
  const E = F.slots; if (!E || !F.img) return; E.innerHTML = '';
  for (let k = 0; k < 27; k++) { const d = el('div', 'slot' + (k === F.sel ? ' on' : '')); d.style.cursor = 'pointer'; const s = el('span', (F.used >>> k) & 1 ? '' : 'dim', fName(k));
    d.append(el('b', '', 'B' + (k + 1) + ' '), s); d.onclick = () => { F.sel = k; drawFm6Slots(); drawFm6Edit(); }; E.appendChild(d); }
}
function drawFm6Edit() {
  const E = F.edit; if (!E || !F.img) return; E.innerHTML = ''; const k = F.sel, used = (F.used >>> k) & 1;
  const v = used ? vUnpack(fVoice(k)) : null, row = el('div', 'row'); row.appendChild(el('b', '', 'B' + (k + 1)));
  if (used) {
    const nm = el('input', 'txt'); nm.maxLength = 10; nm.value = v.name; nm.oninput = () => { v.name = nm.value; fVoice(k).set(vPack(v)); fChanged(); };
    row.append(nm, button(L('bhear'), () => ask(14, [6, k, ...fVoice(k)]).then(b => log('B' + (k + 1) + ': ' + b[2] + L('btracks'))).catch(fail), 'sm'),
      button(L('bclear'), () => { F.used &= ~(1 << k); fVoice(k).fill(0); F.dirty = true; fInfo(); drawFm6Slots(); drawFm6Edit(); }, 'sm red'));
  }
  E.appendChild(row); if (!used) return;
  const set = () => { fVoice(k).set(vPack(v)); fChanged(); };
  const g = el('div', 'knobs'); g.style.marginTop = '8px';
  for (const [n, m] of GLF) g.appendChild(knob({ label: n, min: n === 'ALG' ? 1 : 0, max: n === 'ALG' ? 32 : m, value: n === 'ALG' ? v.g[n] + 1 : v.g[n], onChange: x => { v.g[n] = n === 'ALG' ? x - 1 : x; set(); } }));
  E.appendChild(g);
  const tb = el('table', 'tb'); tb.style.marginTop = '8px'; const hr = el('tr'); hr.appendChild(el('th'));
  for (const [n] of OPF) hr.appendChild(el('th', 'dim', n)); tb.appendChild(hr);
  for (let o = 0; o < 6; o++) { const tr = el('tr'); tr.appendChild(el('th', '', L('op') + (o + 1)));
    for (const [n, m] of OPF) { const td = el('td'); td.appendChild(numIn(v.op[o][n], m, 0, x => { v.op[o][n] = clamp(x | 0, 0, m); set(); })); tr.appendChild(td); }
    tb.appendChild(tr); }
  E.appendChild(tb);
}
function initVoice() {
  return vPack({ op: Array.from({ length: 6 }, (_, o) => ({ R1: 99, R2: 99, R3: 99, R4: 99, L1: 99, L2: 99, L3: 99, L4: 0, BP: 39, LD: 0, RD: 0, LC: 0, RC: 0, RS: 0, AMS: 0, KVS: 0, OUT: o ? 0 : 99, MODE: 0, CRS: 1, FINE: 0, DET: 7 })),
    g: { ALG: 0, FB: 0, SYNC: 1, TRSP: 24, PR1: 99, PR2: 99, PR3: 99, PR4: 99, PL1: 50, PL2: 50, PL3: 50, PL4: 50, LSPD: 35, LDLY: 0, LPMD: 0, LAMD: 0, LSYNC: 1, LWAVE: 0, PMS: 3 }, name: 'INIT VOICE' });
}
function fm6Syx() {                                    /* a DX7 32-voice bank: B1-B27 (empty: the init voice), then 5 init voices */
  const d = new Uint8Array(4104); d.set([0xF0, 0x43, 0, 9, 0x20, 0]);
  for (let k = 0; k < 32; k++) d.set(k < 27 && (F.used >>> k) & 1 ? fVoice(k) : initVoice(), 6 + 128 * k);
  let s = 0; for (let i = 6; i < 4102; i++) s += d[i];
  d[4102] = (128 - (s & 127)) & 127; d[4103] = 0xF7; return d;
}
/* ---- the SYN kits: 16 sounds of 22 bytes, our pads' order */
const SYF = [['WAVE', 0, 5, ['-', 'SINE', 'TRI', 'SQR', 'FM', 'BELL']], ['NOISE', 0, 4, ['-', 'WHITE', 'METAL', 'CYM', 'CHIP']], ['NOTE', 0, 127], ['/16', 0, 15], ['BEND', 0, 96], ['BT', 0, 127],
  ['HOLD', 0, 255], ['DEC', 0, 127], ['TONE', 0, 127], ['T2', 0, 255], ['T2LV', 0, 127], ['CLICK', 0, 127], ['NLEV', 0, 127], ['NHOLD', 0, 255], ['NDEC', 0, 127],
  ['FLT', 0, 255], ['CUT', 0, 127], ['FENV', -128, 127], ['HPF', 0, 127], ['CHIP', 0, 127], ['DRIVE', 0, 127], ['LVL', -128, 127]];
const Y = { img: null, kit: 0, src: 0, pad: 0 };
const yKit = k => Y.img.subarray(16 + XB.KIT * k, 16 + XB.KIT * (k + 1));
let yTry = 0;
function yChanged() { clearTimeout(yTry); yTry = setTimeout(() => ask(14, [5, Y.kit, ...yKit(Y.kit)]).catch(fail), 80); }
function yGet(p, f) { const b = yKit(Y.kit)[4 + 22 * p + f]; return f === 17 ? (b << 24) >> 24 : f === 21 ? b - 128 : f === 1 ? b & 15 : b; }
function ySet(p, f, v) { const a = yKit(Y.kit), i = 4 + 22 * p + f; a[i] = f === 17 ? v & 255 : f === 21 ? (v + 128) & 255 : f === 1 ? (a[i] & 0x10) | (v & 15) : v & 255; yChanged(); }
const yPlay = p => { noteOut(DRUM_T, p, 110, 1); setTimeout(() => noteOut(DRUM_T, p, 0, 0), 300); };
function drawSyn() {
  const P = $('tab_drumsynth'); P.innerHTML = '';
  const c = card(P, L('syn'), '', L('yhelp'), 'wide');
  const r = el('div', 'tools'); const info2 = el('span', 'dim', Y.img ? L('ysrc')[Y.src] || '' : '');
  const ks = select([0, 1, 2, 3].map(k => [k, L('kitN') + (k + 1)]), Y.kit, v => { Y.kit = +v; drawSyn(); });
  const bs = button(L('ysave'), async () => { try { await bankWrite(1, Y.img); Y.src = 2; log(L('ysaved'), 'ok'); drawSyn(); } catch (e) { fail(e); } }, 'pri'); bs.disabled = !Y.img;
  r.append(button(L('bread'), async () => { try { const i = await ask(14, [0]); Y.src = i[6]; Y.img = await bankRead(1); drawSyn(); } catch (e) { fail(e); } }), ks, bs, info2);
  c.appendChild(r); if (!Y.img) return;
  const cr = knob({ label: L('crush'), min: 0, max: 255, value: yKit(Y.kit)[0], onChange: v => { yKit(Y.kit)[0] = v; yChanged(); } }); r.appendChild(cr);
  /* the 16 pads, then the one chosen in full */
  const pg = el('div', 'drumpads'); pg.style.marginTop = '8px';
  PADS.forEach((n, p) => { const d = el('div', 'pad' + (p === Y.pad ? ' cur' : ''), n); d.style.setProperty('--c', TCOL[DRUM_T]); d.onclick = () => { Y.pad = p; yPlay(p); drawSyn(); }; pg.appendChild(d); });
  c.appendChild(pg);
  const p = Y.pad, ed = el('div', 'knobs'); ed.style.marginTop = '10px';
  SYF.forEach(([n, mn, mx, names], f) => ed.appendChild(knob({ label: n, min: mn, max: mx, value: yGet(p, f), bip: mn < 0, color: TCOL[DRUM_T], fmt: names ? v => names[v] : undefined, onChange: v => { ySet(p, f, v); soon('yp', () => yPlay(p), 150); } })));
  const cl = el('label', 'dim'); const cb = el('input'); cb.type = 'checkbox'; cb.checked = !!(yKit(Y.kit)[4 + 22 * p + 1] & 0x10);
  cb.onchange = () => { const a = yKit(Y.kit); a[4 + 22 * p + 1] = (a[4 + 22 * p + 1] & 15) | (cb.checked ? 0x10 : 0); yChanged(); yPlay(p); }; cl.append(cb, ' CLAP');
  const h = el('h2', '', PADS[p]); h.style.color = TCOL[DRUM_T];
  c.append(h, ed, cl, button('▶ ' + PADS[p], () => yPlay(p), 'sm'));
}
/* ---- your theme (SLOOP 2.7's 40 bytes) */
const TH = { name: 'MINE', c: ['#283c50', '#3c5a78', '#7896b4', '#b4c8dc', '#ffffff'], bg: '#08080c', hi: '#ffd23c', trk: ['#50a0ff', '#50e678', '#ffdc3c', '#ff9628'] };
const to565 = h => { const v = parseInt(h.slice(1), 16), r = v >> 16, g = v >> 8 & 255, b = v & 255; return (r >> 3) << 11 | (g >> 2) << 5 | b >> 3; };
const from565 = c => '#' + [((c >> 11) & 31) * 255 / 31, ((c >> 5) & 63) * 255 / 63, (c & 31) * 255 / 31].map(v => Math.round(v).toString(16).padStart(2, '0')).join('');
function themeBytes() {
  const b = new Uint8Array(40), dv = new DataView(b.buffer);
  b.set([0x54, 0x48, 0x4D, 0x32]); const nm = (TH.name || 'MINE').toUpperCase().slice(0, 8); for (let i = 0; i < nm.length; i++) b[4 + i] = nm.charCodeAt(i) & 127;
  TH.c.forEach((v, i) => dv.setUint16(12 + 2 * i, to565(v), true)); TH.trk.forEach((v, i) => dv.setUint16(22 + 2 * i, to565(v), true));
  dv.setUint16(30, to565(TH.bg), true); dv.setUint16(32, to565(TH.hi), true); dv.setUint32(36, crc32(b.slice(0, 36)), true);
  return [...b];
}
function drawTheme() {
  const P = $('tab_theme'); P.innerHTML = '';
  const c = card(P, L('theme'), '', L('themeHelp'), 'wide');
  const r = el('div', 'tools'); const nm = el('input', 'txt'); nm.maxLength = 8; nm.value = TH.name; nm.oninput = () => TH.name = nm.value;
  r.append(button(L('tget'), async () => { try { const b = await ask(ED.THEME, []); const t = b.slice(1, 41);
    if (t[0] === 0x54 && t[1] === 0x48 && t[2] === 0x4D && t[3] === 0x32) { TH.name = str(t, 4, 8); for (let i = 0; i < 5; i++) TH.c[i] = from565(u16(t, 12 + 2 * i)); for (let i = 0; i < 4; i++) TH.trk[i] = from565(u16(t, 22 + 2 * i)); TH.bg = from565(u16(t, 30)); TH.hi = from565(u16(t, 32)); drawTheme(); } } catch (e) { fail(e); } }),
    button(L('tsend'), async () => { try { await ask(ED.THEME, themeBytes()); log(L('tok'), 'ok'); } catch (e) { fail(e); } }, 'pri'), el('span', 'dim', L('tname')), nm);
  c.appendChild(r);
  const row = el('div', 'row'); row.style.alignItems = 'flex-start'; row.style.gap = '20px'; c.appendChild(row);
  const cols = el('div', 'row'); cols.style.maxWidth = '420px';
  const pv = el('canvas'); pv.width = 320; pv.height = 170; pv.style.cssText = 'border-radius:8px;border:1px solid var(--line)';
  const prev = () => { const g = pv.getContext('2d'); g.fillStyle = TH.bg; g.fillRect(0, 0, 320, 170); g.fillStyle = TH.c[0]; g.fillRect(0, 0, 320, 24);
    g.fillStyle = TH.c[4]; g.font = 'bold 13px system-ui'; g.fillText('KEYS · RHODES', 8, 17); g.fillStyle = TH.hi; g.fillText('120.0', 268, 17);
    for (let i = 0; i < 4; i++) { const x = 12 + i * 76; g.strokeStyle = TH.c[2]; g.lineWidth = 5; g.beginPath(); g.arc(x + 26, 70, 22, 0.75 * Math.PI, 2.25 * Math.PI); g.stroke(); g.strokeStyle = TH.trk[i]; g.beginPath(); g.arc(x + 26, 70, 22, 0.75 * Math.PI, (0.75 + 0.3 * (i + 1)) * Math.PI); g.stroke();
      g.fillStyle = TH.c[3]; g.font = '11px system-ui'; g.fillText(['CUT', 'RES', 'ENV', 'DRV'][i], x + 14, 110); }
    for (let i = 0; i < 5; i++) { g.fillStyle = i < 4 ? TH.trk[i] : '#ff9628'; g.fillRect(12 + i * 60, 132, 50, 26); } };
  const all = [...TH.c.map((v, i) => ['c', i]), ['bg'], ['hi'], ...TH.trk.map((v, i) => ['trk', i])];
  all.forEach(([key, i], j) => { const d = el('label', 'tc'), inp = el('input'); inp.type = 'color'; inp.value = i === undefined ? TH[key] : TH[key][i];
    inp.oninput = () => { if (i === undefined) TH[key] = inp.value; else TH[key][i] = inp.value; prev(); }; d.append(inp, el('span', '', L('tlab')[j])); cols.appendChild(d); });
  row.append(cols, pv); prev();
}
