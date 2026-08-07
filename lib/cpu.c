#include "cpu.h"
#include "printf.h"

static uint32_t phase_freq(uint32_t pll_freq, uint32_t k1, uint32_t k2) {
	if (pll_freq == 0 || k1 == 0 || k2 > 5)
		return 0;

	uint32_t divider = k1 * 6 + k2;
	return (pll_freq / divider) * 12 + ((pll_freq % divider) * 12) / divider;
}

uint32_t cpu_get_pll_freq(void) {
	if ((CGU_OSC & CGU_OSC_PLL_BYPASS_N) == 0)
		return CPU_OSC_FREQ;
	if ((CGU_OSC & CGU_OSC_PLL_POWER_UP) == 0 || (CGU_STAT & CGU_STAT_LOCK) == 0)
		return 0;

	uint32_t ndiv = (CGU_OSC & CGU_OSC_NDIV) >> CGU_OSC_NDIV_SHIFT;
	uint32_t mdiv = (CGU_OSC & CGU_OSC_MDIV) >> CGU_OSC_MDIV_SHIFT;
	return CPU_OSC_FREQ * (ndiv + 1) / (mdiv + 1);
}

uint32_t cpu_get_phase_freq(uint32_t phase) {
	uint32_t power_up;
	uint32_t bypass_n;
	uint32_t k1;
	uint32_t k2;

	switch (phase) {
		case 1:
			power_up = CGU_OSC_PHASE1_POWER_UP;
			bypass_n = CGU_OSC_PHASE1_BYPASS_N;
			k1 = (CGU_CON0 & CGU_CON0_PHASE1_K1) >> CGU_CON0_PHASE1_K1_SHIFT;
			k2 = (CGU_CON0 & CGU_CON0_PHASE1_K2) >> CGU_CON0_PHASE1_K2_SHIFT;
			break;
		case 2:
			power_up = CGU_OSC_PHASE2_POWER_UP;
			bypass_n = CGU_OSC_PHASE2_BYPASS_N;
			k1 = (CGU_CON0 & CGU_CON0_PHASE2_K1) >> CGU_CON0_PHASE2_K1_SHIFT;
			k2 = (CGU_CON0 & CGU_CON0_PHASE2_K2) >> CGU_CON0_PHASE2_K2_SHIFT;
			break;
		case 3:
			power_up = CGU_OSC_PHASE3_POWER_UP;
			bypass_n = CGU_OSC_PHASE3_BYPASS_N;
			k1 = (CGU_CON0 & CGU_CON0_PHASE3_K1) >> CGU_CON0_PHASE3_K1_SHIFT;
			k2 = (CGU_CON0 & CGU_CON0_PHASE3_K2) >> CGU_CON0_PHASE3_K2_SHIFT;
			break;
		case 4:
			power_up = CGU_OSC_PHASE4_POWER_UP;
			bypass_n = CGU_OSC_PHASE4_BYPASS_N;
			k1 = (CGU_CON0 & CGU_CON0_PHASE4_K1) >> CGU_CON0_PHASE4_K1_SHIFT;
			k2 = (CGU_CON0 & CGU_CON0_PHASE4_K2) >> CGU_CON0_PHASE4_K2_SHIFT;
			break;
		default:
			return 0;
	}

	uint32_t pll_freq = cpu_get_pll_freq();
	if ((CGU_OSC & bypass_n) == 0)
		return pll_freq;
	if ((CGU_OSC & power_up) == 0)
		return 0;

	return phase_freq(pll_freq, k1, k2);
}

uint32_t cpu_get_sys_freq(void) {
	switch (CGU_CON1 & CGU_CON1_FSYS_CLKSEL) {
		case CGU_CON1_FSYS_CLKSEL_BYPASS:
			return CPU_OSC_FREQ;
		case CGU_CON1_FSYS_CLKSEL_PLL:
			return cpu_get_pll_freq() / 2;
		default:
			return 0;
	}
}

