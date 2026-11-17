#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The installer page: web/installer.tmpl.html with web/fwpkg.js, the sailboat mark (web/sloop-mark.png), the
# pictures (web/inst-hero.webp, web/inst-play.webp: the FM-1 in 3D) and the application
# area inlined (one file, opens from the disk too). It installs through m-bootloader's MIDI MODE only.
#   mkpage.py AREA VERSION OUT.html [BOARD [PIANO]]
# BOARD: fm1 (the default) or smk37; the SMK-37's page also carries the piano bank (tools/mkpiano.py), written in
# clear at flash 0x78000, and speaks of the SMK-37's keys (m-bootloader's menu: < > SEQ PLAY SEQ REC).
import base64, re, sys
area, version, out = sys.argv[1], sys.argv[2], sys.argv[3]
board = sys.argv[4] if len(sys.argv) > 4 else "fm1"
assert board in ("fm1", "smk37"), "BOARD: fm1 or smk37"
t = open("web/installer.tmpl.html").read()
js = open("web/fwpkg.js").read()

def smk_words(t):
    """the page's words for the SMK-37: its name, its keys, its files; each rule must match (a template that
    changed is caught here, not shipped with the FM-1's words)"""
    def sub(old, new, n=None):
        nonlocal t
        c = t.count(old)
        assert c and (n is None or c == n), ("mkpage smk37: not found", old, c)
        t = t.replace(old, new)
    # no pictures of an FM-1 on the SMK-37's page
    t = re.sub(r'<figure class="shot"[^\n]*?</figure>', '', t)
    t = re.sub(r'<img src="data:image/webp;base64,__PLAY__"[^>]*>', '', t)
    # the FM-1's newer flash chip: not a matter on the SMK-37
    t = re.sub(r"<li>FM-1s sold since fall 2026.*?</li>", "", t)
    t = re.sub(r"<li>Les FM-1 vendus depuis l\\'automne 2026.*?</li>", "", t)
    # m-bootloader's menu keys on the SMK-37 (docs/SPEC.md 11.6: <, >, SEQ PLAY, SEQ REC)
    sub("<kbd>OCT-</kbd> / <kbd>OCT+</kbd>", "<kbd>&lt;</kbd> / <kbd>&gt;</kbd>")
    sub("<kbd>PLAY</kbd>", "<kbd>SEQ PLAY</kbd>")
    sub("<kbd>REC</kbd>", "<kbd>SEQ REC</kbd>")
    # the firmware's keys (SLOOP 3 on the SMK-37: the printed names)
    sub("<kbd>OCT-</kbd>", "<kbd>OCTAVE-</kbd>")
    sub("<kbd>OCT+</kbd>", "<kbd>OCTAVE+</kbd>")
    sub("<kbd>HOME</kbd>", "<kbd>PATCH</kbd>")
    sub("<kbd>EDIT</kbd>", "<kbd>PARA</kbd>")
    t = re.sub(r"(?<![A-Z ])PLAY(?=[,.;: ])", "SEQ PLAY", t)       # (in the scripts' strings: "hold PLAY, choose ...")
    # its files
    sub("fm1-uboot", "smk37-uboot")
    # its name, in the words (not in the code: the FM-1's page's ports and names stay in its own branch)
    code = t[t.index("<script>"):]
    head, code = t[:t.index("<script>")], code
    head = head.replace("FM-1", "SMK-37")
    code = re.sub(r"'([^'\\]|\\.)*'", lambda m: m.group(0).replace("FM-1", "SMK-37"), code)
    return head + code

if board == "smk37":
    t = smk_words(t)
t = t.replace("__FWPKG__", js).replace("__VERSION__", version).replace("__BOARD__", board)
t = t.replace("__PIANO__", base64.b64encode(open(sys.argv[5], "rb").read()).decode() if board == "smk37" else "")
t = t.replace("__MARK__", base64.b64encode(open("web/sloop-mark.png", "rb").read()).decode())   # (the sailboat in its dial, square)
t = t.replace("__HERO__", base64.b64encode(open("web/inst-hero.webp", "rb").read()).decode())
t = t.replace("__PLAY__", base64.b64encode(open("web/inst-play.webp", "rb").read()).decode())
t = t.replace("__AREA__", base64.b64encode(open(area, "rb").read()).decode())
open(out, "w").write(t)
print(out, len(t), "bytes")
