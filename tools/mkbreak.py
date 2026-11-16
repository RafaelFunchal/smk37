#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The SLICE engine's built-in BREAK: one bar of sixteenths at 120 BPM, played here with the bank's own acoustic kit
# (the PERC set: VCSL's CC0 drums, assets/CREDITS-SAMPLES.txt), mixed to mono, encoded as the bank's other zones
# (IMA ADPCM, a seek point every 1024 samples) and added to the bank as its last set, BREAK (kind 4: not one of
# SAMPLE's or GRAIN's sets, engine/sample.h). Written for this project; the pattern is our own.
#   tools/mkbreak.py assets/samples.bin OUT.bin [RATE]
import struct, sys, zlib
import numpy as np

SRC, OUT = sys.argv[1], sys.argv[2]
RATE = int(sys.argv[3]) if len(sys.argv) > 3 else 16000
BPM, STEPS = 120, 16
STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
        157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
        1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
        12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767]
IDX = [-1, -1, -1, -1, 2, 4, 6, 8]

img = open(SRC, 'rb').read()
magic, ver, nsets, nzones, _, data_off, data_len, crc, nseek, _ = struct.unpack_from('<IHHHHIIIII', img, 0)
assert magic == 0x42533353 and ver in (2, 3, 4)
sets = [struct.unpack_from('<8sHHB3x', img, 32 + 16 * i) for i in range(nsets)]
assert not any(s[0].rstrip(b'\0') == b'BREAK' for s in sets), 'the bank has its BREAK already'
zat = 32 + 16 * nsets
zones = [img[zat + 28 * i: zat + 28 * i + 28] for i in range(nzones)]
first = list(struct.unpack_from('<%dI' % nzones, img, zat + 28 * nzones))
seek = list(struct.unpack_from('<%dI' % nseek, img, zat + 32 * nzones))
blob = bytearray(img[data_off: data_off + data_len])
assert zlib.crc32(bytes(blob)) & 0xFFFFFFFF == crc

def decode(off, n):
    pred, idx, out = 0, 0, np.zeros(n)
    for k in range(n):
        b = blob[off + (k >> 1)]; code = b >> 4 if k & 1 else b & 15
        step = STEP[idx]; vd = step >> 3
        if code & 4: vd += step
        if code & 2: vd += step >> 1
        if code & 1: vd += step >> 2
        pred = max(-32768, min(32767, pred - vd if code & 8 else pred + vd))
        idx = max(0, min(88, idx + IDX[code & 7])); out[k] = pred / 32768.0
    return out

kit = [s for s in sets if s[0].rstrip(b'\0') == b'PERC'][0]
def hit(note):                                    # the kit's sound for a GM note, at RATE
    for z in range(kit[1], kit[1] + kit[2]):
        off, n, ls, le, rate, root16, pred, idx, lo, hi, looped = struct.unpack('<IIIIIhhBBBB', zones[z])
        if lo <= note <= hi:
            x = decode(off, n)
            src = rate / 65536 * 44100
            t = np.arange(0, len(x) - 1, src / RATE)
            return np.interp(t, np.arange(len(x)), x)
    raise SystemExit('no kit sound for note %d' % note)
def norm(x): return x / (np.max(np.abs(x)) + 1e-12)              # (3.3.1: each hit at full, the bar's levels below; the bank's
K, S, H, O, R = (norm(hit(n)) for n in (36, 38, 42, 46, 37))   #  kit keeps its hats 9 dB down, which left the hat slices silent)
# the bar: (step, sound, level). Kick on 1 and the "and" of 2, a ghost-noted backbeat, hats on every sixteenth with
# their accents, an open hat into the last beat
PAT = [(0, K, 1.0), (3, K, 0.55), (6, K, 0.85), (10, K, 0.9), (11, K, 0.5),
       (4, S, 1.0), (12, S, 1.0), (7, S, 0.22), (9, S, 0.18), (15, S, 0.25), (14, R, 0.35)]
