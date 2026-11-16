/* SPDX-License-Identifier: GPL-3.0-only
 * The text console, drawn a cell at a time (console.h). */
#include "board.h"
#include "console.h"

static char want[15][20], shown[15][20];
static uint16_t want_c[15];
static int cur;
/* the big lines: 3 x 13 cells of 18 x 24 at the top (y 0, 24, 48), over small rows 0..4 */
static char bwant[3][13], bshown[3][13];
static uint16_t bwant_c[3];
static int bcur;
void con_big(int line, const char *s, uint16_t col)
{
    if (line < 0 || line >= 3) return;
    int i = 0;
    for (; s[i] && i < 13; i++) bwant[line][i] = s[i];
    for (; i < 13; i++) bwant[line][i] = ' ';
    if (bwant_c[line] != col) { bwant_c[line] = col; for (int k = 0; k < 13; k++) bshown[line][k] = 0; }
}

void con_line(int row, const char *s, uint16_t col)
{
    if (row < 0 || row >= 15) return;
    int i = 0;
    for (; s[i] && i < 20; i++) want[row][i] = s[i];
    for (; i < 20; i++) want[row][i] = ' ';
    if (want_c[row] != col) { want_c[row] = col; for (int k = 0; k < 20; k++) shown[row][k] = 0; }
}
int con_step(void)
{
    for (int n = 0; n < 39; n++) {
        int r = bcur / 13, c = bcur % 13;
        bcur = (bcur + 1) % 39;
        if (bwant[r][c] != bshown[r][c]) {
            lcd_char(c * 18, r * 24, bwant[r][c], bwant_c[r], 0, 3);
            bshown[r][c] = bwant[r][c];
            return 1;
        }
    }
    for (int n = 0; n < 300; n++) {
        int r = cur / 20, c = cur % 20;
        cur = (cur + 1) % 300;
        if (want[r][c] != shown[r][c]) {
            char t[2] = { want[r][c], 0 };
            lcd_text(c, r, t, want_c[r], 0);
            shown[r][c] = want[r][c];
            return 1;
        }
    }
    return 0;
}
void con_flush(void) { while (con_step()) ; }
char *con_put_u(char *p, uint32_t v) { char t[12]; int n = 0; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n) *p++ = t[--n]; return p; }
char *con_put_i(char *p, int32_t v) { if (v < 0) { *p++ = '-'; return con_put_u(p, (uint32_t)-v); } return con_put_u(p, (uint32_t)v); }
char *con_put_s(char *p, const char *s) { while (*s) *p++ = *s++; return p; }
void con_num(int row, const char *label, int32_t v, const char *unit, uint16_t col)
{
    char b[32], *p = con_put_s(b, label); p = con_put_i(p, v); p = con_put_s(p, unit); *p = 0; con_line(row, b, col);
}
