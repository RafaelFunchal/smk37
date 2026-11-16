#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# SLOOP 2.8.5's sound facts, read from its sources and written as data (docs/sloop/facts.json): the track parameters'
# names (core.h P_*), each engine's eight parameters (label, format, range, default, choice names), its factory sounds
# (name, the eight values, ATK DEC SUS REL, ENV > FILTER, MONO, the sends, the arpeggio, the extra track values), their
# level trims (preset_trim.h), the SAMPLE sets' sounds (tools/gen_samples.py), the PRESETS browser's kinds (ui.c) and the
# drum kits' names. Facts only: no code of SLOOP is used, the values are what its sounds are.
#   tools/sloopfacts.py <sloop-2.8.5 repo> docs/sloop/facts.json
import ast, json, os, re, sys

root = sys.argv[1]
src = os.path.join(root, 'firmware', 'src')
rd = lambda f: open(os.path.join(src, f), encoding='utf-8').read()
nocomment = lambda t: re.sub(r'/\*.*?\*/', '', re.sub(r'//[^\n]*', '', t, flags=0), flags=re.S)

ENG = ['eng_analog.c:ANALOG', 'eng_digital.c:DIGITAL', 'eng_phase.c:PHASE', 'eng_lofi.c:LOFI', 'eng_sample.c:SAMPLE',
       'eng_formant.c:FORMANT', 'eng_trio.c:TRIO', 'eng_drawbar.c:DRAWBAR', 'eng_grain.c:GRAIN', 'eng_fm6.c:FM6',
       'eng_phys.c:PHYS', 'eng_noise.c:NOISE']

# ---- symbols: every enum of core.h and of the engines' files (P_*, NZ_*, PM_*, ...)
SYM = {}
def enums(text):
    for body in re.findall(r'enum\s*\w*\s*\{(.*?)\}', nocomment(text), re.S):
        v = -1
        for item in [x.strip() for x in body.split(',') if x.strip()]:
            if '=' in item:
                k, e = [s.strip() for s in item.split('=', 1)]
                v = cval(e)
            else:
                k = item; v += 1
            SYM[k] = v
def cval(e):
    e = e.strip()
    e = re.sub(r'(\d+)u\b', r'\1', e)
    e = re.sub(r'[A-Za-z_]\w*', lambda m: str(SYM[m.group(0)]) if m.group(0) in SYM else m.group(0), e)
    return int(eval(e.replace('/', '//'), {}))
core = rd('core.h')
enums(core)
for d in re.findall(r'#define\s+(\w+)\s+\(?(\d+)u?\)?', core):
    SYM.setdefault(d[0], int(d[1]))
for item in ENG:
    enums(rd(item.split(':')[0]))
P_NAMES = [k for k, v in sorted(((k, v) for k, v in SYM.items() if k.startswith('P_') and k != 'P_COUNT'), key=lambda kv: kv[1])
           if v < SYM['P_COUNT']]

def split_top(s):                       # split by commas outside braces and parentheses
    out, depth, cur, q = [], 0, '', False
    for ch in s:
        if ch == '"': q = not q
        if not q and ch in '{(': depth += 1
        if not q and ch in '})': depth -= 1
        if ch == ',' and depth == 0 and not q:
            out.append(cur.strip()); cur = ''
        else:
            cur += ch
    if cur.strip(): out.append(cur.strip())
    return out

def names_of(text, ident):
    m = re.search(r'const char \*const ' + ident + r'\[[^\]]*\]\s*=\s*\{(.*?)\};', text, re.S)
    return re.findall(r'"([^"]*)"', m.group(1)) if m else None

