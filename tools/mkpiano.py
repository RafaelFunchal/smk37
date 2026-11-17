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
    ap.add_argument('--no-flat', action='store_true', help="no loop levels table (the loops pulse as recorded): for comparisons")
    ap.add_argument('--no-steady', action='store_true', help="the loops as recorded, their strings beating (no steady pass): for comparisons")
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
    if not a.no_steady: steady(out, smp2, keys, NKEYS)       # (out and smp2 rewritten in place, before the headers: same sizes)

    img = bytearray(struct.pack(HDR, MAGIC, nz2, 2, nr, 0, len(out), rel_gain, 0))
    for k in keys:
        img += struct.pack(KEY, *k)
    for v in vels:
        img += struct.pack(VEL, *v)
    for s in smp2 + rel2:
        img += struct.pack(SMP, *s)
    assert len(img) == hdr_len
    img += out
    if not a.no_flat: img += flat_table(out, smp2 + rel2)
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


# ---- the loops' levels evened out (engine/piano.c reads it: "FLT1", u16 count, u16 0, then 16 gains a sample)
# A loop of 0.13 to 0.5 s repeats the strings' beating and its own decay: its level rises and falls once a loop (the
# pulse players hear, 1 to 2 dB about twice a second, in Erik37's own engine too). Each loop's level is measured around
# 16 points (a circular moving average, at least 2048 samples: whole periods of the lowest notes) and a gain for each
# brings it to the level at the loop's start, where the sample enters it: the engine ramps between them, a block at a
# time. Gains are u8, 64 = 1.0, kept within 0.5 .. 2. A sample without a loop: all 64.
FILT = [(0, 0), (60, 0), (122, -60), (115, -52), (126, -63), (124, -61), (118, -58), (108, -52), (98, -55), (88, -50),
        (76, -46), (60, -40), (40, -36), (16, -32), (30, 0), (-30, -24)]          # (engine/erk_codec.h erk_filt)
NFLT = 16


def decode(data, off, n):
    """the first n samples of a block-ADPCM sample at data[off:] (engine/erk_codec.h), as floats"""
    out, s1, s2, p = [], 0, 0, off
    while len(out) < n:
        h = data[p]; c1, c2 = FILT[h >> 4]; sh = h & 15
        for i in range(32):
            q = data[p + 1 + (i >> 1)]; q = (q >> 4) if i & 1 else (q & 15)
            s = ((c1 * s1 + c2 * s2 + 32) >> 6) + ((q ^ 8) - 8) * (1 << sh)
            s = -32768 if s < -32768 else 32767 if s > 32767 else s
            s2, s1 = s1, s; out.append(float(s))
        p += 17
    return out[:n]


