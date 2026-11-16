#!/usr/bin/env python3
"""The instruments, key by key (host/twin.c `inst`): each hit held against the CC0 recordings the bank is made from.
   python3 -I tools/inst_check.py build/tw/inst SAMPLES_CC0_DIR"""
import sys, os, wave, re
import numpy as np

D, SRC = sys.argv[1], sys.argv[2]
NAMES = ["BOOMBAP", "808", "ACOUSTIC", "GM KIT", "GRAND PNO", "UP BASS", "VIBES", "HORN STAB", "STRING STB", "LOFI FLUTE", "SCRATCH"]
SETS = {"GRAND PNO": "PIANO", "UP BASS": "BASS", "VIBES": "VIBES", "HORN STAB": "HORNS", "STRING STB": "STRGS", "LOFI FLUTE": "FLUTE", "SCRATCH": "SCRCH"}
PADS = ["KICK", "KICK2", "SNARE", "CLAP", "HAT", "OPEN HAT", "PEDAL", "RIM", "SNARE2", "LOW TOM", "HI TOM", "CRASH", "RIDE", "SHAKER", "CONGA", "COWBELL"]
WHITE = [k for k in range(27) if (53 + k) % 12 in (0, 2, 4, 5, 7, 9, 11)]

def load(fn):
    w = wave.open(fn); sr = w.getframerate(); n = w.getnframes(); ch = w.getnchannels(); sw = w.getsampwidth()
    b = w.readframes(n)
    if sw == 2: a = np.frombuffer(b, '<i2').astype(np.float64) / 32768
    elif sw == 3:
        x = np.frombuffer(b, np.uint8).reshape(-1, 3); a = ((x[:, 0].astype(np.int32) | x[:, 1].astype(np.int32) << 8 | x[:, 2].astype(np.int32) << 16) << 8 >> 8) / 8388608.0
    elif sw == 4: a = np.frombuffer(b, '<i4').astype(np.float64) / 2147483648.0
    else: a = (np.frombuffer(b, np.uint8).astype(np.float64) - 128) / 128
    return a.reshape(-1, ch).mean(1), sr

def hits(a, sr, n=16, gap=1.23):
    out = []
    for i in range(n):
        s = int((0.0 + i * gap) * sr); out.append(a[s: s + int(1.1 * sr)])
    return out

BANDS = np.geomspace(60, 9000, 41)
def spec(x, sr, dur=0.35, shift=0.0):
    x = x[:int(dur * sr)]
    if len(x) < 256: x = np.pad(x, (0, 256 - len(x)))
    X = np.abs(np.fft.rfft(x * np.hanning(len(x)), 16384)); f = np.fft.rfftfreq(16384, 1 / sr) * 2 ** (shift / 12)
    e = np.array([X[(f >= BANDS[i]) & (f < BANDS[i + 1])].mean() if ((f >= BANDS[i]) & (f < BANDS[i + 1])).any() else 0 for i in range(40)])
    e = np.log(e + 1e-9); return (e - e.mean()) / (e.std() + 1e-9)

def onset(x):
    m = np.abs(x); t = np.argmax(m > m.max() * 0.1) if m.max() > 1e-4 else 0
    return x[max(0, t - 32):]

def f0(x, sr):
    x = x[int(0.03 * sr): int(0.33 * sr)]
    if len(x) < 2000 or np.abs(x).max() < 1e-4: return 0
    c = np.correlate(x, x, 'full')[len(x) - 1:]; lo, hi = int(sr / 1500), int(sr / 35)
    k = lo + np.argmax(c[lo:hi]); return sr / k if c[k] > 0.3 * c[0] else 0

def centroid(x, sr):
    X = np.abs(np.fft.rfft(x[:int(0.3 * sr)], 16384)) ** 2; f = np.fft.rfftfreq(16384, 1 / sr)
    return float((X * f).sum() / (X.sum() + 1e-12))

def decay(x, sr):
    e = np.convolve(x ** 2, np.ones(int(0.01 * sr)) / int(0.01 * sr), 'same'); p = e.max()
    if p <= 0: return 0
    k = np.argmax(e); after = np.where(e[k:] < p * 1e-3)[0]
    return (after[0] if len(after) else len(e) - k) / sr

# the sources
def root_of(fn):
    m = re.search(r'm(\d+)', fn); return int(m.group(1)) if m else None
src = {}
for st in os.listdir(SRC):
    p = os.path.join(SRC, st)
    if not os.path.isdir(p): continue
    for fn in sorted(os.listdir(p)):
        if fn.endswith('.wav'):
            a, sr = load(os.path.join(p, fn)); src.setdefault(st, []).append((fn, root_of(fn), onset(a), sr))

for n, name in enumerate(NAMES):
    fn = os.path.join(D, 'inst-%02d.wav' % n)
    if not os.path.exists(fn): continue
    a, sr = load(fn); H = [onset(h) for h in hits(a, sr)]
    print('\n== %s' % name)
    if name in SETS:
        for i in (0, 4, 7, 11):                                    # F3, C4, F4, C5
            note = 53 + WHITE[i]; h = H[i]; ff = f0(h, sr)
            got = 12 * np.log2(ff / 440) + 69 if ff else None
            best = []
            for st, L in src.items():
                if st == 'KIT': continue
                fn2, r, s2, sr2 = min(L, key=lambda z: abs((z[1] or 60) - note))
                c = float(np.dot(spec(h, sr, shift=0), spec(s2, sr2, shift=note - (r or note))) / 40)
                best.append((c, st))
            best.sort(reverse=True)
            print('  key %-3s note %d: pitch %s, closest recordings %s  (want %s)' % (
                ['F3', '', '', '', 'C4', '', '', 'F4', '', '', '', 'C5'][i], note, '%.1f' % got if got else '-',
                ', '.join('%s %.2f' % (s, c) for c, s in best[:3]), SETS[name]))
    else:
        kit = src.get('KIT', [])
        for i, h in enumerate(H):
            pk = np.abs(h).max()
            line = '  %-8s peak %5.3f  centroid %6.0f Hz  decay %4.2f s  f0 %5.0f' % (PADS[i], pk, centroid(h, sr), decay(h, sr), f0(h, sr))
            if name in ('ACOUSTIC', 'GM KIT') and kit:
                sc = sorted(((float(np.dot(spec(h, sr), spec(s2, sr2)) / 40), f2) for f2, r, s2, sr2 in kit), reverse=True)
                line += '   closest: ' + ', '.join('%s %.2f' % (f2.split('_', 1)[1][:16], c) for c, f2 in sc[:2])
            print(line)
