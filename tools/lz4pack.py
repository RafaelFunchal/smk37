#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# The firmware's RAM image packed for the flash (3.2): one LZ4 block (the LZ4 block format: token, literals, a 16-bit
# offset, the match's length), found with hash chains and a lazy step; fw/unpack.c unpacks it into RAM at the start.
# The file: [the unpacked length, u32][two sums of its words, u32 each: fw/unpack.c][the block].
# Written for this project.   lz4pack.py IN OUT   (prints the sizes; unpacks what it made and compares)
import sys

MINM, WINDOW, CHAIN = 4, 65535, 96

def pack(src):
    n = len(src); out = bytearray(); head = {}; prev = [0] * n
    def find(i):
        best_len, best_off = 0, 0
        if i + MINM > n - 5: return 0, 0
        key = src[i:i + 4]; j = head.get(key, -1); tries = CHAIN
        limit = n - 5                                   # (the last 5 bytes are literals: the format's rule)
        while j >= 0 and i - j <= WINDOW and tries:
            if src[j + best_len:j + best_len + 1] == src[i + best_len:i + best_len + 1]:
                l = 0
                while i + l < limit and src[j + l] == src[i + l]: l += 1
                if l > best_len: best_len, best_off = l, i - j
            j = prev[j] - 1; tries -= 1
        return (best_len, best_off) if best_len >= MINM else (0, 0)
    def insert(i):
        if i + 4 <= n:
            key = src[i:i + 4]; prev[i] = head.get(key, -1) + 1; head[key] = i
    def emit(lit, mlen, off):
        ll = len(lit); ml = mlen - MINM if mlen else 0
        out.append((min(ll, 15) << 4) | (min(ml, 15) if mlen else 0))
        if ll >= 15:
            r = ll - 15
            while r >= 255: out.append(255); r -= 255
            out.append(r)
        out.extend(lit)
        if mlen:
            out.append(off & 255); out.append(off >> 8)
            if ml >= 15:
                r = ml - 15
                while r >= 255: out.append(255); r -= 255
                out.append(r)
    i = 0; anchor = 0
    while i < n:
        l, o = find(i)
        if l:
            insert(i)
            l2, o2 = find(i + 1)                        # lazy: a longer match one byte on wins
            if l2 > l + 1: i += 1; continue
            emit(src[anchor:i], l, o)
            for k in range(i + 1, i + l): insert(k)
            i += l; anchor = i
        else:
            insert(i); i += 1
    emit(src[anchor:], 0, 0)
    return bytes(out)

def unpack(b, size):
    out = bytearray(); s = 0
    while True:
        t = b[s]; s += 1; n = t >> 4
        if n == 15:
            while True:
                x = b[s]; s += 1; n += x
                if x != 255: break
        out += b[s:s + n]; s += n
        if s >= len(b): break
        off = b[s] | b[s + 1] << 8; s += 2
        n = t & 15
        if n == 15:
            while True:
                x = b[s]; s += 1; n += x
                if x != 255: break
        n += 4
        for _ in range(n): out.append(out[-off])
    assert len(out) == size
    return bytes(out)

if __name__ == "__main__":
    src = open(sys.argv[1], "rb").read()
    src += bytes(-len(src) % 4)
    p = pack(src)
    assert unpack(p, len(src)) == src, "the packed image does not unpack to itself"
    import struct
    words = struct.unpack("<%dI" % (len(src) // 4), src)
    a = b = 0
    for w in words: a = (a * 0x01000193 + w) & 0xFFFFFFFF; a ^= a >> 15; b = ((b ^ a) * 0x9E3779B1) & 0xFFFFFFFF
    head = struct.pack("<III", len(src), a, b)      # (fw/unpack.c: the length; its two sums of the words)
    open(sys.argv[2], "wb").write(head + p)
    print(f"{sys.argv[2]}: {len(src)} bytes packed into {len(p)} ({100 * len(p) // len(src)} %)")
