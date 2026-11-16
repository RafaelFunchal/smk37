#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make a RAM app file for m-bootloader (its SPEC 11.8): the image linked at 0x01C30000, then an 8-byte trailer:
'APP1', CRC16/XMODEM of everything before it (little endian), the board (1 = FM-1, 2 = SMK-37), the format (1).
    tools/ramapp.py IMAGE.raw OUT.bin [--board fm1|smk37|any]"""
import argparse, struct

def crc16(data, crc=0):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

ap = argparse.ArgumentParser()
ap.add_argument('image'); ap.add_argument('out'); ap.add_argument('--board', default='fm1', choices=['any', 'fm1', 'smk37'])
a = ap.parse_args()
img = open(a.image, 'rb').read()
img += b'\0' * (-len(img) % 4)
out = img + b'APP1' + struct.pack('<HBB', crc16(img), {'any': 0, 'fm1': 1, 'smk37': 2}[a.board], 1)
assert 16 <= len(out) <= 0x4FD80, 'the app does not fit the RAM slot'
open(a.out, 'wb').write(out)
print(f'{a.out}: {len(out)} bytes, CRC {crc16(img):04X}')
