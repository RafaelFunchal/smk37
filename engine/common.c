/* SPDX-License-Identifier: GPL-3.0-only
 * What the engines share: the sine table, the state-variable filter's coefficients. Written for this project. */
#include "engine.h"

int16_t gb_sine[1025] GB_LOWRAM;                       /* (filled by gb_tables_init, at snd_init) */
GB_SMALL void gb_tables_init(void)
{
    for (int i = 0; i <= 1024; i++) gb_sine[i] = (int16_t)(gb_sin_turn((float)(i & 1023) * (1.0f / 1024.0f)) * 32767.0f + (i < 512 ? 0.5f : -0.5f));
}
/* Simper's trapezoidal SVF: g = tan(pi fc / fs), damping k (2: flat, small: resonant) */
__attribute__((noinline)) float gb_flush(float x) { return (x < GB_TINY && x > -GB_TINY) ? 0.0f : x; }
void gb_svf_set(gb_svf *f, float hz, float k)
{
    float g = gb_tan_pi(gb_clampf(hz * GB_ISR, 0.0001f, 0.47f));
    f->k = k;
    f->a1 = 1.0f / (1.0f + g * (g + k)); f->a2 = g * f->a1; f->a3 = g * f->a2;
}
