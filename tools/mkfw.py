#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The firmware's application area, plain (the installer page encrypts it with the unit's chip key):
#   +0x000  app_area_head  attribute 0x83, address = the entry (0x02000120), length = the area
#   +0x020  app.bin        attribute 0x82, address 0x120 (relative), its length and CRC16 (over the plain bytes)
#   +0x040  0xFF up to 0x120: no more records (an 0xFF record fails its CRC, which ends the table)
#   +0x120  app.bin: fw/xip.S then the firmware's RAM image; with a bank, padded to BANK_AT and the sample bank after
#           (read in place through the flash window, flash 0x48000: engine/sample.h)
# Format facts: m-bootloader docs/SPEC.md 5.2 and 5.4 (records: CRC16/XMODEM of bytes 2..31 at 0).
#   mkfw.py app.bin out.area [samples.bin]
import struct, sys

def crc16(data, crc=0):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def record(data_crc, address, length, attr, name, index=0):
    body = struct.pack("<HIIBBH", data_crc, address, length, attr, 0, index) + name.encode().ljust(16, b"\0")
    return struct.pack("<H", crc16(body)) + body

import os
BANK_AT = int(os.environ.get("MKFW_BANK_AT", "0x44000"), 0)   # the bank's place in the area (flash 0x48000; 3.3.1: the bank made right is larger; 0x47778 in 3.1.6..3.3;
                                                               # the SMK-37: 0x48000, flash 0x4C000)
AREA_END = int(os.environ.get("MKFW_AREA_END", "0x74000"), 0)  # (flash 0x78000: SLOOP 3's FM6 bank and SYN kits, never reached; the SMK-37: 0x78000, its piano)
def main():
    app = open(sys.argv[1], "rb").read()
    entry, rel = 0x02000120, 0x120
    if len(sys.argv) > 3:
        bank = open(sys.argv[3], "rb").read()
        assert rel + len(app) <= BANK_AT, "the firmware runs into the sample bank"
        assert BANK_AT + len(bank) <= AREA_END, "the sample bank runs into the FM6 bank (flash 0x78000)"
        app = app + b"\xff" * (BANK_AT - rel - len(app)) + bank
    area_len = rel + len(app)
    head = record(crc16(app), entry, area_len, 0x83, "app_area_head")
    appr = record(crc16(app), rel, len(app), 0x82, "app.bin", 1)
    area = head + appr + b"\xff" * (rel - 64) + app
    assert len(area) == area_len and crc16(b"\xff" * 30) != 0xFFFF
    open(sys.argv[2], "wb").write(area)
    print(f"{sys.argv[2]}: app area {area_len} bytes, app.bin {len(app)} bytes, CRC {crc16(app):04X}")

if __name__ == "__main__":
    main()
