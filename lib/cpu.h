#pragma once

#include <pmb887x.h>

static inline bool cpu_enable_irq_or_fiq(bool enable, uint32_t bit) {
	uint32_t cpsr;
	__asm__ volatile("MRS %0, cpsr" : "=r" (cpsr) : );
	__asm__ volatile("MSR  CPSR_c, %0" :  : "r" (enable ? cpsr & ~bit : cpsr | bit));
	return (cpsr & bit) != 0;
}

static inline bool cpu_enable_irq(bool flag) {
	return cpu_enable_irq_or_fiq(flag, 0x80);
}

static inline bool cpu_enable_fiq(bool flag) {
	return cpu_enable_irq_or_fiq(flag, 0x40);
}

uint32_t cpu_get_freq(void);
uint32_t cpu_get_pll_freq(void);
uint32_t cpu_get_phase_freq(uint32_t phase);
uint32_t cpu_get_ahb_freq(void);
uint32_t cpu_get_ebu_freq(void);
uint32_t cpu_get_dsp_freq(void);
uint32_t cpu_get_fpi1_freq(void);
uint32_t cpu_get_ahb_per_freq(void);
uint32_t cpu_get_sys_freq(void);
uint32_t cpu_get_stm_freq(void);
uint32_t cpu_get_clk48m_freq(void);
uint32_t cpu_get_mmci_freq(void);
uint32_t cpu_get_clkout0_freq(void);
uint32_t cpu_get_clkout1_freq(void);
uint32_t cpu_get_clkout2_freq(void);
uint32_t cpu_get_clk32k_freq(void);
uint32_t cpu_get_ms_freq(void);
uint32_t cpu_get_dma_freq(void);
