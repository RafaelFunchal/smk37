#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The factory sample bank of the SAMPLE and GRAIN engines and of the sampled drum kit: SLOOP 2.8.5's sets, made
# here from the same CC0 recordings (SLOOP's repository, assets/samples-cc0; see their CREDITS) as SLOOP describes
# them (docs/sloop/SPEC-MOTEURS.md: their lengths, loops, levels, the GM kit's map and shaping), into SLOOP 3's own
# bank format (engine/sample.h). Written for this project.
#   tools/mksamples.py <assets/samples-cc0> build/samples.bin [--c engine/sample_bank.c]
# The rates are SLOOP's (22.05 kHz, the metal 32 kHz): the bank is ~160 KB (its place: docs/PLAN-FINAL.md).
import hashlib, math, os, re, struct, sys, wave, zlib
import numpy as np

CC0 = sys.argv[1]
OUT = sys.argv[2]
SETS = [("PIANO", "piano"), ("BASS", "oneshot"), ("VIBES", "oneshot"), ("HORNS", "oneshot"), ("STRGS", "oneshot"),
        ("FLUTE", "sus"), ("SCRCH", "oneshot")]
# 3.3.1: the bank as it really is (each sound its own data: before, sounds were shared by mistake, below) is 280 KB at
# SLOOP's rates; its place in the flash holds ~200 KB. So: the bass at 11 kHz, the tonal sets and the kit's drums at
# 16 kHz, the metal at 24 kHz, the bright small percussion kept at 22 kHz, and some sets a little shorter.
RATE = {"PIANO": 16000, "BASS": 11025, "VIBES": 16000, "HORNS": 16000, "STRGS": 16000, "FLUTE": 16000, "SCRCH": 22050,
        "KIT": 16000, "KITHI": 24000, "KITBRIGHT": 22050}
KEEP = {"oneshot": 1.0, "sus": 0.95, "piano": 1.0}
KEEP_SET = {"PIANO": 0.8, "BASS": 0.8, "VIBES": 0.7, "FLUTE": 0.65}       # seconds (else the kind's)
ONE_SOUND = {"SCRCH"}
NOTE = {"C": 0, "C#": 1, "Cs": 1, "D": 2, "D#": 3, "Ds": 3, "E": 4, "F": 5, "F#": 6, "Fs": 6, "G": 7, "G#": 8, "Gs": 8,
        "A": 9, "A#": 10, "As": 10, "B": 11}
# the GM drum map: role -> [(lo, hi, root)] (the toms skip the hats' 42 44 46)
GM_KIT = {"kick": [(35, 36, 36)], "rim": [(37, 37, 37)], "snare": [(38, 38, 38), (40, 40, 40)], "clap": [(39, 39, 39)],
          "chh": [(42, 42, 42), (44, 44, 44)], "ohh": [(46, 46, 46)], "tomlo": [(41, 41, 43), (43, 43, 43), (45, 45, 43)],
          "tomhi": [(47, 48, 48), (50, 50, 48)], "crash": [(49, 49, 49), (52, 52, 52), (55, 55, 55), (57, 57, 57)],
          "ride": [(51, 51, 51), (53, 53, 53), (59, 59, 59)], "tamb": [(54, 54, 54)], "cowbell": [(56, 56, 56)],
          "conga": [(62, 64, 63)], "shaker": [(69, 70, 70), (82, 82, 82)], "claves": [(75, 75, 75)], "wood": [(76, 77, 76)]}
ROLE_WORDS = [("bdrum", "kick"), ("kick", "kick"), ("snare", "snare"), ("hitc", "chh"), ("hito", "ohh"), ("clap", "clap"),
              ("toml", "tomlo"), ("tomh", "tomhi"), ("rim", "rim"), ("sidestick", "rim"), ("cowbell", "cowbell"), ("tamb", "tamb"),
              ("shaker", "shaker"), ("conga", "conga"), ("claves", "claves"), ("wood", "wood"), ("crash", "crash"), ("ride", "ride")]
GM_KEEP = {"crash": 0.5, "ride": 0.5, "ohh": 0.5}
GM_HI = {"chh", "ohh", "crash", "ride"}                         # the metal keeps its top: the higher rate
GM_BRIGHT = {"rim", "shaker", "tamb", "claves", "wood"}           # small and bright: SLOOP's rate
GM_SHAPE = {"kick": (0.03, 0.13), "tomlo": (0.04, 0.22), "tomhi": (0.04, 0.2)}
GM_FADE = {"crash": 0.16, "ride": 0.16}
GM_GAIN = {"chh": -9, "ohh": -9, "crash": -8, "ride": -9, "shaker": -8, "tamb": -6, "conga": -4, "cowbell": -4, "claves": -4}