def preset(tok, trims):
    f = split_top(tok.strip()[1:-1])
    p = {'name': f[0].strip('"'), 'e': [cval(x) for x in split_top(f[1][1:-1])],
         'env': [cval(x) for x in split_top(f[2][1:-1])], 'fenv': cval(f[3]) if len(f) > 3 else 0,
         'mono': cval(f[4]) if len(f) > 4 else 0, 'fx': None, 'arp': None, 'pat': 0, 'x': []}
    for g in f[5:]:
        m = re.match(r'(FX|ARP|PAT|XP)\((.*)\)$', g, re.S)
        mm = re.match(r'\.x\s*=\s*\{(.*)\}$', g, re.S)
        if m:
            a = [cval(x) for x in split_top(m.group(2))]
            if m.group(1) == 'FX': p['fx'] = a
            elif m.group(1) == 'ARP': p['arp'] = a
            elif m.group(1) == 'PAT': p['pat'] = a[0]
            else: p['x'] = a
        elif mm:
            p['x'] = [cval(x) for x in split_top(mm.group(1))]
        else:
            raise SystemExit('preset field not understood: ' + g)
    xs = p['x']; p['x'] = [[P_NAMES[xs[i] - 1], xs[i + 1]] for i in range(0, len(xs) - 1, 2) if xs[i] > 0]
    return p

trim_text = rd('preset_trim.h')
TRIM = [[int(v) for v in re.findall(r'\*/\s*(-?\d+)', row)] for row in re.findall(r'\{(/\*.*?)\}', trim_text, re.S)]

# ---- the SAMPLE engine's sounds: tools/gen_samples.py (its tables are Python literals)
gs = open(os.path.join(root, 'tools', 'gen_samples.py'), encoding='utf-8').read()
_assign = {n.targets[0].id: n.value for n in ast.parse(gs).body if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name)}
lit = lambda name: ast.literal_eval(_assign[name])
CC0_SETS, SET_PRESETS, EXTRA = lit('CC0_SETS'), lit('SET_PRESETS'), lit('EXTRA_PRESETS')
SET_ORDER = [('PERC' if n == 'KIT' else n, k) for n, k in CC0_SETS]
def smp_preset(i, kind, pname, e, env, mono, fx, more):
    e = list(e); e[0], e[3] = i, 0 if kind == 'kit' else 1
    return {'name': pname, 'e': e, 'env': list(env), 'fenv': 0, 'mono': mono, 'fx': list(fx), 'arp': None,
            'pat': 0, 'x': [[more[j], more[j + 1]] for j in range(0, len(more), 2)]}
smp = []
for i, (n, k) in enumerate(SET_ORDER):
    if n in SET_PRESETS: smp.append(smp_preset(i, k, *SET_PRESETS[n]))
for setname, *rest in EXTRA:
    i = next(j for j, (n, _) in enumerate(SET_ORDER) if n == setname)
    smp.append(smp_preset(i, SET_ORDER[i][1], *rest))

