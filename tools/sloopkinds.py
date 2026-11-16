#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# A fact table for the import of SLOOP 2.8 projects (app/sloopimport.c): the kind (bass, keys, ...) of each
# factory sound of SLOOP 2.8.5, by its engine and its place in that engine's list, read from SLOOP's sources
# (its preset names and its PRESETS browser's kinds), and its PRESETS list's order (for its favourites). No code of
# SLOOP is used: only these names and kinds.
#   tools/sloopkinds.py <sloop-2.8.5/firmware/src> app/sloop_kinds.c
import re, sys, os
src = sys.argv[1]
ENG = ['eng_analog.c:ANALOG', 'eng_digital.c:DIGITAL', 'eng_phase.c:PHASE', 'eng_lofi.c:LOFI', 'eng_sample.c:SAMPLE',
       'eng_formant.c:FORMANT', 'eng_trio.c:TRIO', 'eng_drawbar.c:DRAWBAR', 'eng_grain.c:GRAIN', 'eng_fm6.c:FM6',
       'eng_phys.c:PHYS', 'eng_noise.c:NOISE']
ui = open(os.path.join(src, 'ui.c'), encoding='utf-8').read()
SYM = {'ENGI_GRAIN': '8', 'ENGI_FM6': '9', 'ENGI_PHYS': '10', 'ENGI_NOISE': '11', 'ENGI_SLICE': '12', 'ENGI_DRAWBAR': '7'}
bank = [(k, SYM.get(e, e), n) for k, e, n in re.findall(r'\{BK_(\w+), (\w+), "([^"]+)"\}', ui)]
kind_of = {(int(e), n): k for k, e, n in bank}
KMAP = {'BASS': 'K_BASSE', 'KEYS': 'K_KEYS', 'ORGAN': 'K_ORGUE', 'PAD': 'K_PAD', 'LEAD': 'K_LEAD', 'PLUCK': 'K_PLUCK', 'STAB': 'K_BRASS', 'FX': 'K_FX'}
rows = []
names_of = []
for e, item in enumerate(ENG):
    f, nm = item.split(':')
    t = open(os.path.join(src, f), encoding='utf-8').read()
    m = re.search(r'static const preset_t ' + nm + r'_PRESETS\[\] = \{(.*?)\n\};', t, re.S)
    names = re.findall(r'^\s*\{"([^"]+)"', m.group(1), re.M) if m else []
    if not names: names = [n for k, ee, n in bank if int(ee) == e]   # (a table the build generates: the browser's order)
    rows.append([KMAP.get(kind_of.get((e, n), 'FX'), 'K_FX') for n in names])
    names_of.append(names)
out = ['/* SPDX-License-Identifier: GPL-3.0-only', ' * Written by tools/sloopkinds.py: the kind of each SLOOP 2.8.5 factory sound, by engine (its order in',
       ' * SLOOP) and its place in that engine\'s list. Facts only (names and kinds), for app/sloopimport.c. */',
       '#include "sloopimport.h"', '#include "presets.h"']
for e, r in enumerate(rows):
    out.append('static const uint8_t K%d[] = { %s };' % (e, ', '.join(r) if r else 'K_FX'))
out.append('const uint8_t *const sloop_kind[12] = { %s };' % ', '.join('K%d' % e for e in range(12)))
out.append('const uint8_t sloop_nkind[12] = { %s };' % ', '.join(str(len(r)) for r in rows))
# SLOOP's PRESETS list (its favourites name a sound by its place there): each entry's engine << 8 | its place in that engine's list
bk = []
for k, e, n in bank:
    e = int(e); i = names_of[e].index(n) if e < len(names_of) and n in names_of[e] else 255
    bk.append('0x%04X' % (e << 8 | i))
out.append('const uint16_t sloop_bank[%d] = { %s };' % (len(bk), ', '.join(bk)))
out.append('const uint16_t sloop_nbank = %d;' % len(bk))
open(sys.argv[2], 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print(' '.join('%d' % len(r) for r in rows), file=sys.stderr)
