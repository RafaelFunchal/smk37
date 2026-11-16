/* SPDX-License-Identifier: GPL-3.0-only
 * Your FM6 bank and your SYN kits as bytes (xbank.h). Written for this project. */
#include "xbank.h"
#include "store.h"

void xb_dsnd_pack(const gb_dsnd *s, uint8_t o[22])
{
    o[0] = s->wave; o[1] = s->src; o[2] = (uint8_t)(s->note16 >> 4); o[3] = (uint8_t)(s->note16 & 15u);
    o[4] = s->bend; o[5] = s->bt; o[6] = s->hold; o[7] = s->decay; o[8] = s->tlev; o[9] = s->t2; o[10] = s->t2lev;
    o[11] = s->click; o[12] = s->nlev; o[13] = s->nhold; o[14] = s->ndec; o[15] = s->flt; o[16] = s->fcut;
    o[17] = (uint8_t)s->fenv; o[18] = s->hpf; o[19] = s->chip; o[20] = s->drive; o[21] = (uint8_t)(s->qdb + 128);
}
static uint8_t c127(uint8_t v) { return v > 127 ? 127 : v; }
void xb_dsnd_unpack(const uint8_t d[22], gb_dsnd *o)
{
    o->wave = d[0] <= DW_BELL ? d[0] : DW_SINE;
    o->src = (d[1] & 15u) <= DN_CHIP && !(d[1] & ~0x1Fu) ? d[1] : DN_WHITE;
    o->note16 = (uint16_t)(c127(d[2]) * 16u + (d[3] & 15u));
    o->bend = d[4] > 96 ? 96 : d[4]; o->bt = c127(d[5]); o->hold = d[6]; o->decay = c127(d[7]);
    o->tlev = c127(d[8]); o->t2 = d[9]; o->t2lev = c127(d[10]); o->click = c127(d[11]); o->nlev = c127(d[12]);
    o->nhold = d[13]; o->ndec = c127(d[14]); o->flt = d[15]; o->fcut = c127(d[16]); o->fenv = (int8_t)d[17];
    o->hpf = c127(d[18]); o->chip = c127(d[19]); o->drive = c127(d[20]); o->qdb = (int8_t)(d[21] - 128);
}
void xb_kit_pack(const gb_dsnd s[16], uint8_t crush, uint8_t *o)
{
    o[0] = crush; o[1] = o[2] = o[3] = 0;
    for (int i = 0; i < 16; i++) xb_dsnd_pack(&s[i], o + 4 + 22 * i);
}
void xb_kit_unpack(const uint8_t *d, gb_dsnd s[16], uint8_t *crush)
{
    *crush = d[0];
    for (int i = 0; i < 16; i++) xb_dsnd_unpack(d + 4 + 22 * i, &s[i]);
}
static void w16(uint8_t *b, uint32_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }
static void w32(uint8_t *b, uint32_t v) { w16(b, v); w16(b + 2, v >> 16); }
static uint32_t r32(const uint8_t *b) { return b[0] | b[1] << 8 | b[2] << 16 | (uint32_t)b[3] << 24; }
void xb_head(uint8_t *img, int fm6, uint32_t used, uint32_t len)
{
    img[0] = 'S'; img[1] = '3'; img[2] = fm6 ? 'F' : 'S'; img[3] = fm6 ? '6' : 'Y';
    w16(img + 4, 1); w16(img + 6, fm6 ? XB_NFM6 : 4u); w32(img + 8, fm6 ? used : 0u);
    w32(img + 12, crc32_ieee(0, img + 16, len - 16));
}
int xb_check(const uint8_t *img, uint32_t len, int fm6, uint32_t *used)
{
    if (len != (fm6 ? XB_FM6_LEN : XB_SYN_LEN)) return -1;
    if (img[0] != 'S' || img[1] != '3' || img[2] != (fm6 ? 'F' : 'S') || img[3] != (fm6 ? '6' : 'Y')) return -1;
    if ((img[4] | img[5] << 8) != 1 || (uint32_t)(img[6] | img[7] << 8) != (fm6 ? XB_NFM6 : 4u)) return -1;
    if (crc32_ieee(0, img + 16, len - 16) != r32(img + 12)) return -1;
    if (used) *used = fm6 ? r32(img + 8) & ((1u << XB_NFM6) - 1u) : 0u;
    return 0;
}