def read_wav(p):
    w = wave.open(p)
    n, ch, sw, sr = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
    raw = w.readframes(n)
    if sw == 2: x = np.frombuffer(raw, '<i2').astype(np.float64) / 32768.0
    elif sw == 3:
        b = np.frombuffer(raw, np.uint8).reshape(-1, 3)
        x = (b[:, 0].astype(np.int32) | b[:, 1].astype(np.int32) << 8 | b[:, 2].astype(np.int32) << 16)
        x = np.where(x >= 1 << 23, x - (1 << 24), x).astype(np.float64) / (1 << 23)
    elif sw == 4: x = np.frombuffer(raw, '<i4').astype(np.float64) / 2147483648.0
    else: x = (np.frombuffer(raw, np.uint8).astype(np.float64) - 128.0) / 128.0
    return sr, x.reshape(-1, ch).mean(axis=1)
def onset(x):
    pk = np.max(np.abs(x)) + 1e-12
    i = np.argmax(np.abs(x) > 0.02 * pk)
    return max(0, int(i) - 16)
def resample(x, sr, tr):
    if sr == tr: return x
    n = int(len(x) * tr / sr)
    t = np.arange(n) * (sr / tr)
    # a windowed-sinc low-pass first when going down (no aliasing), then linear reading
    if tr < sr:
        fc = 0.45 * tr / sr; k = np.arange(-32, 33); h = np.sinc(2 * fc * k) * np.hanning(65); h /= h.sum()
        x = np.convolve(x, h, mode='same')
    i = np.floor(t).astype(int); f = t - i; i1 = np.minimum(i + 1, len(x) - 1)
    return x[i] * (1 - f) + x[i1] * f
def root_of(name):
    m = re.search(r"_m(\d+)\.wav$", name)
    if m: return int(m.group(1))
    m = re.search(r"_([A-G][#s]?)(-?\d)", name)
    return (int(m.group(2)) + 1) * 12 + NOTE[m.group(1)] if m else 60
def best_loop(x, le, xf, lo, hi):
    tail = x[le - xf:le:4]; tn = np.sqrt((tail * tail).sum()) + 1e-9; best, bl = -2.0, (lo + hi) // 2
    for ls in range(lo, hi, 2):
        seg = x[ls - xf:ls:4]
        c = (seg * tail).sum() / ((np.sqrt((seg * seg).sum()) + 1e-9) * tn)
        if c > best: best, bl = c, ls
    return bl

