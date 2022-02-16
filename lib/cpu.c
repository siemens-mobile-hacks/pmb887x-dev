#include "cpu.h"
#include "printf.h"

// Apply dividers for AHB freq
static uint32_t _ahb_div(uint32_t freq, uint32_t k1, uint32_t k2) {
	if (freq == 0)
		return 0;
	if (k1 == 0)
		return freq / 8;
	return (freq * 12) / ((k1 * 6) + (k2 >= 1 ? k2 - 1 : 0));
}

// Freq after PLL
static uint32_t _pll_freq(void) {
	// fPLL = fOSC * (NDIV + 1)
	uint32_t ndiv = (PLL_OSC & PLL_OSC_NDIV) >> PLL_OSC_NDIV_SHIFT;
	return CPU_OSC_FREQ * (ndiv + 1);
}

uint32_t cpu_get_sys_freq(void) {
	uint32_t freq = _pll_freq();
	uint32_t clksel = PLL_CON1 & PLL_CON1_FSYS_CLKSEL;
	
	// fSYS=0
	if (clksel == PLL_CON1_FSYS_CLKSEL_DISABLE)
		return 0;
	
	if (clksel == PLL_CON1_FSYS_CLKSEL_PLL) {
		// fSYS = fPLL / 2
		return freq / 2;
	}
	
	// fSYS = fOSC
	return CPU_OSC_FREQ;
}

uint32_t cpu_get_stm_freq(void) {
	uint32_t freq = CPU_OSC_FREQ;
	if ((PLL_CON1 & PLL_CON1_FSTM_DIV_EN)) {
		uint32_t div = (PLL_CON1 & PLL_CON1_FSTM_DIV) >> PLL_CON1_FSTM_DIV_SHIFT;
		return freq / div;
	}
	return freq;
}

// CPU freq from AHB
uint32_t cpu_get_freq(void) {
	uint32_t ahb_freq = cpu_get_ahb_freq();
	if ((PLL_CON2 & PLL_CON2_CPU_DIV_EN)) {
		// fCPU = fAHB / (CPU_DIV + 1)
		uint32_t div = ((PLL_CON2 & PLL_CON2_CPU_DIV) >> PLL_CON2_CPU_DIV_SHIFT) + 1;
		return ahb_freq / div;
	}
	// fCPU = fAHB
	return ahb_freq;
}

// Get AHB bus freq
uint32_t cpu_get_ahb_freq(void) {
	uint32_t k1, k2;
	switch ((PLL_CON1 & PLL_CON1_AHB_CLKSEL)) {
		case PLL_CON1_AHB_CLKSEL_BYPASS:
			// fAHB = fOSC
			return CPU_OSC_FREQ;
		break;
		
		case PLL_CON1_AHB_CLKSEL_PLL0:
			// fAHB = fOSC * (NDIV + 1)
			return _pll_freq();
		break;
		
		case PLL_CON1_AHB_CLKSEL_PLL1:
			// PLL1_K1 > 0:		fAHB = (fOSC * (NDIV + 1) * 12) / (PLL1_K1 * 6 + (PLL1_K2 - 1))
			// PLL1_K1 = 0:		fAHB = (fOSC * (NDIV + 1) * 12) / 16
			k1 = (PLL_CON0 & PLL_CON0_PLL1_K1) >> PLL_CON0_PLL1_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL1_K2) >> PLL_CON0_PLL1_K2_SHIFT;
			return _ahb_div(_pll_freq(), k1, k2);
		break;
		
		case PLL_CON1_AHB_CLKSEL_PLL2:
			// PLL2_K1 > 0:		fAHB = (fOSC * (NDIV + 1) * 12) / (PLL1_K2 * 6 + (PLL2_K2 - 1))
			// PLL2_K1 = 0:		fAHB = (fOSC * (NDIV + 1) * 12) / 16
			k1 = (PLL_CON0 & PLL_CON0_PLL2_K1) >> PLL_CON0_PLL2_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL2_K2) >> PLL_CON0_PLL2_K2_SHIFT;
			return _ahb_div(_pll_freq(), k1, k2);
		break;
		
		case PLL_CON1_AHB_CLKSEL_PLL3:
			// PLL3_K1 > 0:		fAHB = (fOSC * (NDIV + 1) * 12) / (PLL1_K3 * 6 + (PLL3_K2 - 1))
			// PLL3_K1 = 0:		fAHB = (fOSC * (NDIV + 1) * 12) / 16
			k1 = (PLL_CON0 & PLL_CON0_PLL3_K1) >> PLL_CON0_PLL3_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL3_K2) >> PLL_CON0_PLL3_K2_SHIFT;
			return _ahb_div(_pll_freq(), k1, k2);
		break;
		
		case PLL_CON1_AHB_CLKSEL_PLL4:
			// PLL4_K1 > 0:		fAHB = (fOSC * (NDIV + 1) * 12) / (PLL4_K1 * 6 + (PLL4_K2 - 1))
			// PLL4_K1 = 0:		fAHB = (fOSC * (NDIV + 1) * 12) / 16
			k1 = (PLL_CON0 & PLL_CON0_PLL4_K1) >> PLL_CON0_PLL4_K1_SHIFT;
			k2 = (PLL_CON0 & PLL_CON0_PLL4_K2) >> PLL_CON0_PLL4_K2_SHIFT;
			return _ahb_div(_pll_freq(), k1, k2);
		break;
	}
	return 0;
}
