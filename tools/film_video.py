#!/usr/bin/env python3
"""SLOOP 3 preview video, in the style of the SLOOP launch videos: the twin's film (screen, panel and sound, from
host/twin.c `film`), the FM-1 drawn with its lights and the hands' gestures, the logo drawn large by the firmware's own
code (host/logo_film.c), English cards.
   python3 -I tools/film_video.py FILM_DIR OUT_DIR [seconds...]     (with seconds: only those frames, as PNGs)"""
import sys, os, subprocess, wave
import numpy as np
from PIL import Image, ImageDraw, ImageFont

SRC, OUT = sys.argv[1], sys.argv[2]
HERE = os.path.dirname(os.path.abspath(__file__))
W, H, FPS, DUR = 1920, 1080, 30, 90.0
BG = (23, 24, 29); ORANGE = (253, 95, 24); WHITE = (240, 240, 243); GREY = (166, 167, 173); DIM = (104, 105, 112)
FI = '/usr/share/fonts/opentype/inter/'
def F(name, size): return ImageFont.truetype(FI + name, size)
F_HDR = F('Inter-SemiBold.otf', 25); F_NOW = F('Inter-Regular.otf', 23)
F_HEAD = F('Inter-Bold.otf', 64); F_BODY = F('Inter-Regular.otf', 31); F_BODYB = F('Inter-SemiBold.otf', 31)
F_CHIP = F('Inter-SemiBold.otf', 19); F_CAP = F('Inter-Regular.otf', 26); F_CAPB = F('Inter-SemiBold.otf', 26)
F_BIG = F('Inter-Bold.otf', 82); F_SUB = F('Inter-Regular.otf', 34); F_SUBB = F('Inter-SemiBold.otf', 34)
F_FOOT = F('Inter-Regular.otf', 21); F_THREE = F('Inter-Bold.otf', 150); F_KICK = F('Inter-SemiBold.otf', 27)
F_TAB = F('Inter-Regular.otf', 28); F_TABB = F('Inter-SemiBold.otf', 28)

def ease(x): x = max(0.0, min(1.0, x)); return x * x * (3 - 2 * x)
def mix(a, b, t): return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))
def rgb565(a):
    a = np.asarray(a).astype(np.uint32)
    r = ((a >> 11) & 31) * 255 // 31; g = ((a >> 5) & 63) * 255 // 63; b = (a & 31) * 255 // 31
    return np.stack([r, g, b], -1).astype(np.uint8)
def fade(im, a):
    if a >= 1: return im
    im = im.copy(); im.putalpha(im.getchannel('A').point(lambda p: int(p * a))); return im

# ---- the film
raw = np.memmap(os.path.join(SRC, 'film.rgb565'), dtype='<u2', mode='r').reshape(-1, 240, 240)
PAN = np.fromfile(os.path.join(SRC, 'film.panel'), np.uint8).reshape(-1, 128)
T0, TW = (float(x) for x in open(os.path.join(SRC, 'film.txt')).read().split())
def fidx(v): return max(0, min(len(raw) - 1, int(round((v - T0) * FPS))))
def screen(v): return rgb565(raw[fidx(v)])
def panel(v):
    p = PAN[fidx(v)]
    return dict(btn=int(p[0:4].view('<u4')[0]), keys=int(p[4:8].view('<u4')[0]), led=p[36:77], glow=p[77:118],
                turned=p[118:125].view(np.int8).astype(int))
# the tracks' colours, as the screen draws them (the numbers of the TRACKS page)
C = [(82, 161, 255), (82, 230, 123), (255, 207, 64), (236, 110, 225), (255, 146, 41)]
TR = rgb565(raw[fidx(10.0)])
for k, y in enumerate((50, 78, 106, 134, 162)):
    if int(TR[y, 6].astype(int).sum()) > 300: C[k] = tuple(int(c) for c in TR[y, 6])