def set_entries(name, kind):
    d = os.path.join(CC0, name); files = sorted(f for f in os.listdir(d) if f.endswith('.wav'))
    if name in ONE_SOUND: files = files[:1]
    tr, out = RATE[name], []
    for f in files:
        sr, x = read_wav(os.path.join(d, f))
        x = resample(x[onset(x):], sr, tr)
        x = x[:int(KEEP_SET.get(name, KEEP[kind]) * tr)].copy(); n = len(x)
        loop = None
        if kind == "piano":                                     # the attack, then a steady tone the envelope fades
            le, xf, a0, w = n - 1, int(0.08 * tr), int(0.30 * tr), int(0.02 * tr)
            env = np.array([max(1e-6, math.sqrt(np.mean(x[max(0, i - w):i + w] ** 2))) for i in range(0, n, w)])
            ref = env[a0 // w]
            g = np.array([ref / env[min(len(env) - 1, i // w)] if i >= a0 else 1.0 for i in range(n)])
            x = x * g
            ls = best_loop(x, le, xf, int(0.45 * tr), int(0.65 * tr))
            a = np.arange(xf) / xf; x[le - xf:le] = x[le - xf:le] * (1 - a) + x[ls - xf:ls] * a
            loop = (ls, le)
        elif kind == "sus":
            ls, le, xf = int(0.40 * tr), n - 1, int(0.06 * tr)
            a = np.arange(xf) / xf; x[le - xf:le] = x[le - xf:le] * (1 - a) + x[ls - xf:ls] * a
            loop = (ls, le)
        else:
            fade = int(0.08 * tr); x[n - fade:] *= 1 - np.arange(fade) / fade
        x = x * (30000.0 / 32768.0 / (np.max(np.abs(x)) + 1e-12))
        out.append(dict(x=x, rate=tr, root=root_of(f), loop=loop))
    out.sort(key=lambda e: e['root'])
    roots = [e['root'] for e in out]                            # the keys between two roots: half each
    for i, e in enumerate(out):
        e['lo'] = 0 if i == 0 else (roots[i - 1] + roots[i]) // 2 + 1
        e['hi'] = 127 if i == len(out) - 1 else (roots[i] + roots[i + 1]) // 2
    return out

def kit_entries():
    d = os.path.join(CC0, "KIT"); src = {}
    for f in sorted(os.listdir(d)):
        n = f.lower()
        r = next((r for w, r in ROLE_WORDS if w in n), None)
        if r and r not in src: src[r] = os.path.join(d, f)
    out = []
    for role, path in src.items():
        tr = RATE["KITHI"] if role in GM_HI else RATE["KITBRIGHT"] if role in GM_BRIGHT else RATE["KIT"]
        sr, x = read_wav(path)
        x = resample(x[onset(x):], sr, tr)
        x = x[:int(GM_KEEP.get(role, 0.4) * tr)].copy()
        if role in GM_SHAPE:
            hold, tau = GM_SHAPE[role]; h = int(hold * tr); i = np.arange(len(x))
            x = x * np.where(i < h, 1.0, np.exp(-(i - h) / (tau * tr)))
        pk = np.max(np.abs(x)) + 1e-12; end = len(x)
        while end > 64 and abs(x[end - 1]) < 0.004 * pk: end -= 1
        x = x[:end].copy()
        fade = min(len(x) // 4, int(GM_FADE.get(role, 0.03) * tr)); x[len(x) - fade:] *= 1 - np.arange(fade) / fade
        x = x * (30000.0 / 32768.0 / pk) * 10 ** (GM_GAIN.get(role, 0) / 20)
        for lo, hi, root in GM_KIT[role]:
            out.append(dict(x=x, rate=tr, root=root, loop=None, lo=lo, hi=hi, role=role))
    return out

# IMA ADPCM (the standard tables), each zone from the decoder's rest; its state kept at the loop start
STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
        143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282,
        1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
        10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767]
IDX = [-1, -1, -1, -1, 2, 4, 6, 8]
def ima(x, ls):
    s = np.clip(np.round(x * 32767.0), -32768, 32767).astype(int)
    pred, idx, nib, st = 0, 0, [], (0, 0)
    for n, v in enumerate(s):
        if n == ls: st = (pred, idx)
        step = STEP[idx]; d = v - pred; code = 0
        if d < 0: code = 8; d = -d
        vd = step >> 3
        if d >= step: code |= 4; d -= step; vd += step
        if d >= step >> 1: code |= 2; d -= step >> 1; vd += step >> 1
        if d >= step >> 2: code |= 1; vd += step >> 2
        pred = max(-32768, min(32767, pred - vd if code & 8 else pred + vd))
        idx = max(0, min(88, idx + IDX[code & 7])); nib.append(code)
    if len(nib) & 1: nib.append(0)
    return bytes(nib[i] | nib[i + 1] << 4 for i in range(0, len(nib), 2)), st

sets, zones, blob, cache = [], [], bytearray(), {}
def add_zone(e):
    # the same sound written once (the GM kit's notes sharing a recording), known by its samples and its loop: not by
    # id(), which Python gives again to a new array once the old one is gone (3.3.1: zones played other sets' sounds)
    n = len(e['x']); ls, le = e['loop'] if e['loop'] else (n - 1, n - 1)
    key = (hashlib.sha1(np.ascontiguousarray(e['x'], dtype=np.float64).tobytes()).hexdigest(), ls)
    if key not in cache:
        data, st = ima(e['x'], ls); cache[key] = (len(blob), st); blob.extend(data)
        if len(blob) & 1: blob.append(0)
    off, st = cache[key]
    zones.append(struct.pack('<IIIIIhhBBBB', off, n, ls, min(le, n - 1), int(round(e['rate'] / 44100 * 65536)), e['root'] * 16,
                             st[0], st[1], e['lo'], e['hi'], 1 if e['loop'] else 0))
for name, kind in SETS:
    es = set_entries(name, kind); z0 = len(zones)
    for e in es: add_zone(e)
    sets.append((name, z0, len(zones) - z0, kind))
es = kit_entries(); z0 = len(zones)
for e in es: add_zone(e)
sets.append(("PERC", z0, len(zones) - z0, "kit"))
KIND = {"piano": 0, "oneshot": 1, "sus": 2, "kit": 3}
# the seek index (GRAIN reads a zone anywhere): the decoder's state every 1024 samples of every zone, from its start
def seek_states(off, n):
    pred, idx, out = 0, 0, []
    for k in range(n):
        if k % 1024 == 0: out.append((pred & 0xFFFF) | idx << 16)
        b = blob[off + (k >> 1)]; code = b >> 4 if k & 1 else b & 15
        step = STEP[idx]; vd = step >> 3
        if code & 4: vd += step
        if code & 2: vd += step >> 1
        if code & 1: vd += step >> 2
        pred = max(-32768, min(32767, pred - vd if code & 8 else pred + vd))
        idx = max(0, min(88, idx + IDX[code & 7]))
    return out
seek_first, seek = [], []
for zb in zones:
    off, n = struct.unpack_from('<II', zb, 0)
    seek_first.append(len(seek)); seek += seek_states(off, n)
head = 32 + 16 * len(sets) + 28 * len(zones) + 4 * len(zones) + 4 * len(seek)
head = (head + 255) & ~255
body = (b''.join(struct.pack('<8sHHB3x', n.encode()[:8], z, c, KIND[k]) for n, z, c, k in sets) + b''.join(zones) +
        b''.join(struct.pack('<I', x) for x in seek_first) + b''.join(struct.pack('<I', x) for x in seek))
crc = zlib.crc32(bytes(blob)) & 0xFFFFFFFF
hdr = struct.pack('<IHHHHIIIII', 0x42533353, 4, len(sets), len(zones), 0, head, len(blob), crc, len(seek), 0)   # "S3SB" 2
img = hdr + body
img += b'\xFF' * (head - len(img)) + bytes(blob)
open(OUT, 'wb').write(img)
print('samples: %d sets, %d zones, %d seek points, %d bytes of ADPCM, %d bytes in all' % (len(sets), len(zones), len(seek), len(blob), len(img)), file=sys.stderr)
