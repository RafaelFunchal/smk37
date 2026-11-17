/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37 board, first part (hal/fm1/board.c's, rewired): a time base (timer 0, free running), the screen (ST7789V
 * on SPI1 group A: PB6 clock, PB7 data, PC8 CS, PB5 D/C; no backlight pin) with a text console, and the pads' lights
 * on the same SPI1 in group B (PC10), sent while the screen's bus is idle. Facts: hal/smk37/smk37.h. */
#include "smk37.h"
#include "../fm1/board.h"
#undef LCD_CS
#undef LCD_DC
#define LCD_CS SMK_LCD_CS
#define LCD_DC SMK_LCD_DC

static uint32_t lsb_mhz = 60;

/* ---------------------------------------------------------------- time */
void time_init(uint32_t lsb)
{
    lsb_mhz = lsb ? lsb : 60;
    T0_CON = 0x4000u; T0_CNT = 0; T0_PRD = 0xFFFFFFFFu; T0_CON = 1;
}
uint32_t time_ticks(void) { return T0_CNT; }
uint32_t time_mhz(void) { return lsb_mhz; }
void time_wait_us(uint32_t us)
{
    while (us > 1000u) { time_wait_us(1000u); us -= 1000u; }      /* steps of 1 ms: no long count */
    uint32_t t0 = T0_CNT, n = us * lsb_mhz;
    while ((uint32_t)(T0_CNT - t0) < n) ;
}

