#include "cpu.h"

void enable_irq(int flag) {
	unsigned int cpsr;
	asm volatile ("MRS %0, cpsr" : "=r" (cpsr) : );
	cpsr = !flag ? cpsr | 0x80 : cpsr & ~0x80;
	asm volatile ("MSR  CPSR_c, %0" :  : "r" (cpsr));
}

void enable_fiq(int flag) {
	unsigned int cpsr;
	asm volatile ("MRS %0, cpsr" : "=r" (cpsr) : );
	cpsr = !flag ? cpsr | 0x40 : cpsr & ~0x40;
	asm volatile ("MSR  CPSR_c, %0" :  : "r" (cpsr));
}
