/* Sound: a track's sound, its 48 values as knobs (heard at once), the sounds of the FM-1 to choose from, A/B,
 * copy / paste, random, and the sound kept among yours (the 32 places of EDIT 5). ED_SOUND, ED_SET, ED_DESC,
 * ED_NAMES, ED_LIVE op 10; your sounds' file (READ / WRITE kind 1). */
words({
  snd: 'Sound', sndHelp: 'Each value is heard at once on the FM-1 (drag a knob; Shift: finer; double-click: back to where it was when read).\nA / B: two versions to compare (the one shown is the one heard). COPY / PASTE: between tracks of the same engine. RANDOM: each value moved at random by the amount, in the groups ticked.',
  browse: 'Sounds of the FM-1', browseHelp: 'Click: that sound on the track (the drum track takes kits only). Your sounds come after the factory ones.', search: 'search…', allKinds: 'every kind',
  factory: 'Factory', yours: 'Yours', reload: '↻', copy: 'COPY', paste: 'PASTE', random: 'RANDOM', amount: 'amount', A: 'A', B: 'B',
  keep: 'Keep among your sounds', keepHelp: 'The sound as it is now (its 48 values, its sends) into one of your 32 places, with a name and a kind. It is then in PRESETS after the factory sounds, and in EDIT 5 (USER SOUNDS). The other places stay as they are.',
  place: 'place ', name: 'name', kind: 'kind', keepBtn: 'Keep it', kept: 'Kept in your sounds: ', otherEng: 'Not the same engine: choose a sound of that engine first.', drumNo: 'The drum track\'s sound is its kit: see Drum kit.',
  g: ['Engine', 'Envelope', 'LFO', 'Voice', 'Insert', 'Slicer'], hear: 'Hear'
}, {
  snd: 'Son', sndHelp: 'Chaque valeur s\'entend tout de suite sur le FM-1 (glisse un knob ; Maj : plus fin ; double-clic : la valeur lue).\nA / B : deux versions à comparer (celle affichée est celle entendue). COPY / PASTE : entre pistes du même moteur. RANDOM : chaque valeur bougée au hasard de la quantité, dans les groupes cochés.',
  browse: 'Sons du FM-1', browseHelp: 'Clic : ce son sur la piste (la piste de batterie ne prend que des kits). Tes sons viennent après ceux d\'usine.', search: 'chercher…', allKinds: 'toutes les catégories',
  factory: 'Usine', yours: 'Les tiens', reload: '↻', copy: 'COPY', paste: 'PASTE', random: 'RANDOM', amount: 'quantité', A: 'A', B: 'B',
  keep: 'Garder parmi tes sons', keepHelp: 'Le son tel qu\'il est (ses 48 valeurs, ses envois) dans une de tes 32 places, avec un nom et une catégorie. Il est ensuite dans PRESETS après les sons d\'usine, et dans EDIT 5 (USER SOUNDS). Les autres places ne changent pas.',
  place: 'place ', name: 'nom', kind: 'catégorie', keepBtn: 'Garder', kept: 'Gardé dans tes sons : ', otherEng: 'Pas le même moteur : choisis d\'abord un son de ce moteur.', drumNo: 'Le son de la piste de batterie est son kit : voir Kit batterie.',
  g: ['Moteur', 'Enveloppe', 'LFO', 'Voix', 'Insert', 'Slicer'], hear: 'Écouter'
});
const GROUPS = [[0, 8], [8, 16], [16, 27], [27, 36], [36, 41], [41, 45]];
const desc = {};
async function getDesc(e) {
  if (!desc[e]) { const b = await ask(ED.DESC, [e]); const ps = []; for (let k = 0; k < 48; k++) { const o = 13 + 12 * k; ps.push({ min: i16(b, o), max: i16(b, o + 2), name: str(b, o + 4, 8) }); } desc[e] = { name: str(b, 1, 12), p: ps }; }
  return desc[e];
}
const SND = { t: 0, s: null, ab: [null, null], abOn: 0, clip: null, rnd: 30, rg: [1, 1, 1, 0, 0, 0], knobs: [], loadedPreset: -2, filter: '', kind: -1 };
async function getSound(t = SND.t) {
  const b = await ask(ED.SOUND, [t]);
  const s = { t: b[0], e: b[1], preset: i16(b, 2), name: str(b, 4, 16), v: [] };
  for (let k = 0; k < 48; k++) s.v.push(i16(b, 20 + 2 * k));
  s.d = await getDesc(s.e); s.v0 = s.v.slice(); return s;
}
async function soundLoad() {
  if (!info) return;
  try { SND.s = await getSound(SND.t); SND.loadedPreset = LV ? LV.t[SND.t].preset : -2; SND.ab = [SND.s.v.slice(), null]; SND.abOn = 0; drawSoundParams(); } catch (e) { fail(e); }
}
const setParam = (k, v) => soon('p' + SND.t + '.' + k, () => ask(ED.SET, [SND.t, k, ...w16(v)]).catch(fail));
async function sendAll(v) { const s = SND.s; for (let k = 0; k < 48; k++) if (s.v[k] !== v[k]) { s.v[k] = v[k]; await ask(ED.SET, [SND.t, k, ...w16(v[k])]); } drawSoundParams(); }
function drawSound() {
  const P = $('tab_sound'); P.innerHTML = '';
  const tr = el('div', 'row'); tr.style.marginBottom = '10px';
  for (const t of SHOWN) { const b = button(trkName(t), () => { SND.t = t; act(9, t); drawSound(); soundLoad(); }, t === SND.t ? 'on' : ''); b.style.borderColor = TCOL[t]; tr.appendChild(b); }
  P.appendChild(tr);
  const cards = el('div', 'cards'); cards.style.gridTemplateColumns = 'minmax(0,2fr) minmax(280px,1fr)'; P.appendChild(cards);
  const left = el('div'), right = el('div'); left.style.cssText = right.style.cssText = 'display:flex;flex-direction:column;gap:12px;min-width:0'; cards.append(left, right);
  /* the values */
  const c = card(left, L('snd'), '', L('sndHelp')); SND.head = el('h2'); c.insertBefore(SND.head, c.children[1]);
  const tools = el('div', 'tools');
  const bA = button('A', () => abSwitch(0), 'sm'), bB = button('B', () => abSwitch(1), 'sm'); SND.abBtns = [bA, bB];
  const rs = el('input'); rs.type = 'range'; rs.min = 5; rs.max = 100; rs.value = SND.rnd; rs.style.width = '90px'; rs.oninput = () => SND.rnd = +rs.value;
  tools.append(button(L('reload'), soundLoad, 'sm'), bA, bB, button(L('copy'), () => { if (SND.s) { SND.clip = { e: SND.s.e, v: SND.s.v.slice() }; log(L('copy') + ' ✓'); } }, 'sm'),
    button(L('paste'), () => { if (!SND.s || !SND.clip) return; if (SND.clip.e !== SND.s.e) { log(L('otherEng'), 'bad'); return; } sendAll(SND.clip.v).catch(fail); }, 'sm'),
    button(L('random'), randomize, 'sm'), el('span', 'dim', L('amount')), rs);
  L('g').forEach((n, i) => { const lb = el('label', 'dim'); lb.style.fontSize = '12px'; const cb = el('input'); cb.type = 'checkbox'; cb.checked = !!SND.rg[i]; cb.onchange = () => SND.rg[i] = cb.checked ? 1 : 0; lb.append(cb, n); tools.appendChild(lb); });
  c.appendChild(tools);
  SND.box = el('div'); SND.box.style.cssText = 'display:grid;gap:8px'; c.appendChild(SND.box);
  const kc = card(left, L('hear'), ''); kc.appendChild(mkKeys(k => noteOut(SND.t, KEYS[k].note, 100, 1), k => noteOut(SND.t, KEYS[k].note, 0, 0)));
  /* the browser */
  const bc = card(right, L('browse'), '', L('browseHelp'));
  const br = el('div', 'row'); const q = el('input', 'txt'); q.placeholder = L('search'); q.value = SND.filter; q.style.flex = '1'; q.oninput = () => { SND.filter = q.value; drawBrowser(); };
  const ks = select([[-1, L('allKinds')], ...NAMES.kinds.map(k => [k.i, k.name])], SND.kind, v => { SND.kind = +v; drawBrowser(); });
  br.append(q, ks); SND.list = el('div', 'list'); SND.list.style.marginTop = '8px'; bc.append(br, SND.list);
  /* keep among yours */
  const uc = card(right, L('keep'), '', L('keepHelp'));
  const pl = select(NAMES.user.map(u => [u.i, L('place') + (u.i + 1) + (u.used ? ' · ' + u.name : '')]), 0);
  const nm = el('input', 'txt'); nm.maxLength = 12; nm.placeholder = L('name'); nm.style.width = '130px';
  const kd = select(NAMES.kinds.filter(k => k.name !== 'KIT').map(k => [k.i, k.name]), 8);
  const ur = el('div', 'row'); ur.append(pl, nm, kd, button(L('keepBtn'), () => keepSound(+pl.value, nm.value, +kd.value), 'pri')); uc.appendChild(ur);
  SND.keepName = nm;
  drawBrowser(); drawSoundParams();
}
function drawSoundParams() {
  const s = SND.s; if (!SND.box) return; SND.box.innerHTML = ''; SND.knobs = [];
  if (!s) return;
  SND.head.textContent = trkName(SND.t) + ' · ' + s.name + (s.preset < -1 ? ' *' : '') + ' · ' + s.d.name; SND.head.style.color = TCOL[SND.t];
  if (SND.keepName && !SND.keepName.value) SND.keepName.value = s.name.slice(0, 12);
  SND.abBtns.forEach((b, i) => b.classList.toggle('on', SND.abOn === i));
  GROUPS.forEach(([a, z], gi) => {
    const g = el('div', 'pgroup'), ks = el('div', 'knobs'); g.append(el('h4', '', L('g')[gi]), ks); let any = false;
    for (let k = a; k < z; k++) {
      const d = s.d.p[k]; if (!d.name || d.max <= d.min) continue; any = true;
      const kn = knob({ label: d.name, min: d.min, max: d.max, value: s.v[k], def: s.v0[k], bip: d.min < 0, color: TCOL[SND.t], id: 'snd.' + d.name,
        onChange: v => { s.v[k] = v; if (SND.ab[SND.abOn]) SND.ab[SND.abOn][k] = v; setParam(k, v); } });
      SND.knobs[k] = kn; ks.appendChild(kn);
    }
    if (any) SND.box.appendChild(g);
  });
}
function abSwitch(i) {
  if (!SND.s || i === SND.abOn) return;
  SND.ab[SND.abOn] = SND.s.v.slice(); if (!SND.ab[i]) SND.ab[i] = SND.s.v.slice();
  SND.abOn = i; sendAll(SND.ab[i]).catch(fail);
}
function randomize() {
  const s = SND.s; if (!s) return; const v = s.v.slice();
  GROUPS.forEach(([a, z], gi) => { if (!SND.rg[gi]) return; for (let k = a; k < z; k++) { const d = s.d.p[k]; if (!d.name || d.max <= d.min) continue; v[k] = clamp(Math.round(v[k] + (Math.random() - 0.5) * 2 * (d.max - d.min) * SND.rnd / 100), d.min, d.max); } });
  sendAll(v).catch(fail);
}
function drawBrowser() {
  const L1 = SND.list; if (!L1) return; L1.innerHTML = ''; const f = SND.filter.trim().toUpperCase(), drum = SND.t === DRUM_T;
  const cur = LV ? LV.t[SND.t].preset & 0x7FFF : -1;
  const kindName = k => (NAMES.kinds[k] || {}).name || '';
  let last = -1;
  const ps = NAMES.presets.filter(p => (kindName(p.kind) === 'KIT') === drum && (SND.kind < 0 || p.kind === SND.kind) && (!f || p.name.includes(f) || kindName(p.kind).includes(f) || (ENGINES[p.eng] || '').includes(f)))
    .sort((a, b) => a.kind - b.kind || a.i - b.i);
  for (const p of ps) {
    if (p.kind !== last) { last = p.kind; L1.appendChild(el('div', 'hd', kindName(p.kind))); }
    const d = el('div', cur === p.i ? 'on' : ''); d.append(el('span', '', p.name), el('span', 'cat', ENGINES[p.eng] || ''));
    d.onclick = () => choose(p.i); L1.appendChild(d);
  }
  if (!drum) {
    const us = NAMES.user.filter(u => u.used && (!f || u.name.includes(f)) && SND.kind < 0);
    if (us.length) L1.appendChild(el('div', 'hd', L('yours')));
    for (const u of us) { const d = el('div', cur === 1000 + u.i ? 'on' : ''); d.append(el('span', '', u.name), el('span', 'cat', 'USER ' + (u.i + 1))); d.onclick = () => choose(1000 + u.i); L1.appendChild(d); }
  }
}
async function choose(k) { try { await live(10, SND.t, k & 255, k >> 8); await soundLoad(); drawBrowser(); } catch (e) { fail(e); } }
/* the sound into one of your places: your sounds' file read, that place rewritten, the file written back */
async function keepSound(place, name, kind) {
  const s = SND.s; if (!s) return;
  if (SND.t === DRUM_T) { log(L('drumNo'), 'bad'); return; }
  try {
    const f = await fileRead(1, 0); if (f[0] !== 0x53 || f[1] !== 0x33 || f[2] !== 0x55 || f[3] !== 0x53) throw new Error(L('err')[5]);
    const r = new Uint8Array(80), tk = LV ? LV.t[SND.t] : { cho: 0, dly: 0, rev: 0 };
    r[0] = 0xA5; r[1] = s.e; r[2] = kind; r[3] = 0;
    const nm = (name || s.name || 'MINE').toUpperCase().slice(0, 12); for (let i = 0; i < nm.length; i++) r[4 + i] = nm.charCodeAt(i) & 127;
    r[16] = clamp(s.v[35] | 0, 0, 127); r[17] = tk.cho; r[18] = tk.dly; r[19] = tk.rev;
    r[24] = 1000 & 255; r[25] = 1000 >> 8; r[26] = 0; r[27] = 1;
    for (let k = 0; k < 48; k++) { const d = s.d.p[k]; r[28 + k] = d.max > d.min ? Math.round((s.v[k] - d.min) / (d.max - d.min) * 255) : 0; }
    f.set(r, 8 + place * 80);
    await fileWrite(1, 0, f);
    log(L('kept') + (place + 1) + ' · ' + nm, 'ok');
    await retryBusy(() => readNames()); drawSound(); soundLoad();
  } catch (e) { fail(e); }
}
liveHooks.push(st => {                                  /* the sound changed on the FM-1: read again */
  if (!$('tab_sound').classList.contains('on') || !SND.s) return;
  if (st.t[SND.t].preset !== SND.loadedPreset && (st.t[SND.t].preset & 0x7FFF) !== (SND.loadedPreset & 0x7FFF)) { SND.loadedPreset = st.t[SND.t].preset; soon('sndre', soundLoad, 300); }
});