def flat_table(data, smps):
    import math
    t = bytearray(b'FLT1' + struct.pack('<HH', len(smps), 0))
    worst = 0.0
    for s in smps:
        off, ln, loop, llen = s[0], s[1], s[2], s[3]
        if not llen:
            t += bytes([64] * NFLT); continue
        x = decode(data, off, loop + llen)[loop:]
        sq = [v * v for v in x]; n = len(sq); w = max(min(n, 2048), n // 8)
        pre = [0.0]
        for v in sq + sq:
            pre.append(pre[-1] + v)                       # (circular: the loop twice)
        def lvl(c):                                       # the RMS of w samples centred on c, round the loop
            a = (c - w // 2) % n
            return math.sqrt(max(pre[a + w] - pre[a], 1e-9) / w)
        ref = lvl(0)
        for k in range(NFLT):
            g = ref / lvl(k * n // NFLT)
            g = 0.5 if g < 0.5 else 2.0 if g > 2.0 else g
            worst = max(worst, abs(20 * math.log10(g)))
            t.append(int(round(g * 64)))
    print('mkpiano: loops evened out (the largest correction %.1f dB)' % worst)
    return t


# ---- the loops made steady (the chorus players hear): a loop is periodic, so all it holds lies on the lines k / T of
# its length T. A partial's two or three strings, slightly apart, are two or three neighbouring lines there: they beat,
# and the loop repeats that beating 2 to 7 times a second, the partial swelling and fading (a chorus, notes that seem
# to waver). Each partial's cluster of lines is merged into its strongest line (the cluster's energy kept, its phase):
# every partial then sounds at one steady level. Lines a cluster: within 0.3 of the sample's fundamental. The attack
# before the loop is left as recorded, crossfaded over its last 60 ms into the steady loop (extended backwards, as it
# is periodic: the loop's own junction needs no crossfade). Re-encoded with Erik37's encoder (tools/erkcodec.c), the
# decoder's history at the loop's start taken from its exact decode. Sizes unchanged.
def steady(data, smps, keys, nkeys):
    import math, subprocess, tempfile
    import numpy as np
    here = os.path.dirname(os.path.abspath(__file__))
    tmp = tempfile.mkdtemp(prefix='mkpiano-')
    codec = os.path.join(tmp, 'erkcodec')
    subprocess.check_call(['cc', '-O2', '-o', codec, os.path.join(here, 'erkcodec.c'), '-lm'])
    nl = 2
    roots = {}
    for k, kk in enumerate(keys):                         # each zone's root, from a key's inc and its sample's rate
        z, inc = kk[10], kk[5]
        if z not in roots and inc:
            rate = smps[z * nl][6]
            roots[z] = 21 + k - 12 * math.log2(inc * 44100.0 / (65536.0 * rate))
    merged = 0
    for i, s in enumerate(smps):
        off, ln, loop, llen, h1, h2, rate, gain = s
        if not llen:
            continue
        x = np.array(decode(data, off, loop + llen))
        L = x[loop:loop + llen]; N = len(L)
        X = np.fft.rfft(L); mag = np.abs(X); df = rate / N
        f0 = 440.0 * 2 ** ((roots.get(i // nl, 60) - 69) / 12)
        w = max(1, int(0.3 * f0 / df))
        Y = np.zeros_like(X); taken = np.zeros(len(X), bool)
        for b in np.argsort(-mag):
            if taken[b]:
                continue
            lo, hi = max(0, b - w), min(len(X), b + w + 1)
            free = ~taken[lo:hi]
            e = np.sum(mag[lo:hi][free] ** 2)
            Y[b] = math.sqrt(e) * (X[b] / mag[b] if mag[b] > 0 else 1.0)
            taken[lo:hi] = True; merged += int(np.count_nonzero(free)) - 1
        y = np.fft.irfft(Y, N)
        R_ = min(loop, int(0.06 * rate))                  # the attack's end into the steady loop
        z = np.concatenate([x[:loop], y])
        if R_ > 0:
            a = 0.5 - 0.5 * np.cos(np.pi * (np.arange(R_) + 1) / R_)
            ext = y[(np.arange(R_) - R_) % N]
            z[loop - R_:loop] = (1 - a) * x[loop - R_:loop] + a * ext
        pk = np.max(np.abs(z)); sc = min(1.0, 32000.0 / pk) if pk > 0 else 1.0
        z16 = np.clip(np.round(z * sc), -32768, 32767).astype('<i2')
        z16.tofile(os.path.join(tmp, 'in.s16'))
        subprocess.check_call([codec, 'enc', os.path.join(tmp, 'in.s16'), os.path.join(tmp, 'o.bin'), os.path.join(tmp, 'd.s16')])
        enc = open(os.path.join(tmp, 'o.bin'), 'rb').read(); dec = np.fromfile(os.path.join(tmp, 'd.s16'), '<i2')
        nbytes = (loop + llen + 31) // 32 * 17
        assert len(enc) == nbytes, (len(enc), nbytes)
        data[off:off + nbytes] = enc
        g = int(round(gain / sc)); g = 65535 if g > 65535 else g
        smps[i] = [off, ln, loop, llen, int(dec[loop - 1]) if loop else 0, int(dec[loop - 2]) if loop > 1 else 0, rate, g]
    print('mkpiano: loops made steady (%d lines merged into their partials)' % merged)


if __name__ == '__main__':
    main()