# ---- the logo, large, by the firmware's own drawing (on black and on white: its colour and its coverage)
LK, LN = 3, 90; LS = 240 * LK
def logo_frames():
    exe = os.path.join(HERE, '..', 'build', 'logo_film')
    cols = ['%02x%02x%02x' % c for c in C]
    def run(bg): return np.frombuffer(subprocess.run([exe, str(LK), str(LN), *map(str, bg), *cols], check=True, capture_output=True).stdout, '<u2').reshape(LN, LS, LS)
    k = rgb565(run((0, 0, 0))).astype(np.float32); w = rgb565(run((255, 255, 255))).astype(np.float32)
    a = np.clip(1 - (w - k).mean(-1, keepdims=True) / 255.0, 0, 1)
    col = np.where(a > 0.004, k / np.maximum(a, 1e-3), 0)
    return np.concatenate([np.clip(col, 0, 255), a * 255], -1).astype(np.uint8)
LOGO = logo_frames()
def logo_img(t, size=None):
    im = Image.fromarray(LOGO[max(0, min(LN - 1, int(t * 30)))], 'RGBA')
    return im if not size or size == LS else im.resize((size, size), Image.LANCZOS)

# ---- the FM-1, drawn (units: the drawing of the earlier SLOOP videos, 808 x 485), SS times larger, then reduced
SS = 2.5
FM = '/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf'
F_PL = ImageFont.truetype(FM, int(12 * SS)); F_PB = ImageFont.truetype(FM, int(10 * SS)); F_PS = ImageFont.truetype(FM, int(8 * SS))
def s(v): return int(round(v * SS))
KNOBS = {'MASTER': (69, 78, None), 'SELECT': (156, 78, 0), 'PRESETS': (69, 162, 6), 'ALGORITHM': (156, 162, 1),
         'KNOB1': (459, 75, 2), 'KNOB2': (548, 75, 3), 'KNOB3': (638, 75, 4), 'KNOB4': (728, 75, 5)}
BTNS = [('FX', 2), ('SCL', 3), ('ENV', 4), ('LFO', 5), ('EDIT', 6), ('GLO', 7), ('HOME', 8), ('SAVE', 9), ('ARP', 10), ('SEQ', 11), ('PLAY', 12), ('REC', 13)]
BX = [469, 520, 571, 622, 673, 723]; BY = [161, 211]
WHITE_X = [64 + i * 45.87 for i in range(16)]
BLACK_X = [86, 133, 181, 271, 317, 408, 454, 500, 592, 638, 730]
KEYPOS = {}                                     # key 0..26 (F3..G5) -> (x, white)
wi = bi = 0
for k in range(27):
    if (53 + k) % 12 in (0, 2, 4, 5, 7, 9, 11): KEYPOS[k] = (WHITE_X[wi], True); wi += 1
    else: KEYPOS[k] = (BLACK_X[bi], False); bi += 1
SCR = (233, 56, 150)                            # the screen in the drawing: x, y, size

def rr(d, box, r, fill=None, outline=None, width=1):
    d.rounded_rectangle([s(box[0]), s(box[1]), s(box[2]), s(box[3])], radius=s(r), fill=fill, outline=outline, width=max(1, s(width)))

def fm1_base():
    im = Image.new('RGBA', (s(808), s(485)), (0, 0, 0, 0))
    g = np.linspace(0, 1, s(485))[:, None, None]
    body = (np.array([103, 108, 116]) * (1 - g) + np.array([86, 91, 99]) * g).repeat(s(808), 1).astype(np.uint8)
    mask = Image.new('L', im.size, 0); ImageDraw.Draw(mask).rounded_rectangle([0, 0, s(808) - 1, s(485) - 1], radius=s(40), fill=255)
    im.paste(Image.fromarray(body), (0, 0), mask)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([0, 0, s(808) - 1, s(485) - 1], radius=s(40), outline=(118, 122, 131), width=s(1.6))
    for name, (x, y, e) in KNOBS.items():
        d.text((s(x), s(y - 38)), name, font=F_PL, fill=(236, 237, 240), anchor='mm')
        d.ellipse([s(x - 24), s(y - 24), s(x + 24), s(y + 24)], fill=(30, 30, 37))
        d.ellipse([s(x - 20), s(y - 20), s(x + 20), s(y + 20)], fill=(52, 53, 60))
        for a in range(0, 360, 20):                                   # (the knurl)
            c, sn = np.cos(np.radians(a)), np.sin(np.radians(a))
            d.line([s(x + 21 * c), s(y + 21 * sn), s(x + 23.5 * c), s(y + 23.5 * sn)], fill=(20, 20, 26), width=s(1.2))
    rr(d, (47, 209, 180, 244), 8, fill=(38, 39, 44))
    rr(d, (434, 133, 758, 245), 12, fill=(38, 39, 44))
    rr(d, (210, 36, 406, 230), 22, fill=(14, 14, 17), outline=(70, 73, 80), width=1.5)
    rr(d, (28, 272, 786, 465), 30, fill=(48, 49, 56))
    for x in (225, 362, 545, 684): d.ellipse([s(x - 2), s(360), s(x + 2), s(364)], fill=(18, 18, 22))
    return im
