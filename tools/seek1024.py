#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The sample bank's seek index, every 1024 samples instead of 512 (format 4, 3.1.6: ~2.8 KB of flash for the
# visualiser): every other seek point kept, each zone's first one renumbered; the ADPCM untouched.
#   tools/seek1024.py IN.bin OUT.bin
import struct, sys, zlib
img = open(sys.argv[1], 'rb').read()
magic, ver, nsets, nzones, r0, data_off, data_len, crc, nseek, r1 = struct.unpack_from('<IHHHHIIIII', img, 0)
assert magic == 0x42533353 and ver == 3, "not a format-3 bank"
zat = 32 + 16 * nsets
first = list(struct.unpack_from('<%dI' % nzones, img, zat + 28 * nzones))
seek = list(struct.unpack_from('<%dI' % nseek, img, zat + 32 * nzones))
nf, ns = [], []
for z in range(nzones):
    a = first[z]; b = first[z + 1] if z + 1 < nzones else nseek
    nf.append(len(ns)); ns += seek[a:b:2]
head = 32 + 16 * nsets + 28 * nzones + 4 * nzones + 4 * len(ns)
head = (head + 255) & ~255
body = img[32:zat + 28 * nzones] + b''.join(struct.pack('<I', x) for x in nf) + b''.join(struct.pack('<I', x) for x in ns)
hdr = struct.pack('<IHHHHIIIII', magic, 4, nsets, nzones, r0, head, data_len, crc, len(ns), r1)
out = hdr + body
out += b'\xFF' * (head - len(out)) + img[data_off:data_off + data_len]
open(sys.argv[2], 'wb').write(out)
print('seek points %d -> %d; the bank %d -> %d bytes' % (nseek, len(ns), len(img), len(out)), file=sys.stderr)