uint32_t cpu_get_stm_freq(void) {
	if ((CGU_CON1 & CGU_CON1_FSTM_DIV_EN) == 0)
		return CPU_OSC_FREQ;

	uint32_t divider = (CGU_CON1 & CGU_CON1_FSTM_DIV) >> CGU_CON1_FSTM_DIV_SHIFT;
	return CPU_OSC_FREQ >> (divider + 2);
}

uint32_t cpu_get_ahb_freq(void) {
	switch (CGU_CON1 & CGU_CON1_AHB_CLKSEL) {
		case CGU_CON1_AHB_CLKSEL_BYPASS:
			return CPU_OSC_FREQ;
		case CGU_CON1_AHB_CLKSEL_PLL:
			return cpu_get_pll_freq();
		case CGU_CON1_AHB_CLKSEL_PHASE1:
			return cpu_get_phase_freq(1);
		case CGU_CON1_AHB_CLKSEL_PHASE2:
			return cpu_get_phase_freq(2);
		case CGU_CON1_AHB_CLKSEL_PHASE3:
			return cpu_get_phase_freq(3);
		case CGU_CON1_AHB_CLKSEL_PHASE4:
			return cpu_get_phase_freq(4);
		default:
			return 0;
	}
}

uint32_t cpu_get_freq(void) {
	uint32_t ahb_freq = cpu_get_ahb_freq();
	if ((CGU_CON2 & CGU_CON2_CPU_DIV_EN) == 0)
		return ahb_freq;

	uint32_t divider = ((CGU_CON2 & CGU_CON2_CPU_DIV) >> CGU_CON2_CPU_DIV_SHIFT) + 1;
	return ahb_freq / divider;
}

uint32_t cpu_get_ebu_freq(void) {
	switch (CGU_CON2 & CGU_CON2_EBU_CLKSEL) {
		case CGU_CON2_EBU_CLKSEL_OSC:
			return CPU_OSC_FREQ;
		case CGU_CON2_EBU_CLKSEL_PLL:
			return cpu_get_pll_freq();
		case CGU_CON2_EBU_CLKSEL_PHASE1:
			return cpu_get_phase_freq(1);
		case CGU_CON2_EBU_CLKSEL_PHASE2:
			return cpu_get_phase_freq(2);
		case CGU_CON2_EBU_CLKSEL_PHASE3:
			return cpu_get_phase_freq(3);
		case CGU_CON2_EBU_CLKSEL_PHASE4:
			return cpu_get_phase_freq(4);
		case CGU_CON2_EBU_CLKSEL_AHB:
			return cpu_get_ahb_freq();
		default:
			return 0;
	}
}

uint32_t cpu_get_dsp_freq(void) {
	switch (CGU_CON2 & CGU_CON2_DSP_CLKSEL) {
		case CGU_CON2_DSP_CLKSEL_PHASE1:
			return cpu_get_phase_freq(1);
		default:
			return 0;
	}
}

uint32_t cpu_get_fpi1_freq(void) {
	uint32_t frequency;

	switch (CGU_CON1 & CGU_CON1_FPI1_CLKSEL) {
		case CGU_CON1_FPI1_CLKSEL_OSC:
			frequency = CPU_OSC_FREQ;
			break;
		case CGU_CON1_FPI1_CLKSEL_CLK32K:
			return CPU_CLK32K_FREQ;
		case CGU_CON1_FPI1_CLKSEL_PLL_DIV_2:
			frequency = cpu_get_pll_freq() / 2;
			break;
		default:
			return 0;
	}

	uint32_t divider = (CGU_CON1 & CGU_CON1_FPI1_CLKDIV) >> CGU_CON1_FPI1_CLKDIV_SHIFT;
	return frequency >> divider;
}

