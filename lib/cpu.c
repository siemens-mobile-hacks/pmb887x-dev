#include "cpu.h"
#include "printf.h"

static uint32_t phase_freq(uint32_t pll_freq, uint32_t k1, uint32_t k2) {
	if (pll_freq == 0 || k1 == 0 || k2 > 5)
		return 0;

	uint32_t divider = k1 * 6 + k2;
	return (pll_freq / divider) * 12 + ((pll_freq % divider) * 12) / divider;
}

static uint32_t pll_freq(void) {
	if ((PLL_OSC & PLL_OSC_PLL_BYPASS_N) == 0)
		return CPU_OSC_FREQ;
	if ((PLL_OSC & PLL_OSC_PLL_POWER_UP) == 0 || (PLL_STAT & PLL_STAT_LOCK) == 0)
		return 0;

	uint32_t ndiv = (PLL_OSC & PLL_OSC_NDIV) >> PLL_OSC_NDIV_SHIFT;
	uint32_t mdiv = (PLL_OSC & PLL_OSC_MDIV) >> PLL_OSC_MDIV_SHIFT;
	return CPU_OSC_FREQ * (ndiv + 1) / (mdiv + 1);
}

uint32_t cpu_get_sys_freq(void) {
	uint32_t clksel = PLL_CON1 & PLL_CON1_FSYS_CLKSEL;
	if (clksel == PLL_CON1_FSYS_CLKSEL_BYPASS)
		return CPU_OSC_FREQ;
	if (clksel == PLL_CON1_FSYS_CLKSEL_PLL)
		return pll_freq() / 2;
	return 0;
}

uint32_t cpu_get_stm_freq(void) {
	if ((PLL_CON1 & PLL_CON1_FSTM_DIV_EN) != 0) {
		uint32_t divider = (PLL_CON1 & PLL_CON1_FSTM_DIV) >> PLL_CON1_FSTM_DIV_SHIFT;
		return CPU_OSC_FREQ >> (divider + 2);
	}
	return CPU_OSC_FREQ;
}

uint32_t cpu_get_freq(void) {
	uint32_t ahb_freq = cpu_get_ahb_freq();
	if ((PLL_CON2 & PLL_CON2_CPU_DIV_EN) != 0) {
		uint32_t divider = ((PLL_CON2 & PLL_CON2_CPU_DIV) >> PLL_CON2_CPU_DIV_SHIFT) + 1;
		return ahb_freq / divider;
	}
	return ahb_freq;
}

uint32_t cpu_get_ahb_freq(void) {
	uint32_t k1;
	uint32_t k2;

	switch (PLL_CON1 & PLL_CON1_AHB_CLKSEL) {
		case PLL_CON1_AHB_CLKSEL_BYPASS:
			return CPU_OSC_FREQ;
		case PLL_CON1_AHB_CLKSEL_PLL0:
			return pll_freq();
		case PLL_CON1_AHB_CLKSEL_PLL1:
			k1 = (PLL_CON0 & PLL_CON0_PLL1_K1) >> PLL_CON0_PLL1_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL1_K2) >> PLL_CON0_PLL1_K2_SHIFT;
			return phase_freq(pll_freq(), k1, k2);
		case PLL_CON1_AHB_CLKSEL_PLL2:
			k1 = (PLL_CON0 & PLL_CON0_PLL2_K1) >> PLL_CON0_PLL2_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL2_K2) >> PLL_CON0_PLL2_K2_SHIFT;
			return phase_freq(pll_freq(), k1, k2);
		case PLL_CON1_AHB_CLKSEL_PLL3:
			k1 = (PLL_CON0 & PLL_CON0_PLL3_K1) >> PLL_CON0_PLL3_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL3_K2) >> PLL_CON0_PLL3_K2_SHIFT;
			return phase_freq(pll_freq(), k1, k2);
		case PLL_CON1_AHB_CLKSEL_PLL4:
			k1 = (PLL_CON0 & PLL_CON0_PLL4_K1) >> PLL_CON0_PLL4_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL4_K2) >> PLL_CON0_PLL4_K2_SHIFT;
			return phase_freq(pll_freq(), k1, k2);
		default:
			return 0;
	}
}
