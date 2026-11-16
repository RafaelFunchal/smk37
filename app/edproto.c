/* SPDX-License-Identifier: GPL-3.0-only
 * The editor's frames (edproto.h). Written for this project. */
#include "edproto.h"

int ed_parse(const uint8_t *sx, uint32_t n, uint8_t *out, uint32_t cap, uint32_t *len)
{
    if (n < 6 || sx[0] != 0xF0 || sx[1] != ED_H0 || sx[2] != ED_H1 || sx[3] != ED_H2 || sx[n - 1] != 0xF7) return -1;
    uint32_t acc = 0, bits = 0, o = 0;
    for (uint32_t i = 5; i + 1 < n; i++) {
        if (sx[i] & 0x80u) return -1;
        acc |= (uint32_t)sx[i] << bits; bits += 7;
        if (bits >= 8) { if (o < cap) out[o] = (uint8_t)acc; o++; acc >>= 8; bits -= 8; }
    }
    *len = o < cap ? o : cap;
    return sx[4] & 0x7F;
}
uint32_t ed_frame(int cmd, const uint8_t *raw, uint32_t n, uint8_t *out, uint32_t cap)
{
    uint32_t need = 6 + (n * 8 + 6) / 7, o = 0, acc = 0, bits = 0;
    if (need > cap) return 0;
    out[o++] = 0xF0; out[o++] = ED_H0; out[o++] = ED_H1; out[o++] = ED_H2; out[o++] = (uint8_t)(cmd & 0x7F);
    for (uint32_t i = 0; i < n; i++) {
        acc |= (uint32_t)raw[i] << bits; bits += 8;
        while (bits >= 7) { out[o++] = (uint8_t)(acc & 0x7Fu); acc >>= 7; bits -= 7; }
    }
    if (bits) out[o++] = (uint8_t)(acc & 0x7Fu);
    out[o++] = 0xF7;
    return o;
}
int ed_usb_packet(const uint8_t *sx, uint32_t n, uint32_t i, uint8_t pk[4])
{
    uint32_t k = n - i < 3 ? n - i : 3, end = 0;
    for (uint32_t j = 0; j < k; j++) if (sx[i + j] == 0xF7) { k = j + 1; end = 1; break; }
    pk[0] = (uint8_t)(end ? 4 + k : 4);                 /* (CIN 4: a SysEx goes on; 5, 6, 7: it ends with 1, 2, 3 bytes) */
    for (uint32_t j = 0; j < 3; j++) pk[1 + j] = j < k ? sx[i + j] : 0;
    return (int)k;
}
