#include <pmb887x.h>

#include "clock.h"

__SRAM void cgu_pll_set(uint32_t ndiv, uint32_t mdiv) {
	uint32_t core_mask = CGU_OSC_NDIV | CGU_OSC_MDIV | CGU_OSC_PLL_POWER_UP;
	uint32_t core = (ndiv << CGU_OSC_NDIV_SHIFT) | (mdiv << CGU_OSC_MDIV_SHIFT) | CGU_OSC_PLL_POWER_UP;
	uint32_t osc = (CGU_OSC & ~(core_mask | CGU_OSC_PLL_BYPASS_N)) | core;

	CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
	bool is_core_running = (CGU_OSC & core_mask) == core && (CGU_STAT & CGU_STAT_LOCK) != 0;
	if (!is_core_running) {
		CGU_OSC = osc & ~CGU_OSC_PLL_POWER_UP;
		CGU_OSC = osc;
		while ((CGU_STAT & CGU_STAT_LOCK) == 0);
	}
	CGU_OSC = osc | CGU_OSC_PLL_BYPASS_N;
}

/* Bypassing only the main PLL output keeps the core and the phase outputs, including CLK48M, running. */
__SRAM void cgu_pll_output_bypass(void) {
	CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
	CGU_OSC &= ~CGU_OSC_PLL_BYPASS_N;
}

__SRAM uint32_t cgu_pll_enable(void) {
	bool is_core_running = (CGU_OSC & CGU_OSC_PLL_POWER_UP) != 0 && (CGU_STAT & CGU_STAT_LOCK) != 0;
	if (is_core_running) {
		CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
		CGU_OSC |= CGU_OSC_PLL_BYPASS_N;
	} else {
		cgu_pll_set(3, 0);
	}
	return cpu_get_pll_freq();
}

void cgu_pll_output_configure(bool is_bypassed) {
	if (is_bypassed) {
		cgu_pll_output_bypass();
	} else {
		cgu_pll_enable();
	}
}

__SRAM void cgu_firmware_pll_enable(void) {
	uint32_t timings = EBU_SDRMCON_CRP | EBU_SDRMCON_CRCD | EBU_SDRMCON_CRC | EBU_SDRMCON_CRCE;
	EBU_SDRMCON(0) = (EBU_SDRMCON(0) & ~timings) | (0x00D7A870 & timings);
	EBU_SDRMREF(0) = (EBU_SDRMREF(0) & ~EBU_SDRMREF_REFRESHC) | (0x00000A19 & EBU_SDRMREF_REFRESHC);
	EBU_SDRMOD(0) = (EBU_SDRMOD(0) & ~EBU_SDRMOD_CASLAT) | (0x00000033 & EBU_SDRMOD_CASLAT);

	cgu_pll_set(3, 0);
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_EBU_CLKSEL) | CGU_CON2_EBU_CLKSEL_PLL;
	CGU_CON1 |= CGU_CON1_AHB_CLKSEL_PLL;
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