BASE = fm1_base()

def pill(d, cx, y0, y1, w, fill, outline=None):
    d.rounded_rectangle([s(cx - w / 2), s(y0), s(cx + w / 2), s(y1)], radius=s(w / 2), fill=fill, outline=outline, width=s(2) if outline else 1)

def fm1(v, width):
    """the FM-1 at time v, `width` px wide: its screen, its lights, the hands on it"""
    st = panel(v); before = panel(v - 0.3); im = BASE.copy(); d = ImageDraw.Draw(im)
    for name, (x, y, e) in KNOBS.items():
        a = np.radians(-90 + (st['turned'][e] * 22 if e is not None else 0))
        if e is not None and st['turned'][e] != before['turned'][e]:
            d.ellipse([s(x - 27), s(y - 27), s(x + 27), s(y + 27)], outline=ORANGE, width=s(2.6))
        d.line([s(x + 5 * np.cos(a)), s(y + 5 * np.sin(a)), s(x + 17 * np.cos(a)), s(y + 17 * np.sin(a))], fill=(244, 244, 246), width=s(3.2))
    for oi, (lab, b) in enumerate([('OCT-', 0), ('OCT+', 1)]):
        x0 = 56 + oi * 66
        rr(d, (x0, 213, x0 + 48, 240), 6, fill=(107, 112, 120))
        d.text((s(x0 + 24), s(226.5)), lab, font=F_PS, fill=(200, 203, 208), anchor='mm')
    for i, (lab, b) in enumerate(BTNS):
        x, y = BX[i % 6], BY[i // 6]
        held = st['btn'] >> b & 1; lit = st['led'][b]; glow = st['glow'][b]
        face = (178, 182, 188) if lit else (124, 129, 137) if glow else (109, 114, 122)
        if held: face = mix(face, (70, 74, 80), 0.3)
        dy = 1.2 if held else 0
        rr(d, (x - 21, y - 19 + dy, x + 21, y + 19 + dy), 7, fill=face, outline=ORANGE if held else None, width=2.4)
        tc = (252, 252, 253) if lit else (205, 208, 214)
        if lab == 'PLAY':
            d.text((s(x), s(y - 6 + dy)), 'PLAY', font=F_PS, fill=tc, anchor='mm'); d.text((s(x), s(y + 6 + dy)), 'STOP', font=F_PS, fill=tc, anchor='mm')
            d.line([s(x - 12), s(y + dy), s(x + 12), s(y + dy)], fill=tc, width=s(0.8))
        else: d.text((s(x), s(y + dy)), lab, font=F_PB, fill=tc, anchor='mm')
    for k, (x, white) in KEYPOS.items():
        y0, y1 = (368, 450) if white else (296, 357)
        held = st['keys'] >> k & 1; lit = st['led'][14 + k]; glow = st['glow'][14 + k]
        face = (84, 87, 94) if white else (97, 100, 107)
        if held: face = mix(face, (150, 153, 160), 0.3)
        pill(d, x, y0, y1, 38, face, ORANGE if held else None)
        sl = 40 if white else 30; my = (y0 + y1) / 2
        if lit:
            for r, a in ((9, 0.10), (6, 0.18), (4, 0.3)):
                pill(d, x, my - sl / 2 - r, my + sl / 2 + r, 5 + 2 * r, mix(face, (255, 255, 255), a))
            pill(d, x, my - sl / 2, my + sl / 2, 5, (246, 246, 246))
        else: pill(d, x, my - sl / 2, my + sl / 2, 4.2, (172, 174, 180) if glow else mix(face, (125, 128, 135), 0.6))
    k = width / 808.0
    out = im.resize((width, int(round(485 * k))), Image.LANCZOS)
    sc = Image.fromarray(screen(v)).resize((int(round(SCR[2] * k)),) * 2, Image.LANCZOS)
    out.paste(sc, (int(round(SCR[0] * k)), int(round(SCR[1] * k))))
    return out

# ---- text, with **bold** words
def draw_rich(d, x, y, text, width, fr, fb, cr, cb, lh):
    words, bold = [], False
    for part in text.split('**'):
        toks = part.split(' ')
        for j, w in enumerate(toks):
            if j and words: words[-1][2] = True               # (a space after the word before)
            if w: words.append([w, bold, False])
        bold = not bold
    lines, line, lw = [], [], 0.0
    for w, b, sp in words:
        f = fb if b else fr; tw = d.textlength(w, font=f)
        if line and lw + tw > width: lines.append(line); line, lw = [], 0.0
        line.append((w, b, sp)); lw += tw + (d.textlength(' ', font=f) if sp else 0)
    if line: lines.append(line)
    for ln in lines:
        xx = x
        for w, b, sp in ln:
            f = fb if b else fr; d.text((xx, y), w, font=f, fill=cb if b else cr)
            xx += d.textlength(w, font=f) + (d.textlength(' ', font=f) if sp else 0)
        y += lh
    return y

def chips(d, x, y, items):
    for it in items:
        if it.startswith('['):
            t = it[1:-1]; tw = d.textlength(t, font=F_CHIP)
            d.rounded_rectangle([x, y - 2, x + tw + 20, y + 30], radius=6, fill=(44, 45, 52), outline=(78, 79, 88), width=1)
            d.text((x + 10, y + 14), t, font=F_CHIP, fill=WHITE, anchor='lm'); x += tw + 20 + 10
        else:
            d.text((x, y + 14), it, font=F_CHIP, fill=GREY, anchor='lm'); x += d.textlength(it, font=F_CHIP) + 10

def spaced(d, x, y, t, f, fill, sp):
    for ch in t: d.text((x, y), ch, font=f, fill=fill); x += d.textlength(ch, font=f) + sp
    return x

NOW = 'BOOMBAP kit · UP BASS · DUSTY PNO · SCRATCH · VIBES · 90 BPM'
SECTIONS = [  # start, end, number, title, headline, chips, paragraphs, caption
    (6.4, 13.0, '01', 'REBUILT FROM SCRATCH', 'Every line, written again', [],
     ['SLOOP 3 is **not an update**. The **engine**, the **sequencer** and the **interface** were written again, from zero.',
      'Same buttons, same layers, **all 153 sounds**. A **new interface**, in English.'],
     '**TRACKS** · five tracks, a 90s boom bap beat'),
    (13.0, 21.4, '02', 'THE SOUNDS', '13 engines, 153 sounds', ['TAP', '[EDIT]', '·', 'TURN', '[KNOB1]'],
     ['Every factory sound **rebuilt, value for value**, each on its own pages.',
      'New in SLOOP 3: **SLICE**, a sample cut across the keys.'],
     '**DUSTY PNO** · the sound’s pages'),
    (21.4, 27.6, '03', 'SOUNDS', 'Find a sound by name', ['TURN', '[PRESETS]'],
     ['Every sound by **category**, page by page. **16 favourites** come first, and **32 sounds** of your own.',
      'On the drum track: **46 kits**.'],
     '**SOUNDS** · the browser'),
    (27.6, 36.0, '04', 'THE SEQUENCER', '5 tracks, 128 steps', ['TAP', '[SEQ]'],
     ['**8 patterns** per track and **8 undos**.',
      'Step **conditions**, **chance** and **parameter locks**.',
      'Live recording kept at **1/96 of a step**.'],
     '**DRUMS** · the BOOMBAP kit’s steps'),
    (36.0, 44.7, '05', 'NOTES', 'A piano roll on the FM-1', ['TAP', '[SEQ]', '→', '[SEQ]'],
     ['Zoom from **8 to 64 steps**.',
      'Every note has its own sheet: **slide**, **nudge**, **move**, **copy**, **delete**.'],
     '**NOTES** · the dusty piano, Dm9 to A7'),
    (44.7, 56.0, '06', 'PUNCH-IN EFFECTS', '27 punch-ins, chained', ['HOLD', '[FX]', '+', '[KEY]'],
     ['**Gate**, loops of **1/16** and **1/32**, **echo**, **reverse**, **tape stop**…',
      'Hold **FX** and play them like keys, one into the next. **FX + HOME** locks the layer.'],
     '**FX** · GATE → 1/16 → ECHO → 1/32 → REV → STOP'),
    (56.0, 68.6, '07', 'THE VISUALISER', 'Twelve pictures of what plays', ['TAP', '[HOME]', '·', 'TURN', '[SELECT]'],
     ['**HOME** on the tracks screen: oscilloscope, spectrum, spectrogram, vectors, meters, tape…',
      'In your tracks’ colours, **at no cost to the sound**.'],
     '**HOME** · SELECT changes the picture'),
    (68.6, 78.6, '08', 'UNDER THE HOOD', 'A new engine underneath', [], [], '**TRACKS** · the beat goes on'),
]
TABLE = [('Sound', '32-bit float'), ('Audio blocks', '1.45 ms'), ('Voices', '32 + 8, on both cores'),
         ('Tracks', '4 synths + drums'), ('Patterns', '8 per track, up to 128 steps'), ('USB', 'audio 44.1 / 48 kHz, MIDI'),
         ('MIDI in', 'USB and TRS'), ('Web editor', 'in the browser, no install'), ('SLOOP 2.8 projects', 'come along')]
END0 = 78.6
TICKS = [sec[0] for sec in SECTIONS] + [END0]

LAYERS = {}
def layer(i):
    """a section's text, once: RGBA, the whole frame"""
    if i in LAYERS: return LAYERS[i]
    st, en, num, title, head, ch, paras, cap = SECTIONS[i]
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    x = spaced(d, 80, 44, num, F_HDR, ORANGE, 2.5); spaced(d, x + 12, 44, title, F_HDR, WHITE, 3.2)
    d.text((80, 96), head, font=F_HEAD, fill=WHITE)
    y = 192
    if ch: chips(d, 82, y, ch); y += 54
    for p in paras: y = draw_rich(d, 80, y, p, 930, F_BODY, F_BODYB, GREY, WHITE, 43) + 16
    if not paras:                                          # (the table, as in the SLOOP PRO video)
        y = 214; d.text((80, y), 'SLOOP 3', font=F_HDR, fill=DIM); y += 46
        for k, v in TABLE:
            d.line([80, y, 1000, y], fill=(52, 53, 60), width=1)
            d.text((80, y + 13), k, font=F_TABB, fill=WHITE); d.text((1000, y + 13), v, font=F_TAB, fill=GREY, anchor='ra')
            y += 64
    draw_rich(d, 1098, 912, cap, 740, F_CAP, F_CAPB, GREY, WHITE, 36)
    LAYERS[i] = im; return im

def chrome(d, v, a=1.0):
    """what stays: the playing now, the timeline"""
    c = mix(BG, GREY, a)
    tw = d.textlength(NOW, font=F_NOW); x = 1840 - tw
    d.text((x, 45), NOW, font=F_NOW, fill=c)
    for i, hgt in enumerate((10, 16, 12, 18)): d.rounded_rectangle([x - 34 + i * 6, 68 - hgt, x - 30 + i * 6, 68], radius=2, fill=c)
    d.line([80, 1063, 1840, 1063], fill=mix(BG, (44, 45, 52), a), width=3)
    px = 80 + (1840 - 80) * min(1.0, v / DUR)
    d.line([80, 1063, px, 1063], fill=mix(BG, (125, 126, 132), a), width=3)
    for t in TICKS:
        tx = 80 + (1840 - 80) * t / DUR; d.line([tx, 1057, tx, 1069], fill=mix(BG, (90, 91, 98), a), width=2)

def big_screen(img, v):
    x, y, S = 1098, 150, 720
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([x - 22, y - 22, x + S + 22, y + S + 22], radius=30, fill=(13, 13, 16), outline=(62, 63, 71), width=2)
    img.paste(Image.fromarray(screen(v)).resize((S, S), Image.NEAREST), (x, y))

HW_W, HW_X, HW_Y = 880, 60, 518
def section_frame(v):
    img = Image.new('RGB', (W, H), BG); d = ImageDraw.Draw(img)
    chrome(d, v)
    big_screen(img, v)
    hw_a = 1 - ease((v - (SECTIONS[-1][0] - 0.3)) / 0.5)          # (the drawing makes room for the table)
    if hw_a > 0:
        hw = fade(fm1(v, HW_W), hw_a); img.paste(hw, (HW_X, HW_Y), hw)
    for i, sec in enumerate(SECTIONS):
        st, en = sec[0], sec[1]
        a_in = 1.0 if i == 0 else ease((v - st) / 0.4)
        a_out = 1.0 if i == len(SECTIONS) - 1 else 1 - ease((v - (en - 0.35)) / 0.35)
        if st - 0.01 <= v < en + 0.01 and min(a_in, a_out) > 0:
            L = fade(layer(i), min(a_in, a_out)); img.paste(L, (0, 0), L)
    return img

def intro_frame(v):
    img = Image.new('RGB', (W, H), BG); d = ImageDraw.Draw(img)
    lg = logo_img(v); img.paste(lg, (600, 40), lg)
    a = ease((v - 2.6) / 0.5)
    if a > 0:
        d.text((1170, 614 - 6 * (1 - a)), '3', font=F_THREE, fill=mix(BG, ORANGE, a), anchor='ls')
        t = 'A FIRST LOOK'; x = (W - (d.textlength(t, font=F_KICK) + 3.5 * len(t))) / 2
        spaced(d, x, 762 + 10 * (1 - a), t, F_KICK, mix(BG, WHITE, a), 3.5)
    b = ease((v - 3.2) / 0.5)
    if b > 0:
        t = 'Free firmware for the M-VAVE FM-1'
        d.text(((W - d.textlength(t, font=F_SUB)) / 2, 818 + 10 * (1 - b)), t, font=F_SUB, fill=mix(BG, GREY, b))
    chrome(d, v, ease((v - 3.2) / 0.5))
    return img

def end_frame(v):
    img = Image.new('RGB', (W, H), BG); d = ImageDraw.Draw(img)
    chrome(d, v, 1 - ease((v - END0) / 0.6))
    a = ease((v - END0 - 0.5) / 0.6); b = ease((v - END0 - 1.3) / 0.6); c = ease((v - END0 - 2.2) / 0.6)
    if a > 0:
        lg = fade(logo_img(5.0, 330), a); img.paste(lg, (795, 90), lg)
        t = 'Release date: not yet known.'
        d.text(((W - d.textlength(t, font=F_BIG)) / 2, 460 + 12 * (1 - a)), t, font=F_BIG, fill=mix(BG, WHITE, a))
    if b > 0:
        y = 596
        for t, f, col in (('Everything was rebuilt, so everything is being re-tested,', F_SUB, GREY),
                          ('rigorously, on real FM-1s.', F_SUB, GREY), ('SLOOP 3 comes out when it is ready.', F_SUBB, WHITE)):
            d.text(((W - d.textlength(t, font=f)) / 2, y + 10 * (1 - b)), t, font=f, fill=mix(BG, col, b)); y += 50
    if c > 0:
        t1, t2 = 'SLOOP 2.8, today: ', 'isod89.github.io/sloop-fm1'
        w1, w2 = d.textlength(t1, font=F_SUB), d.textlength(t2, font=F_SUBB); x = (W - w1 - w2) / 2
        d.text((x, 820), t1, font=F_SUB, fill=mix(BG, GREY, c)); d.text((x + w1, 820), t2, font=F_SUBB, fill=mix(BG, ORANGE, c))
        t = 'Filmed on the PC twin: the SLOOP 3 firmware on a simulated FM-1, with its own screen and sound.'
        d.text(((W - d.textlength(t, font=F_FOOT)) / 2, 975), t, font=F_FOOT, fill=mix(BG, DIM, c))
    return img

def frame(v):
    if v < 5.7: img = intro_frame(v)
    elif v < SECTIONS[0][0]: img = Image.blend(intro_frame(v), section_frame(SECTIONS[0][0]), ease((v - 5.7) / 0.7))
    elif v < END0: img = section_frame(v)
    elif v < END0 + 0.6: img = Image.blend(section_frame(v), end_frame(v), ease((v - END0) / 0.6))
    else: img = end_frame(v)
    if v > DUR - 1.2: img = Image.blend(img, Image.new('RGB', (W, H), BG), ease((v - (DUR - 1.2)) / 1.2))
    return img

if len(sys.argv) > 3:
    os.makedirs(OUT, exist_ok=True)
    for x in sys.argv[3:]: frame(float(x)).save(os.path.join(OUT, 'check-%s.png' % x))
    sys.exit(0)

# ---- one file per scene, then concat, then the sound
os.makedirs(OUT, exist_ok=True)
bounds = [0.0] + [sec[0] for sec in SECTIONS] + [END0, DUR]
parts = []
for k in range(len(bounds) - 1):
    a, b = bounds[k], bounds[k + 1]
    n0, n1 = int(round(a * FPS)), int(round(b * FPS))
    name = os.path.join(OUT, 'scene%02d.mp4' % k); tmp = os.path.join(OUT, 'scene%02d.part.mp4' % k)
    if os.path.exists(name) and os.environ.get('KEEP_SCENES'): parts.append(name); continue
    p = subprocess.Popen(['ffmpeg', '-v', 'error', '-y', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '%dx%d' % (W, H), '-r', str(FPS),
                          '-i', '-', '-c:v', 'libx264', '-crf', '16', '-preset', 'medium', '-tune', 'animation', '-pix_fmt', 'yuv420p',
                          '-r', str(FPS), tmp], stdin=subprocess.PIPE)
    for n in range(n0, n1): p.stdin.write(frame(n / FPS).tobytes())
    p.stdin.close()
    if p.wait() != 0: sys.exit('ffmpeg failed on scene %d' % k)
    os.replace(tmp, name); parts.append(name); print('scene', k, a, b, flush=True)
with open(os.path.join(OUT, 'list.txt'), 'w') as f:
    for p in parts: f.write("file '%s'\n" % os.path.abspath(p))

w = wave.open(os.path.join(SRC, 'film.wav')); sr = w.getframerate()
a = np.frombuffer(w.readframes(w.getnframes()), np.int16).reshape(-1, 2).astype(np.float64) / 32768
# the last bars, continued past the film's end: 4 bars at 90 BPM, the period found in the sound itself
x = a[len(a) - sr * 9: len(a) - sr * 1, 0]
P = min(range(int(sr * 10.55), int(sr * 10.75), 2), key=lambda L: float(((x - a[len(a) - sr * 9 - L: len(a) - sr * 1 - L, 0]) ** 2).mean()))
need = int(DUR * sr) - int(TW * sr) - len(a) + sr
xf = int(0.03 * sr); r = np.linspace(0, 1, xf)[:, None]
ext = a[len(a) - P - xf: len(a) - P - xf + need].copy()       # ext[0] lands where a's last xf samples begin
ext[:xf] = ext[:xf] * r + a[len(a) - xf:] * (1 - r)
a = np.concatenate([np.zeros((int(TW * sr), 2)), a[:-xf], ext])[:int(DUR * sr)]
t = np.arange(len(a)) / sr
a *= np.clip((DUR - 0.4 - t) / 4.0, 0, 1)[:, None] ** 1.5
a *= 0.89 / np.abs(a).max()
wo = wave.open(os.path.join(OUT, 'sound.wav'), 'wb'); wo.setnchannels(2); wo.setsampwidth(2); wo.setframerate(sr)
wo.writeframes((a * 32767).astype('<i2').tobytes()); wo.close()
print('period', P, P / sr)
final = os.path.join(OUT, 'sloop3-preview.mp4'); tmp = os.path.join(OUT, 'sloop3-preview.part.mp4')
r = subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'concat', '-safe', '0', '-i', os.path.join(OUT, 'list.txt'),
                    '-i', os.path.join(OUT, 'sound.wav'), '-map', '0:v', '-map', '1:a', '-c:v', 'copy',
                    '-af', 'volume=4dB,alimiter=limit=0.89:attack=4:release=60:level=disabled', '-c:a', 'aac', '-b:a', '256k', '-ar', '48000',
                    '-shortest', '-movflags', '+faststart', tmp])
if r.returncode: sys.exit('ffmpeg failed on the final file')
os.replace(tmp, final); print('done', final)
