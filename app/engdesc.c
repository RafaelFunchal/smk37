/* SPDX-License-Identifier: GPL-3.0-only
 * The engines as the player sees them (engdesc.h), written by tools/mkengdesc.py from SLOOP 2.8.5's facts
 * (docs/sloop/facts.json): its labels and choices. */
#include "engdesc.h"

static const char *const L0[5] = { "SAW", "SQR", "TRI", "SIN", "PWM" };
static const char *const L1[46] = { "ACOUSTIC", "DEEP", "TIGHT", "BRIGHT", "DUST", "808", "909", "606", "80S", "VINTAGE", "TRAP", "DRILL", "BOOMBAP", "LO-FI", "PHONK", "HOUSE", "D.HOUSE", "TECHNO", "MINIMAL", "ELECTRO", "DISCO", "GARAGE", "JUNGLE", "DUBSTEP", "DEMBOW", "AMAPIANO", "AFRO", "LATIN", "TRIBAL", "SYNTHWV", "CHIP", "ARCADE", "GLITCH", "INDUSTR", "HYPER", "AMBIENT", "JAZZ", "USR1", "USR2", "USR3", "USR4", "USR3+4", "SYN1", "SYN2", "SYN3", "SYN4" };
static const char *const L2[8] = { "1", "2", "3", "4", "5", "6", "7", "8" };
static const char *const L3[15] = { ".5", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "14", "16" };
static const char *const L4[5] = { "4BIT", "4B/2", "8BIT", "1BIT", "STEP" };
static const char *const L5[5] = { "PLS", "TRI", "SAW", "NOIS", "WRAM" };
static const char *const L6[4] = { "OFF", "OCT", "MAJ", "MIN" };
static const char *const L7[8] = { "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP" };
static const char *const L8[9] = { "-", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP" };
static const char *const L9[2] = { "MIX", "RING" };
static const char *const L10[16] = { "FLUTE", "MELLO", "HOLLW", "SMOOT", "3BAR", "BLUES", "GOSPL", "ROCK", "TOPS", "CLARI", "REED", "STRNG", "CHAPL", "BRITE", "BASS", "FULL" };
static const char *const L11[7] = { "OFF", "2ND", "3RD", "2SOFT", "3SOFT", "2SLOW", "3SLOW" };
static const char *const L12[3] = { "OFF", "SLOW", "FAST" };
static const char *const L13[4] = { "ANLG", "DUST", "LFSR", "META" };
static const char *const L14[16] = { "SAW3", "PLS3", "PPT", "SST", "TRI3", "S&T", "P&S", "P+N", "NOIS", "SYNC", "SYNCP", "SYNC3", "RING", "RING3", "R+S", "R+SP" };
static const char *const L15[4] = { "LP", "BP", "HP", "NOT" };
static const char *const L16[4] = { "MODAL", "STRNG", "MEMB", "SYMP" };
static const char *const L17[33] = { "PAT", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32" };
static const char *const L18[35] = { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16", "B17", "B18", "B19", "B20", "B21", "B22", "B23", "B24", "B25", "B26", "B27" };
static const char *const L19[12] = { "PIANO", "BASS", "VIBES", "HORNS", "STRGS", "FLUTE", "SCRCH", "PERC", "USR1", "USR2", "USR3", "USR4" };
static const char *const L20[2] = { "OFF", "ON" };
static const char *const L21[5] = { "BREAK", "USR1", "USR2", "USR3", "USR4" };
static const char *const L22[5] = { "4", "8", "16", "32", "AUTO" };
static const char *const L23[3] = { "ONE", "GATE", "LOOP" };
#if BOARD_SMK37
static const char *const LP0[4] = { "CONCERT", "STUDIO", "VINTAGE", "JAZZ" };
static const char *const LP1[4] = { "SOFT", "NORMAL", "HARD", "FIXED" };
#endif
const edesc gb_edesc[ENG_N] = {
    [ENG_VA] = { "ANALOG", { "OSC", "FLT" }, { { "WAVE", PF_LIST, 5, L0 }, { "DTN", PF_CENT, 0, 0 }, { "MIX", PF_PCT, 0, 0 }, { "NOIS", PF_PCT, 0, 0 }, { "CUT", PF_CUT, 0, 0 }, { "RES", PF_PCT, 0, 0 }, { "DRV", PF_PCT, 0, 0 }, { "KTR", PF_PCT, 0, 0 } }, 4, 5 },
    [ENG_DRUM] = { "DRUMS", { "KIT", "SOUNDS" }, { { "KIT", PF_LIST, 46, L1 }, { "TUNE", PF_SEMI, 0, 0 }, { "DEC", PF_NUM, 0, 0 }, { "DRIVE", PF_PCT, 0, 0 }, { "", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 } }, -1, -1 },
    [ENG_FM] = { "DIGITAL", { "OPS", "MOD" }, { { "ALG", PF_LIST, 8, L2 }, { "R2", PF_LIST, 15, L3 }, { "R3", PF_LIST, 15, L3 }, { "R4", PF_LIST, 15, L3 }, { "IDX", PF_PCT, 0, 0 }, { "MDEC", PF_TIME, 0, 0 }, { "FB", PF_PCT, 0, 0 }, { "", PF_NUM, 0, 0 } }, -1, -1 },
    [ENG_LOFI] = { "LOFI", { "CHIP", "MOTN" }, { { "CHIP", PF_LIST, 5, L4 }, { "WAVE", PF_LIST, 5, L5 }, { "DUTY", PF_NUM, 0, 0 }, { "CRSH", PF_PCT, 0, 0 }, { "SWP", PF_PCT, 0, 0 }, { "VIB", PF_PCT, 0, 0 }, { "ARP", PF_LIST, 4, L6 }, { "TONE", PF_PCT, 0, 0 } }, 7, -1 },
    [ENG_PD] = { "PHASE", { "PHS", "LINE" }, { { "WAVE", PF_LIST, 8, L7 }, { "WAVE2", PF_LIST, 9, L8 }, { "DCW", PF_PCT, 0, 0 }, { "ENV", PF_PCT, 0, 0 }, { "DTN", PF_CENT, 0, 0 }, { "LINE", PF_LIST, 2, L9 }, { "SUB", PF_PCT, 0, 0 }, { "", PF_NUM, 0, 0 } }, -1, -1 },
    [ENG_WHEEL] = { "WHEEL", { "BARS", "TONE" }, { { "REG", PF_LIST, 16, L10 }, { "SUB", PF_NUM, 0, 0 }, { "BODY", PF_NUM, 0, 0 }, { "TOP", PF_NUM, 0, 0 }, { "PERC", PF_LIST, 7, L11 }, { "CLICK", PF_PCT, 0, 0 }, { "DRV", PF_PCT, 0, 0 }, { "ROTR", PF_LIST, 3, L12 } }, -1, -1 },
    [ENG_NOISE] = { "NOISE", { "SRC", "MOVE" }, { { "MODE", PF_LIST, 4, L13 }, { "COLR", PF_PCT, 0, 0 }, { "FREQ", PF_CUT, 0, 0 }, { "RES", PF_PCT, 0, 0 }, { "TRK", PF_PCT, 0, 0 }, { "DENS", PF_PCT, 0, 0 }, { "DRFT", PF_PCT, 0, 0 }, { "CRSH", PF_PCT, 0, 0 } }, 2, 3 },
    [ENG_VOICE] = { "VOICE", { "VOWL", "TONE" }, { { "VOWL", PF_NUM, 0, 0 }, { "VOWL2", PF_NUM, 0, 0 }, { "TALK", PF_TIME, 0, 0 }, { "SHIFT", PF_SEMI, 0, 0 }, { "BUZZ", PF_PCT, 0, 0 }, { "BRTH", PF_PCT, 0, 0 }, { "Q", PF_PCT, 0, 0 }, { "RAND", PF_PCT, 0, 0 } }, -1, 6 },
    [ENG_TRIO] = { "TRIO", { "OSC", "TONE" }, { { "WAVE", PF_LIST, 16, L14 }, { "INT2", PF_SEMI, 0, 0 }, { "INT3", PF_SEMI, 0, 0 }, { "DTN", PF_CENT, 0, 0 }, { "MODE", PF_LIST, 4, L15 }, { "CUT", PF_CUT, 0, 0 }, { "RES", PF_PCT, 0, 0 }, { "PW", PF_PCT, 0, 0 } }, 5, 6 },
    [ENG_PHYS] = { "PHYS", { "BODY", "EXCT" }, { { "MODEL", PF_LIST, 4, L16 }, { "STRC", PF_PCT, 0, 0 }, { "BRIT", PF_PCT, 0, 0 }, { "DAMP", PF_PCT, 0, 0 }, { "POS", PF_PCT, 0, 0 }, { "ACC", PF_PCT, 0, 0 }, { "BOW", PF_PCT, 0, 0 }, { "EXC", PF_PCT, 0, 0 } }, 2, -1 },
    [ENG_FM6] = { "FM6", { "OPS", "PATCH" }, { { "ALG", PF_LIST, 33, L17 }, { "FB", PF_NUM, 0, 0 }, { "MLVL", PF_BI, 0, 0 }, { "MRAT", PF_NUM, 0, 0 }, { "MEG", PF_BI, 0, 0 }, { "VMOD", PF_NUM, 0, 0 }, { "DTUN", PF_PCT, 0, 0 }, { "PTCH", PF_LIST, 35, L18 } }, -1, -1 },
    [ENG_SAMPLE] = { "SAMPLE", { "SET", "TONE" }, { { "SET", PF_LIST, 12, L19 }, { "TUNE", PF_SEMI, 0, 0 }, { "BITS", PF_NUM, 0, 0 }, { "LOOP", PF_LIST, 2, L20 }, { "CUT", PF_NUM, 0, 0 }, { "", PF_NUM, 0, 0 }, { "DRV", PF_PCT, 0, 0 }, { "", PF_NUM, 0, 0 } }, 4, -1 },
    [ENG_GRAIN] = { "GRAIN", { "GRAN", "SPRY" }, { { "SRC", PF_LIST, 12, L19 }, { "POS", PF_PCT, 0, 0 }, { "SIZE", PF_PCT, 0, 0 }, { "DENS", PF_PCT, 0, 0 }, { "PTCH", PF_SEMI, 0, 0 }, { "SPRD", PF_PCT, 0, 0 }, { "RAND", PF_PCT, 0, 0 }, { "TONE", PF_PCT, 0, 0 } }, 7, -1 },
    [ENG_SLICE] = { "SLICE", { "CUT", "PLAY" }, { { "SRC", PF_LIST, 5, L21 }, { "DIV", PF_LIST, 5, L22 }, { "STRT", PF_NUM, 0, 0 }, { "PTCH", PF_SEMI, 0, 0 }, { "MODE", PF_LIST, 3, L23 }, { "REV", PF_LIST, 2, L20 }, { "DCAY", PF_NUM, 0, 0 }, { "TONE", PF_NUM, 0, 0 } }, 7, -1 },
#if BOARD_SMK37                     /* (by hand, not tools/mkengdesc.py: the SMK-37's piano, engine/piano.c) */
    [ENG_PIANO] = { "PIANO", { "TONE", "DAMP" }, { { "TONE", PF_LIST, 4, LP0 }, { "BRILL", PF_PCT, 0, 0 }, { "TOUCH", PF_LIST, 4, LP1 }, { "DECAY", PF_NUM, 0, 0 }, { "DAMPR", PF_NUM, 0, 0 }, { "NOISE", PF_PCT, 0, 0 }, { "WIDTH", PF_PCT, 0, 0 }, { "UNA C", PF_PCT, 0, 0 } }, 1, -1 },
#endif
};
