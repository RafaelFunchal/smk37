/* SPDX-License-Identifier: GPL-3.0-only
 * The power-on logo (app/vis.c, vis_splash_draw) drawn large, for the videos: the same shapes and timing at K times
 * the FM-1's scale, on the video's background. Writes N frames of S x S RGB565 (30 a second) to stdout.
 *   logo_film K FRAMES BG_R BG_G BG_B C1 .. C5 (the tracks' colours, rrggbb) */
#include "../app/gfx.h"
#undef GFX_W
#undef GFX_H
#define GFX_W 4000                                      /* (the shapes clipped to the large picture, not the FM-1 screen) */
#define GFX_H 4000
#include "../app/vis.c"
#include <stdio.h>
#include <stdlib.h>
ui_palette ui_pal;                                       /* (unused by the logo; vis.c names it) */
static uint16_t hex565(const char *h) { unsigned v = (unsigned)strtoul(h, 0, 16); return RGB((v >> 16) & 255, (v >> 8) & 255, v & 255); }
int main(int argc, char **argv)
{
    if (argc < 11) return 2;
    float K = (float)atof(argv[1]); int N = atoi(argv[2]), S = (int)(240.0f * K);
    uint16_t bg = RGB(atoi(argv[3]), atoi(argv[4]), atoi(argv[5])), col[5];
    for (int k = 0; k < 5; k++) col[k] = hex565(argv[6 + k]);
    gb_tables_init();
    uint16_t *px = malloc((size_t)S * S * 2);
    for (int f = 0; f < N; f++) {
        float t = (float)f / 30.0f, reach[5]; uint16_t bc[5];
        gfx_target(px, 0, 0, S, S); gfx_fill(0, 0, S, S, bg);
        for (int k = 0; k < 5; k++) { reach[k] = back((t - 0.55f - 0.1f * (float)k) / 0.35f); bc[k] = col[k];
            float g = t - 1.6f - 0.07f * (float)k; if (g > 0.0f && g < 0.25f) bc[k] = gfx_mix(col[k], LW, (unsigned)(20.0f * (1.0f - g * 4.0f))); }
        xf T = { K * (120.0f - 120.0f * 0.62f), K * 12.0f, K * 0.62f };
        logo(&T, ease(t / 0.7f), 1.0f, gfx_mix(bg, LW, 6), ease((t - 0.25f) / 0.4f), reach, bc, ease((t - 1.05f) / 0.4f), ease((t - 0.15f) / 0.35f), 0.0f, 0.0f);
        word(K * (120.0f - 443.0f * 0.36f), K * 132.0f, K * 0.36f, (t - 1.0f) / 0.7f, LW);
        fwrite(px, 2, (size_t)S * S, stdout);
    }
    return 0;
}