engines = []
for idx, item in enumerate(ENG):
    f, nm = item.split(':')
    t = rd(f)
    m = re.search(r'static const engine_t ENG_\w+ = \{(.*?)\n\};', t, re.S)
    body = nocomment(m.group(1))
    name = re.search(r'"([^"]+)"', body).group(1)
    pages = re.findall(r'"([^"]+)"', re.search(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', body).group(0))
    eb = re.search(r'(?:\.edit\s*=\s*)?\{\s*(\{\s*"[^"]*",\s*F_.*?)\}\s*,\s*(?:\n|\s)*(?:\.presets|\w+_PRESETS|SMP_PRESET)', body, re.S)
    edits = []
    for row in re.findall(r'\{\s*("[^"]*",\s*F_\w+[^}]*)\}', eb.group(1)):
        a = split_top(row)
        lab, fmt = a[0].strip('"'), a[1]
        lo, hi, de = cval(a[2]) if 'NALL' not in a[2] else 0, None, cval(a[4])
        try: hi = cval(a[3])
        except Exception: hi = None                 # (a table the build makes: SMP_NALL - 1)
        nms = names_of(t, a[5]) if a[5] not in ('0', 'NULL') else None
        edits.append({'label': lab, 'fmt': fmt, 'min': lo, 'max': hi, 'def': de, 'names': nms,
                      'unit': a[6].strip('"') if len(a) > 6 and a[6] != '0' else None})
    mac = re.search(r'\{\s*(P_\w+)\s*,\s*(P_\w+)\s*,\s*(P_\w+)\s*,\s*(P_\w+)\s*\}', body)
    if nm == 'SAMPLE':
        pres = smp
    else:
        pm = re.search(r'static const preset_t ' + nm + r'_PRESETS\[\] = \{(.*?)\n\};', t, re.S)
        toks = split_top(nocomment(pm.group(1)))
        pres = [preset(x, None) for x in toks if x.startswith('{')]
    for i, p in enumerate(pres):
        p['trim'] = TRIM[idx][i] if idx < len(TRIM) and i < len(TRIM[idx]) else 0
    engines.append({'index': idx, 'file': f, 'name': name, 'pages': pages, 'edit': edits,
                    'macro': list(mac.groups()) if mac else None, 'presets': pres})

# ---- the PRESETS browser's kinds (ui.c) and the drum kits' names
ui = rd('ui.c')
ESYM = {'ENGI_GRAIN': 8, 'ENGI_FM6': 9, 'ENGI_PHYS': 10, 'ENGI_NOISE': 11, 'ENGI_SLICE': 12, 'ENGI_DRAWBAR': 7}
kinds = [[k, ESYM.get(e, None) if not e.isdigit() else int(e), n] for k, e, n in re.findall(r'\{BK_(\w+), (\w+), "([^"]+)"\}', ui)]
dr = rd('drums.c')
kitnames = names_of(dr, 'DRUM_KIT_NAMES') or re.findall(r'"([^"]+)"', re.search(r'DRUM_KIT_NAMES\[[^\]]*\]\s*=\s*\{(.*?)\};', dr, re.S).group(1))

# ---- the synthesised kits (tools/gen_drumkits.py): each lane's sound in musical units (Hz, ms, dB), as SLOOP wrote
# them. Its tables are read as data (the AST walked here: S(...) a sound with S's defaults, kit(base, LANE=...) a base
# kit with lanes replaced or changed), nothing of it is run
gk_src = open(os.path.join(root, 'tools', 'gen_drumkits.py'), encoding='utf-8').read()
gk = ast.parse(gk_src)
Sdef = next(n for n in gk.body if isinstance(n, ast.FunctionDef) and n.name == 'S')
S_ARGS = [a.arg for a in Sdef.args.args]
S_DEF = dict(zip(S_ARGS[len(S_ARGS) - len(Sdef.args.defaults):], [ast.literal_eval(d) for d in Sdef.args.defaults]))
GENV = {}
def ev(n):
    if isinstance(n, ast.Constant): return n.value
    if isinstance(n, ast.UnaryOp) and isinstance(n.op, ast.USub): return -ev(n.operand)
    if isinstance(n, (ast.Tuple, ast.List)): return [ev(x) for x in n.elts]
    if isinstance(n, ast.Name): return GENV[n.id]
    if isinstance(n, ast.Subscript): import copy; return copy.deepcopy(ev(n.value)[ev(n.slice)])
    if isinstance(n, ast.Dict): return {ev(k): ev(v) for k, v in zip(n.keys, n.values)}
    if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Add, ast.Sub, ast.Mult, ast.Div)):
        a, b = ev(n.left), ev(n.right)
        return a + b if isinstance(n.op, ast.Add) else a - b if isinstance(n.op, ast.Sub) else a * b if isinstance(n.op, ast.Mult) else a / b
    if isinstance(n, ast.Call) and isinstance(n.func, ast.Name):
        f = n.func.id
        if f == 'S':
            d = dict(S_DEF); d.update(zip(S_ARGS, [ev(a) for a in n.args])); d.update({k.arg: ev(k.value) for k in n.keywords}); return d
        if f == 'dict':
            import copy
            d = copy.deepcopy(ev(n.args[0])) if n.args else {}
            d.update({k.arg: ev(k.value) for k in n.keywords}); return d
        if f == 'kit':
            import copy
            k = copy.deepcopy(ev(n.args[0]))
            for kw in n.keywords:
                ch = ev(kw.value)
                if 'wave' in ch: k[kw.arg] = ch
                else: k[kw.arg].update(ch)
            return k
    raise SystemExit('gen_drumkits: not understood: ' + ast.dump(n)[:120])