/* ---------------------------------------------------------------- the panel */
static void spi_wait(void)
{
    uint32_t t0 = T0_CNT;
    while (!(SPI1_CON & (1u << 15))) if ((uint32_t)(T0_CNT - t0) > 20000u * lsb_mhz) break;   /* 20 ms at most */
    SPI1_CON |= 1u << 14;
}
static void spi_byte(uint8_t b) { SPI1_BUF = b; spi_wait(); }
static void lcd_cmd(uint8_t c) { pin_set(LCD_DC, 0); pin_set(LCD_CS, 0); spi_byte(c); pin_set(LCD_CS, 1); }
static void lcd_data(const uint8_t *d, int n) { pin_set(LCD_DC, 1); pin_set(LCD_CS, 0); while (n--) spi_byte(*d++); pin_set(LCD_CS, 1); }
__attribute__((section(".xtext.smkboard"))) static void lcd_dma(const void *p, uint32_t n)                   /* n bytes from internal RAM, n > 0 */
{
    pin_set(LCD_DC, 1); pin_set(LCD_CS, 0);
    const uint8_t *b = (const uint8_t *)p;
    while (n) {
        uint32_t k = n > 0x8000u ? 0x8000u : n;
        SPI1_CON &= ~(1u << 12);                                  /* transmit */
        SPI1_ADR = (uint32_t)(uintptr_t)b; SPI1_CNT = k; spi_wait();
        b += k; n -= k;
    }
    pin_set(LCD_CS, 1);
}
static void lcd_window(int x, int y, int w, int h)
{
    uint8_t d[4];
    lcd_cmd(0x2A); d[0] = 0; d[1] = (uint8_t)x; d[2] = 0; d[3] = (uint8_t)(x + w - 1); lcd_data(d, 4);
    lcd_cmd(0x2B); d[0] = 0; d[1] = (uint8_t)y; d[2] = 0; d[3] = (uint8_t)(y + h - 1); lcd_data(d, 4);
    lcd_cmd(0x2C);
}
static uint16_t line[240];
__attribute__((section(".xtext.smkboard"))) void lcd_fill(int x, int y, int w, int h, uint16_t rgb565)
{
    for (int i = 0; i < w; i++) line[i] = (uint16_t)((rgb565 >> 8) | (rgb565 << 8));   /* high byte first */
    lcd_window(x, y, w, h);
    pin_set(LCD_DC, 1);
    for (int r = 0; r < h; r++) lcd_dma(line, (uint32_t)w * 2u);
}
/* a tile sent while the CPU goes on: the window set (a few bytes, polled), then the pixels by DMA */
static int tile_on;
void lcd_tile_start(int x, int y, int w, int h, const void *px)
{
    {   /* the last tile's end; 3.3: never for ever (a tile is 4 ms at most: past 30, the transfer is given up) */
        uint32_t t0 = T0_CNT;
        while (lcd_tile_busy()) if ((uint32_t)(T0_CNT - t0) > 30000u * lsb_mhz) { SPI1_CON |= 1u << 14; pin_set(LCD_CS, 1); tile_on = 0; break; }
    }
    lcd_window(x, y, w, h);
    pin_set(LCD_DC, 1); pin_set(LCD_CS, 0);
    SPI1_CON &= ~(1u << 12);                                      /* transmit */
    SPI1_ADR = (uint32_t)(uintptr_t)px; SPI1_CNT = (uint32_t)(w * h * 2);
    tile_on = 1;
}
int lcd_tile_busy(void)
{
    if (!tile_on) return 0;
    if (!(SPI1_CON & (1u << 15))) return 1;
    SPI1_CON |= 1u << 14;
    pin_set(LCD_CS, 1);
    tile_on = 0;
    return 0;
}
__attribute__((section(".xtext.smkboard"))) static void lcd_cmd_data(uint8_t c, const uint8_t *d, int n) { lcd_cmd(c); if (n) lcd_data(d, n); }
__attribute__((section(".xtext.smkboard"))) void lcd_init(void)
{
    pin_out(LCD_CS, 1); pin_out(LCD_DC, 0); pin_out(SMK_LCD_CLK, 0); pin_out(SMK_LCD_MOSI, 0);
    pin_out(SMK_PADLED_PIN, 0);                                    /* the pads' line idles low (their latch) */
    IOMAP_CON1 &= ~SMK_SPI1_GROUP_B;
    SPI1_CON = 0x4021u; SPI1_BAUD = 4;                             /* 12 MHz at a 60 MHz LSB */
    time_wait_us(100000);
    lcd_cmd(0x11); time_wait_us(120000);                           /* the stock v016 set-up, as the panel was tuned */
    static const uint8_t MAD[] = { 0x00 }, COL[] = { 0x05 }, POR[] = { 0x0C, 0x0C, 0x00, 0x33, 0x33 }, GC[] = { 0x35 },
        VCOM[] = { 0x32 }, VDV[] = { 0x01 }, VRH[] = { 0x15 }, VDS[] = { 0x20 }, FR[] = { 0x0F }, PW[] = { 0xA4, 0xA1 },
        GAM[] = { 0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34 };
    lcd_cmd_data(0x36, MAD, 1); lcd_cmd_data(0x3A, COL, 1); lcd_cmd_data(0xB2, POR, 5); lcd_cmd_data(0xB7, GC, 1);
    lcd_cmd_data(0xBB, VCOM, 1); lcd_cmd_data(0xC2, VDV, 1); lcd_cmd_data(0xC3, VRH, 1); lcd_cmd_data(0xC4, VDS, 1);
    lcd_cmd_data(0xC6, FR, 1); lcd_cmd_data(0xD0, PW, 2); lcd_cmd_data(0xE0, GAM, 14); lcd_cmd_data(0xE1, GAM, 14);
    lcd_cmd(0x21);
    lcd_fill(0, 0, 240, 240, 0x0000);
    lcd_cmd(0x29);
}
/* the pads' frame (panel.c): SPI1 over to PC10 at BAUD 8 for its ~0.5 ms, the screen deselected; 0 sent, -1 a tile
 * still on the bus (the next round sends it) */
