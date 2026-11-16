/* SPDX-License-Identifier: GPL-3.0-only
 * A text console on the panel (console.c), 20 x 15 cells, drawn one changed cell at a time: a whole line takes
 * ~5 ms on the SPI panel, longer than the audio can wait; a cell ~0.3 ms, between two audio halves. */
#pragma once
#include <stdint.h>
void con_line(int row, const char *s, uint16_t col);          /* what a row should show (padded to 20) */
void con_big(int line, const char *s, uint16_t col);          /* big lines 0..2 (13 cells of 18 x 24) over rows 0..4 */
void con_num(int row, const char *label, int32_t v, const char *unit, uint16_t col);
int con_step(void);                                           /* draws one changed cell; 0 when all are drawn */
void con_flush(void);
char *con_put_u(char *p, uint32_t v);
char *con_put_i(char *p, int32_t v);
char *con_put_s(char *p, const char *s);
#define C_WHITE RGB(255, 255, 255)
#define C_GREEN RGB(80, 255, 120)
#define C_RED RGB(255, 80, 80)
#define C_YEL RGB(255, 210, 60)
#define C_CYAN RGB(80, 220, 255)
#define C_GREY RGB(140, 140, 150)
