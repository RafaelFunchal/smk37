#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Writes engine/fm6_patches.c: FM6's factory patches F1..F8 and its init voice, as SLOOP 2.8.5 has them
# (docs/sloop/facts.json: each operator's settings, read from SLOOP's tools as data).   tools/mkfm6.py engine/fm6_patches.c
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
f = json.load(open(os.path.join(HERE, '..', 'docs', 'sloop', 'facts.json'), encoding='utf-8'))
def op(o):
    return '{ { %s }, { %s }, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }' % (
        ', '.join(map(str, o['r'])), ', '.join(map(str, o['l'])), o['ol'], o['fc'], o['ff'], o['det'], o['mode'], o['kvs'],
        o['ams'], o['rs'], o['bp'], o['ld'], o['rd'], o['lc'], o['rc'])
def voice(v):
    return ('    { "%s", { %s }, %d, %d, %d, { %s }, { %s }, %d, %d, %d, %d, %d, %d, %d, %d },' % (
        v['name'], ',\n        '.join(op(o) for o in v['ops']), v['alg'], v['fb'], v['oks'], ', '.join(map(str, v['pr'])),
        ', '.join(map(str, v['pl'])), v['lfs'], v['lfd'], v['lpmd'], v['lamd'], v['lks'], v['lfw'], v['lpms'], v['trnsp']))
out = ['/* SPDX-License-Identifier: GPL-3.0-only', ' * FM6\'s factory patches (F1..F8) and its init voice, written by tools/mkfm6.py from SLOOP 2.8.5\'s facts', ' * (docs/sloop/facts.json). OP1 first. */', '#include "fm6.h"', '',
       'const fm6_patch fm6_init_voice =', voice(f['fm6_init'])[4:-1] + ';',
       'const fm6_patch fm6_factory[FM6_NFACTORY] = {']
out += [voice(v) for v in f['fm6_patches']]
out.append('};')
open(sys.argv[1], 'w', encoding='utf-8').write('\n'.join(out) + '\n')
