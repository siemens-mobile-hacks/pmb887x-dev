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
uint32_t cpu_get_ahb_freq(void);
