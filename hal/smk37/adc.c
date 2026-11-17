/* SPDX-License-Identifier: GPL-3.0-only
 * The SMK-37's stand-ins for the FM-1's ADC (hal/fm1/adc.h): the converter is the panel's (panel.c reads the three
 * muxes a tick at a time), so adc_read gives what it read. MASTER: the FM-1's pot is K1 here, a virtual pot of
 * 0..1023, 24 a detent (from three quarters up, as SLOOP Pro); the battery: mux C 7, through the FM-1's divider. */
#include "smk37.h"
#include "smk_panel.h"
#include "../fm1/adc.h"

int smk_enc_take(int k);
static int32_t master = 768;
void adc_init(void) { }
int32_t adc_read(uint32_t ch)
{
    if (ch == ADC_CH_MASTER) {
        master += smk_enc_take(0) * 24;
        master = master < 0 ? 0 : master > 1023 ? 1023 : master;
        return master;
    }
    if (ch == ADC_CH_BATT) { int b = smk_batt_raw(); return b ? b : -1; }
    return -1;
}