uint32_t cpu_get_ahb_per_freq(void) {
	uint32_t frequency;

	switch (CGU_CON3 & CGU_CON3_AHB_PER_CLKSEL) {
		case CGU_CON3_AHB_PER_CLKSEL_OSC:
			frequency = CPU_OSC_FREQ;
			break;
		case CGU_CON3_AHB_PER_CLKSEL_CLK32K:
			return CPU_CLK32K_FREQ;
		case CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2:
			frequency = cpu_get_pll_freq() / 2;
			break;
		default:
			return 0;
	}

	uint32_t divider = (CGU_CON3 & CGU_CON3_AHB_PER_CLKDIV) >> CGU_CON3_AHB_PER_CLKDIV_SHIFT;
	return frequency >> divider;
}

uint32_t cpu_get_clk48m_freq(void) {
	uint32_t frequency;

	switch (CGU_CON2 & CGU_CON2_CLK48M_CLKSEL) {
		case CGU_CON2_CLK48M_CLKSEL_OSC:
			frequency = CPU_OSC_FREQ;
			break;
		case CGU_CON2_CLK48M_CLKSEL_PHASE4:
			frequency = cpu_get_phase_freq(4);
			break;
		default:
			return 0;
	}

	uint32_t divider = (CGU_CON3 & CGU_CON3_CLK48M_CLKDIV) >> CGU_CON3_CLK48M_CLKDIV_SHIFT;
	return frequency >> divider;
}

uint32_t cpu_get_mmci_freq(void) {
	uint32_t frequency;

	switch (CGU_CON3 & CGU_CON3_MMCI_CLKSEL) {
		case CGU_CON3_MMCI_CLKSEL_OSC:
			frequency = CPU_OSC_FREQ;
			break;
		case CGU_CON3_MMCI_CLKSEL_CLK32K:
			frequency = CPU_CLK32K_FREQ;
			break;
		case CGU_CON3_MMCI_CLKSEL_PHASE4:
			frequency = cpu_get_phase_freq(4);
			break;
		default:
			return 0;
	}

	uint32_t divider = (CGU_CON3 & CGU_CON3_MMCI_CLKDIV) >> CGU_CON3_MMCI_CLKDIV_SHIFT;
	return frequency >> divider;
}

uint32_t cpu_get_clkout0_freq(void) {
	if ((CGU_CON2 & CGU_CON2_CLKOUT0_EN) == 0)
		return 0;

	uint32_t divider = (CGU_CON2 & CGU_CON2_CLKOUT0_CLKDIV) >> CGU_CON2_CLKOUT0_CLKDIV_SHIFT;
	return CPU_OSC_FREQ >> divider;
}

uint32_t cpu_get_clkout1_freq(void) {
	if ((CGU_CON2 & CGU_CON2_CLKOUT1_EN) == 0)
		return 0;

	uint32_t divider = (CGU_CON2 & CGU_CON2_CLKOUT1_CLKDIV) >> CGU_CON2_CLKOUT1_CLKDIV_SHIFT;
	return CPU_OSC_FREQ >> divider;
}

uint32_t cpu_get_clkout2_freq(void) {
	if ((CGU_CON3 & CGU_CON3_CLKOUT2_EN) == 0)
		return 0;

	uint32_t divider = (CGU_CON3 & CGU_CON3_CLKOUT2_CLKDIV) >> CGU_CON3_CLKOUT2_CLKDIV_SHIFT;
	return cpu_get_phase_freq(4) >> divider;
}

uint32_t cpu_get_clk32k_freq(void) {
	return (CGU_CON2 & CGU_CON2_CLK32K_EN) != 0 ? CPU_CLK32K_FREQ : 0;
}

uint32_t cpu_get_ms_freq(void) {
	switch (CGU_CON2 & CGU_CON2_MS_CLKSEL) {
		case CGU_CON2_MS_CLKSEL_OSC:
			return CPU_OSC_FREQ;
		case CGU_CON2_MS_CLKSEL_CLK32K:
			return CPU_CLK32K_FREQ;
		case CGU_CON2_MS_CLKSEL_OSC_DIV_64:
			return CPU_OSC_FREQ / 64;
		default:
			return 0;
	}
}

uint32_t cpu_get_dma_freq(void) {
	return (CGU_CON3 & CGU_CON3_DMA_CLK_DISABLE) == 0 ? cpu_get_pll_freq() : 0;
}
