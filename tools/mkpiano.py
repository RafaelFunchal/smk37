#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Erik37: Copyright (C) 2026 M. Geisler (genkigenki/makkuro); this tool adapted for SLOOP 3.
"""The PIANO engine's flash image (engine/piano.c), from Erik37's bank (erik37/app/assets/erik37.bank, ERK1 format:
engine/erk_bank.h). The bank is cut down to fit SLOOP's flash budget:
  - zone 0 (A0..C#1) is dropped: its keys play zone 1, their rate (inc) recomputed from the lowest key of zone 1;
  - of the three velocity layers only 0 (soft) and 2 (loud) are kept; the velocity table is rewritten so the layer
    position stays continuous (old p = l0 + w (l1 - l0), 0..2 -> new l0 = 0, l1 = 1, w = p / 2);
  - the image is laid out for its flash address (--base): no sample's bytes fall in --hole (0xFF padding instead,
    the offsets moved), and it must end by --limit.
The output is a raw image that begins at --base. The samples' 'off' stay relative to the data area, as
erk_bank_open expects. Samples: "Salamander Grand Piano V3" by Alexander Holm, CC BY 3.0 (see engine/PIANO.md)."""
import argparse, os, struct, sys

HDR = '<IHHHHIHH'            # erk_bank_hdr, 20 bytes
KEY = '<IIIIHHHHHHBBBB'      # erk_key, 32 bytes
VEL = '<HHHBB'               # erk_vel, 8 bytes
SMP = '<IIIIhhHH'            # erk_smp, 24 bytes
MAGIC, NKEYS, BLK, BLKB = 0x314B5245, 88, 32, 17
SMS = struct.calcsize(SMP)
KEEP = (0, 2)                # the velocity layers kept (soft, loud)


def auto_int(s):
    return int(s, 0)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    here = os.path.dirname(os.path.abspath(__file__))
    ap.add_argument('--bank', default=os.path.normpath(os.path.join(here, '..', 'assets', 'erik37.bank')))
    ap.add_argument('--out', default=os.path.normpath(os.path.join(here, '..', 'build', 'piano', 'piano.img')))
    ap.add_argument('--base', type=auto_int, default=0x78000, help='flash address of the image (default 0x78000)')
    ap.add_argument('--limit', type=auto_int, default=0xC0000, help='flash address the image must end by (default 0xC0000)')
    ap.add_argument('--hole', default='0xC0000:0xC2000', help='lo:hi, flash no sample may use (the keyboard calibration)')
    a = ap.parse_args()
    hole_lo, hole_hi = (int(x, 0) for x in a.hole.split(':'))

    b = open(a.bank, 'rb').read()
    magic, nz, nl, nr, _, dsize, rel_gain, _ = struct.unpack_from(HDR, b, 0)
    if magic != MAGIC or nl != 3 or nz < 2:
        sys.exit('mkpiano: %s is not a 3-layer ERK1 bank' % a.bank)
    o = struct.calcsize(HDR)
    keys = [list(struct.unpack_from(KEY, b, o + 32 * k)) for k in range(NKEYS)]
    o += 32 * NKEYS
    vels = [list(struct.unpack_from(VEL, b, o + 8 * v)) for v in range(128)]
    o += 8 * 128
    smp = [list(struct.unpack_from(SMP, b, o + SMS * i)) for i in range(nz * nl)]
    o += SMS * nz * nl
    rel = [list(struct.unpack_from(SMP, b, o + SMS * i)) for i in range(nr)]
    o += SMS * nr
    data = b[o:]
    if len(data) < dsize:
        sys.exit('mkpiano: the bank is cut short')

    # (a) zone 0 out: its keys on zone 1 (now 0), inc from the lowest key of zone 1, by equal temperament
    z1 = [k for k in range(NKEYS) if keys[k][10] == 1]
    k1 = min(z1)
    inc1 = keys[k1][5]
    for k in range(NKEYS):
        if keys[k][10] == 0:
            inc = int(round(inc1 * 2.0 ** ((k - k1) / 12.0)))
            assert 0 < inc < 65536
            keys[k][5] = inc
            keys[k][10] = 1
        keys[k][10] -= 1
    nz2 = nz - 1

    # (b) layers 0 and 2; the velocity table continuous over them
    for v in vels:
        amp, coef, w, l0, l1 = v
        p = l0 + (w / 32768.0) * (l1 - l0) if l0 != l1 else float(l0)
        v[2] = min(32767, int(round(p / 2.0 * 32768.0)))
        v[3], v[4] = 0, 1
    smp2 = [smp[z * nl + l] for z in range(1, nz) for l in KEEP]

    # (c) the layout: the tables, then each sample's blocks, none in the hole
    hdr_len = struct.calcsize(HDR) + 32 * NKEYS + 8 * 128 + SMS * (len(smp2) + nr)
    assert hdr_len % 4 == 0
    if a.base < hole_hi and a.base + hdr_len > hole_lo:
        sys.exit('mkpiano: the tables fall in the hole')
    out = bytearray()
    pads = 0

    def put(s):
        nonlocal pads
        n = (max(s[1], s[2] + s[3]) + BLK - 1) // BLK * BLKB
        src = data[s[0]:s[0] + n]
        if len(src) != n:
            sys.exit('mkpiano: a sample runs past the data')
        at = a.base + hdr_len + len(out)
        if at < hole_hi and at + n > hole_lo:              # it would touch the hole: past it
            gap = hole_hi - at
            out.extend(b'\xff' * gap)
            pads += gap
        s = list(s)
        s[0] = len(out)
        out.extend(src)
        return s

    smp2 = [put(s) for s in smp2]
    rel2 = [put(s) for s in rel]
    while len(out) % 4:
        out.append(0xFF)

    img = bytearray(struct.pack(HDR, MAGIC, nz2, 2, nr, 0, len(out), rel_gain, 0))
    for k in keys:
        img += struct.pack(KEY, *k)
    for v in vels:
        img += struct.pack(VEL, *v)
    for s in smp2 + rel2:
        img += struct.pack(SMP, *s)
    assert len(img) == hdr_len
    img += out
    end = a.base + len(img)
    print('mkpiano: %d zones x 2 layers + %d release noises, %d bytes (tables %d, data %d, hole padding %d)'
          % (nz2, nr, len(img), hdr_len, len(out), pads))
    print('mkpiano: flash 0x%X .. 0x%X (limit 0x%X: %d bytes %s)'
          % (a.base, end, a.limit, abs(a.limit - end), 'free' if end <= a.limit else 'OVER'))
    if end > a.limit:
        sys.exit('mkpiano: the image does not fit: it ends at 0x%X, past the limit 0x%X' % (end, a.limit))
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, 'wb') as f:
        f.write(img)
    print('mkpiano: wrote', a.out)


if __name__ == '__main__':
    main()
