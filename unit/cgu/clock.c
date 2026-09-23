#include <pmb887x.h>

#include "clock.h"

__SRAM void cgu_pll_set(uint32_t ndiv, uint32_t mdiv) {
	uint32_t pll_mask = CGU_OSC_NDIV | CGU_OSC_MDIV | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	uint32_t osc = (CGU_OSC & ~pll_mask) | (ndiv << CGU_OSC_NDIV_SHIFT) | (mdiv << CGU_OSC_MDIV_SHIFT);

	CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
	CGU_OSC = osc;
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP;
	while ((CGU_STAT & CGU_STAT_LOCK) == 0);
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
}

void cgu_phases_enable(uint32_t phases) {
	CGU_OSC |= (phases << 1) | (phases << 9);
}

void cgu_phase_set_divider(uint32_t phase, uint32_t k1, uint32_t k2) {
	uint32_t shift = (phase - 1) * 8;
	uint32_t config = (k1 << 3) | k2;
	CGU_CON0 = (CGU_CON0 & ~(0xFF << shift)) | (config << shift);
}

void cgu_fsys_select(enum cgu_fsys_source source) {
	CGU_CON1 = (CGU_CON1 & ~CGU_CON1_FSYS_CLKSEL) |
		((uint32_t) source << CGU_CON1_FSYS_CLKSEL_SHIFT);
}

void cgu_ahb_select(enum cgu_ahb_source source) {
	CGU_CON1 = (CGU_CON1 & ~CGU_CON1_AHB_CLKSEL) |
		((uint32_t) source << CGU_CON1_AHB_CLKSEL_SHIFT);
}

void cgu_fpi1_select(enum cgu_fpi1_source source, uint32_t divider) {
	CGU_CON1 = (CGU_CON1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV)) |
		((uint32_t) source << CGU_CON1_FPI1_CLKSEL_SHIFT) |
		(divider << CGU_CON1_FPI1_CLKDIV_SHIFT);
}
