#pragma once

#include <stdint.h>

enum cgu_fsys_source {
	CGU_FSYS_OSC,
	CGU_FSYS_PLL = 2,
};

enum cgu_ahb_source {
	CGU_AHB_OSC,
	CGU_AHB_DISABLED,
	CGU_AHB_PLL,
	CGU_AHB_PHASE1,
	CGU_AHB_PHASE2,
	CGU_AHB_PHASE3,
	CGU_AHB_PHASE4,
};

enum cgu_fpi1_source {
	CGU_FPI1_OSC,
	CGU_FPI1_CLK32K,
	CGU_FPI1_PLL_DIV_2,
	CGU_FPI1_DISABLED,
};

enum cgu_phase_mask {
	CGU_PHASE1 = 1 << 0,
	CGU_PHASE2 = 1 << 1,
	CGU_PHASE3 = 1 << 2,
	CGU_PHASE4 = 1 << 3,
	CGU_PHASE_ALL = CGU_PHASE1 | CGU_PHASE2 | CGU_PHASE3 | CGU_PHASE4,
};

void cgu_pll_set(uint32_t ndiv, uint32_t mdiv);
void cgu_phases_enable(uint32_t phases);
void cgu_phase_set_divider(uint32_t phase, uint32_t k1, uint32_t k2);
void cgu_fsys_select(enum cgu_fsys_source source);
void cgu_ahb_select(enum cgu_ahb_source source);
void cgu_fpi1_select(enum cgu_fpi1_source source, uint32_t divider);