for s in range(STEPS):
    if s == 13: PAT.append((s, O, 0.55))
    elif s != 14: PAT.append((s, H, 0.62 if s % 4 == 0 else 0.45 if s % 2 == 0 else 0.3))
step_n = RATE * 60 / BPM / 4
L = int(round(step_n * STEPS))
bar = np.zeros(L + RATE)
for s, x, g in PAT:
    a = int(round(s * step_n)); bar[a: a + len(x)] += g * x
tail = bar[L:]; bar = bar[:L]; bar[:len(tail)] += tail[:L] * 0.0   # (one bar: what rings past it is cut, not wrapped)
bar = np.tanh(bar * 1.4) / np.tanh(1.4)                             # (a little glue)
bar *= 0.89 / np.max(np.abs(bar))
fade = int(0.003 * RATE); bar[-fade:] *= np.linspace(1, 0, fade)

def ima(x):
    s = np.clip(np.round(x * 32767.0), -32768, 32767).astype(int)
    pred, idx, nib = 0, 0, []
    for v in s:
        step = STEP[idx]; d = v - pred; code = 0
        if d < 0: code = 8; d = -d
        vd = step >> 3
        if d >= step: code |= 4; d -= step; vd += step
        if d >= step >> 1: code |= 2; d -= step >> 1; vd += step >> 1
        if d >= step >> 2: code |= 1; vd += step >> 2
        pred = max(-32768, min(32767, pred - vd if code & 8 else pred + vd))
        idx = max(0, min(88, idx + IDX[code & 7])); nib.append(code)
    if len(nib) & 1: nib.append(0)
    return bytes(nib[i] | nib[i + 1] << 4 for i in range(0, len(nib), 2))
data = ima(bar)
off = len(blob); blob.extend(data)
if len(blob) & 1: blob.append(0)
n = len(bar)
zones.append(struct.pack('<IIIIIhhBBBB', off, n, n - 1, n - 1, int(round(RATE / 44100 * 65536)), 60 * 16, 0, 0, 0, 127, 0))
sets.append((b'BREAK', nzones, 1, 4))
# its seek points: the decoder's state every 1024 samples
pred, idx, sk = 0, 0, []
for k in range(n):
    if k % 1024 == 0: sk.append((pred & 0xFFFF) | idx << 16)
    b = blob[off + (k >> 1)]; code = b >> 4 if k & 1 else b & 15
    step = STEP[idx]; vd = step >> 3
    if code & 4: vd += step
    if code & 2: vd += step >> 1
    if code & 1: vd += step >> 2
    pred = max(-32768, min(32767, pred - vd if code & 8 else pred + vd))
    idx = max(0, min(88, idx + IDX[code & 7]))
first.append(len(seek)); seek += sk
nz = len(zones)
head = 32 + 16 * len(sets) + 28 * nz + 4 * nz + 4 * len(seek)
head = (head + 255) & ~255
body = (b''.join(struct.pack('<8sHHB3x', nm[:8], z, c, k) for nm, z, c, k in sets) + b''.join(zones) +
        b''.join(struct.pack('<I', x) for x in first) + b''.join(struct.pack('<I', x) for x in seek))
crc = zlib.crc32(bytes(blob)) & 0xFFFFFFFF
hdr = struct.pack('<IHHHHIIIII', 0x42533353, 4, len(sets), nz, 0, head, len(blob), crc, len(seek), 0)
out = hdr + body
out += b'\xFF' * (head - len(out)) + bytes(blob)
open(OUT, 'wb').write(out)
# (and the bar as a WAV, to hear it)
import wave
w = wave.open(OUT + '.wav', 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
w.writeframes((np.clip(bar, -1, 1) * 32767).astype('<i2').tobytes()); w.close()
print('BREAK: %d samples at %d Hz (%d bytes); the bank: %d sets, %d zones, %d bytes' % (n, RATE, len(data), len(sets), nz, len(out)), file=sys.stderr)
