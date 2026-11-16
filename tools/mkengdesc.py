#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Writes app/engdesc.c: each engine's eight values as SLOOP 2.8.5 shows them (docs/sloop/facts.json: labels, ranges,
# choices; facts, not code), for the engines SLOOP 3 has (ENG below), the drums' own.   tools/mkengdesc.py app/engdesc.c
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
facts = json.load(open(os.path.join(HERE, '..', 'docs', 'sloop', 'facts.json'), encoding='utf-8'))
# SLOOP 3's engines (engine.h ENG_* order) -> SLOOP's engine index (None: the drums, our own)
ENG = [('ENG_VA', 0), ('ENG_DRUM', None), ('ENG_FM', 1), ('ENG_LOFI', 3), ('ENG_PD', 2), ('ENG_WHEEL', 7), ('ENG_NOISE', 11), ('ENG_VOICE', 5), ('ENG_TRIO', 6), ('ENG_PHYS', 10), ('ENG_FM6', 9), ('ENG_SAMPLE', 4), ('ENG_GRAIN', 8)]
FMT = {'F_INT': 'PF_NUM', 'F_PCT': 'PF_PCT', 'F_BIPCT': 'PF_BI', 'F_TIME': 'PF_TIME', 'F_LFOHZ': 'PF_HZ', 'F_CUTOFF': 'PF_CUT',
       'F_SEMI': 'PF_SEMI', 'F_ENUM': 'PF_LIST', 'F_DB': 'PF_DB'}
CUT = ('CUT', 'FREQ', 'TONE', 'BRIT')
RES = ('RES', 'Q')
lists, rows = {}, []
def lst(names):
    key = tuple(names)
    if key not in lists: lists[key] = 'L%d' % len(lists)
    return lists[key]
for ours, si in ENG:
    if si is None:
        kits = facts['drum_kits'][:5] + [k['name'] for k in facts['synth_kits']] + facts['drum_kits'][5:]
        rows.append('    [%s] = { "DRUMS", { "KIT", "SOUNDS" }, { { "KIT", PF_LIST, %d, %s }, { "TUNE", PF_SEMI, 0, 0 }, { "DEC", PF_NUM, 0, 0 }, '
                    '{ "DRIVE", PF_PCT, 0, 0 }, { "", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 } }, -1, -1 },' % (ours, len(kits), lst(kits)))
        continue
    e = facts['engines'][si]
    ps, cut, res = [], -1, -1
    for k, d in enumerate(e['edit']):
        lab = d['label'] if d['label'] != '-' else ''
        fmt = 'PF_CENT' if d.get('unit') == 'ct' else FMT.get(d['fmt'], 'PF_NUM')
        n, ch = 0, '0'
        if (d['names'] or lab in ('SET', 'SRC')) and fmt in ('PF_LIST', 'PF_NUM'):
            fmt = 'PF_LIST'
            names = d['names'][:d['max'] - d['min'] + 1] if d['max'] is not None else d['names']
            if lab == 'PTCH': names = names[:35]                # (FM6: the 8 factory patches, the bank's 27)
            if lab in ('SET', 'SRC'): names = facts['sample_sets'] + ['USR1', 'USR2', 'USR3', 'USR4']   # (the bank's sets)
            n, ch = len(names), lst(names)
        ps.append('{ "%s", %s, %d, %s }' % (lab, fmt, n, ch))
        if lab in CUT and cut < 0: cut = k
        if lab in RES and res < 0: res = k
    rows.append('    [%s] = { "%s", { "%s", "%s" }, { %s }, %d, %d },' % (ours, e['name'], e['pages'][0], e['pages'][1], ', '.join(ps), cut, res))
# SLICE: not in SLOOP 2.8.5's release (no facts there): ours, as engine/slice.c describes it
rows.append('    [ENG_SLICE] = { "SLICE", { "CUT", "PLAY" }, { { "SRC", PF_LIST, 5, %s }, { "DIV", PF_LIST, 5, %s }, { "STRT", PF_NUM, 0, 0 }, '
            '{ "PTCH", PF_SEMI, 0, 0 }, { "MODE", PF_LIST, 3, %s }, { "REV", PF_LIST, 2, %s }, { "DCAY", PF_NUM, 0, 0 }, { "TONE", PF_NUM, 0, 0 } }, 7, -1 },'
            % (lst(['BREAK', 'USR1', 'USR2', 'USR3', 'USR4']), lst(['4', '8', '16', '32', 'AUTO']), lst(['ONE', 'GATE', 'LOOP']), lst(['OFF', 'ON'])))
out = ['/* SPDX-License-Identifier: GPL-3.0-only', ' * The engines as the player sees them (engdesc.h), written by tools/mkengdesc.py from SLOOP 2.8.5\'s facts', ' * (docs/sloop/facts.json): its labels and choices. */', '#include "engdesc.h"', '']
for names, nm in lists.items():
    out.append('static const char *const %s[%d] = { %s };' % (nm, len(names), ', '.join('"%s"' % x for x in names)))
out.append('const edesc gb_edesc[ENG_N] = {')
out += rows
out.append('};')
open(sys.argv[1], 'w', encoding='utf-8').write('\n'.join(out) + '\n')