int lcd_pads_send(const uint8_t *frame, uint32_t n)
{
    if (lcd_tile_busy()) return -1;
    pin_set(LCD_CS, 1);
    SPI1_BAUD = SMK_PADLED_BAUD;
    IOMAP_CON1 |= SMK_SPI1_GROUP_B;
    SPI1_CON &= ~(1u << 12);
    /* the latch: the frame's first SMK_PADLED_LATCH bytes are zeros; sent 6 more times first, ~300 us low in all (SLOOP
     * Pro: the stock 36 bytes, 43 us, are too short for newer WS2812 parts: the pin switch's glitch then counts as a
     * bit and shifts the chain, a lit pad turning its neighbour green) */
    for (int i = 0; i < 6; i++) { SPI1_CON |= 1u << 14; SPI1_ADR = (uint32_t)(uintptr_t)frame; SPI1_CNT = SMK_PADLED_LATCH; spi_wait(); }
    SPI1_CON |= 1u << 14;
    SPI1_ADR = (uint32_t)(uintptr_t)frame; SPI1_CNT = n;
    spi_wait();
    IOMAP_CON1 &= ~SMK_SPI1_GROUP_B;
    SPI1_BAUD = 4;
    return 0;
}

/* our own 5 x 7 font: seven rows of five bits, the leftmost pixel in bit 4 */
__attribute__((section(".xtext.smkfont"))) static const char FONT_CH[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ .:-+/%=x()<>";
__attribute__((section(".xtext.smkfont"))) static const uint8_t FONT[][7] = {
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},
    {31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,2,12},
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{28,18,17,17,17,18,28},{31,16,16,30,16,16,31},
    {31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,2,18,12},
    {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,17,25,21,19,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},
    {31,1,2,4,8,16,31},{0,0,0,0,0,0,0},{0,0,0,0,0,12,12},{0,12,12,0,12,12,0},{0,0,0,31,0,0,0},
    {0,4,4,31,4,4,0},{1,2,2,4,8,8,16},{24,25,2,4,8,19,3},{0,0,31,0,31,0,0},{0,0,17,10,4,10,17},
    {2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{2,4,8,16,8,4,2},{8,4,2,1,2,4,8},
};
static uint16_t cell[18 * 24];
/* one character at pixel (x, y), the 5 x 7 font scaled by 2 (a 12 x 16 cell) or 3 (18 x 24): one window, one DMA */
__attribute__((section(".xtext.smkboard"))) void lcd_char(int x, int y, char c, uint16_t fg, uint16_t bg, int scale)
{
    uint16_t f = (uint16_t)((fg >> 8) | (fg << 8)), b = (uint16_t)((bg >> 8) | (bg << 8));
    int w = 6 * scale, h = 8 * scale;
    if (c >= 'a' && c <= 'z' && c != 'x') c = (char)(c - 32);
    int g = 36;
    for (int i = 0; FONT_CH[i]; i++) if (FONT_CH[i] == c) { g = i; break; }
    for (int yy = 0; yy < h; yy++) for (int xx = 0; xx < w; xx++) {
        int on = 0, fx = (xx - (scale >> 1)) / scale, fy = (yy - (scale >> 1)) / scale;   /* a margin round the glyph */
        if (xx >= (scale >> 1) && yy >= (scale >> 1) && fx < 5 && fy < 7) on = (FONT[g][fy] >> (4 - fx)) & 1;
        cell[yy * w + xx] = on ? f : b;
    }
    lcd_window(x, y, w, h);
    lcd_dma(cell, (uint32_t)(w * h * 2));
}
__attribute__((section(".xtext.smkboard"))) void lcd_text(int col, int row, const char *s, uint16_t fg, uint16_t bg)
{
    for (; *s && col < 20; s++, col++) lcd_char(col * 12, row * 16, *s, fg, bg, 2);
}

/* ---------------------------------------------------------------- keys */
/* (the FM-1's early PLAY / REC read: on the SMK the panel's scan reads every button, hal/smk37/panel.c) */
void keys_init(void) { }
int key_down(uint32_t col) { (void)col; return 0; }
