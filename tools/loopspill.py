#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Lists the loops of an FM-1 build (backward branches in llvm-objdump -d output) with their instruction count,
# float operations and stack accesses (spills): a quick check that a hot loop keeps everything in registers.
#   llvm-objdump -d build/bench/bench.elf | python3 tools/loopspill.py [function-regex]
import re, sys
pat = re.compile(sys.argv[1]) if len(sys.argv) > 1 else None
fn, ins = None, []
def flush():
    if fn is None or (pat and not pat.search(fn)): return
    addrs = [a for a, _ in ins]
    for a, t in ins:
        m = re.search(r'goto -?\d+ <[^+>]+\+0x([0-9a-f]+)>', t) or re.search(r'goto -?\d+ <([^+>]+)>', t)
        if not m or 'goto' not in t: continue
        m2 = re.search(r'<[^>]*>', t)
        tgt = re.search(r'goto (-?\d+)', t)
        if not tgt: continue
        off = int(tgt.group(1))
        if off >= 0: continue
        dst = a + off + (4 if len(t.split('\t')[0].split()) >= 4 else 2)
        body = [x for b, x in ins if dst - 4 <= b <= a]
        sp = sum(1 for x in body if re.search(r'\[sp(\+\d+)?\]', x) and 'sp++' not in x and '--sp' not in x)
        fl = sum(1 for x in body if '(f)' in x)
        print(f"{fn:22} loop {dst:#x}..{a:#x}: {len(body):3} instr, {fl:3} float, {sp:2} stack")
for line in sys.stdin:
    m = re.match(r'^([0-9a-f]+) <(.*)>:', line)
    if m:
        flush(); fn, ins = m.group(2), []; continue
    m = re.match(r'^\s+([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)$', line)
    if m: ins.append((int(m.group(1), 16), m.group(2) + '\t' + m.group(3)))
flush()
