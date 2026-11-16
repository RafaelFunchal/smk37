#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The installer page: web/installer.tmpl.html with web/fwpkg.js, the sailboat mark (web/sloop-mark.png), the
# pictures (web/inst-hero.webp, web/inst-play.webp: the FM-1 in 3D) and the application
# area inlined (one file, opens from the disk too). It installs through m-bootloader's MIDI MODE only.
#   mkpage.py AREA VERSION OUT.html
import base64, sys
area, version, out = sys.argv[1], sys.argv[2], sys.argv[3]
t = open("web/installer.tmpl.html").read()
js = open("web/fwpkg.js").read()
t = t.replace("__FWPKG__", js).replace("__VERSION__", version)
t = t.replace("__MARK__", base64.b64encode(open("web/sloop-mark.png", "rb").read()).decode())   # (the sailboat in its dial, square)
t = t.replace("__HERO__", base64.b64encode(open("web/inst-hero.webp", "rb").read()).decode())
t = t.replace("__PLAY__", base64.b64encode(open("web/inst-play.webp", "rb").read()).decode())
t = t.replace("__AREA__", base64.b64encode(open(area, "rb").read()).decode())
open(out, "w").write(t)
print(out, len(t), "bytes")