for n in gk.body:
    if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id in ('LANES',) + tuple(
            x.targets[0].id for x in gk.body if isinstance(x, ast.Assign) and isinstance(x.targets[0], ast.Name) and x.targets[0].id.startswith('K')):
        GENV[n.targets[0].id] = ev(n.value)
LANES = GENV['LANES']
lvf = os.path.join(root, 'tools', 'drumkit_levels.json')
lv = json.load(open(lvf)) if os.path.exists(lvf) else {}
synkits = [{'name': n, 'style': st, 'crush': c, 'lanes': [dict(k[l], trim=lv.get(n, {}).get(l, 0.0)) for l in LANES]}
           for n, st, c, k in GENV['KITS']]
# ---- FM6's factory patches (tools/gen_fm6_patches.py): each operator's settings and the voice's, read as data
fm_src = ast.parse(open(os.path.join(root, 'tools', 'gen_fm6_patches.py'), encoding='utf-8').read())
def fdefaults(name):
    fd = next(n for n in fm_src.body if isinstance(n, ast.FunctionDef) and n.name == name)
    args = [a.arg for a in fd.args.args]
    return args, dict(zip(args[len(args) - len(fd.args.defaults):], [ast.literal_eval(d) for d in fd.args.defaults]))
OPA, OPD = fdefaults('op'); VA, VD = fdefaults('voice')
def fev(n):
    if isinstance(n, ast.Call) and isinstance(n.func, ast.Name) and n.func.id in ('op', 'voice'):
        A, D = (OPA, OPD) if n.func.id == 'op' else (VA, VD)
        d = dict(D); d.update(zip(A, [fev(a) for a in n.args])); d.update({k.arg: fev(k.value) for k in n.keywords})
        return d
    if isinstance(n, (ast.List, ast.Tuple)): return [fev(x) for x in n.elts]
    if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Mult, ast.Add)):   # INIT: [op(ol=99)] + [op() for _ in range(5)]
        return fev(n.left) * fev(n.right) if isinstance(n.op, ast.Mult) else fev(n.left) + fev(n.right)
    if isinstance(n, ast.ListComp):
        return [fev(n.elt) for _ in range(ast.literal_eval(n.generators[0].iter.args[0]))]
    return ast.literal_eval(n)
fm6 = []
for n in fm_src.body:
    if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id == 'PATCHES':
        for t in n.value.elts:
            fm6.append(fev(t.elts[1]))
    if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Name) and n.targets[0].id == 'INIT':
        fm6_init = fev(n.value)
out = {'source': 'SLOOP 2.8.5 (isod89/sloop-fm1), read as facts', 'track_params': P_NAMES,
       'voice_modes': ['POLY', 'MONO', 'LEGATO', 'UNISON'], 'engines': engines, 'kinds': kinds,
       'sample_sets': [n for n, _ in SET_ORDER], 'drum_kits': kitnames,
       'drum_lanes': LANES, 'synth_kits': synkits, 'fm6_patches': fm6, 'fm6_init': fm6_init}
os.makedirs(os.path.dirname(sys.argv[2]) or '.', exist_ok=True)
json.dump(out, open(sys.argv[2], 'w', encoding='utf-8'), indent=1, ensure_ascii=False)
print('presets:', ' '.join(f"{e['name']}={len(e['presets'])}" for e in engines), '=', sum(len(e['presets']) for e in engines),
      '| kits', len(kitnames), '+ synth', len(synkits), '| params', len(P_NAMES), file=sys.stderr)
