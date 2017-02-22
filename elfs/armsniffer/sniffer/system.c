#include <swilib.h>
#include "system.h"

//#pragma swi_number=0x00
//__swi void system_mode_sg();

//#pragma swi_number=0x04
//__swi void system_mode_nsg();

//#pragma swi_number=0x81B5
//__swi int is_nsg();

void system_mode_sg() {
	asm("SWI 0");
}

void system_mode_nsg() {
	asm("SWI 4");
}

void SetDomainAccess(unsigned int domains) {
	__MCR(15, 0, domains, 3, 0, 0);
	for (volatile int i = 0; i < 20; i++)
		asm volatile("NOP");
}

void SetMemoryAccess(unsigned int domains) {
	if (isnewSGold())
		system_mode_nsg();
	else
		system_mode_sg();
	
	__disable_interrupt();
	SetDomainAccess(domains);
	__enable_interrupt();
}

void UnlockAllMemoryAccess() {
	if (isnewSGold())
		system_mode_nsg();
	else
		system_mode_sg();
	__disable_interrupt();
	SetDomainAccess(0xFFFFFFFF);
	__enable_interrupt();
}

void DisableInterrupt() {
	if (isnewSGold())
		system_mode_nsg();
	else
		system_mode_sg();
	__disable_interrupt();
}

void EnableInterrupt() {
	if (isnewSGold())
		system_mode_nsg();
	else
		system_mode_sg();
	__enable_interrupt();
}

void __enable_interrupt() {
	volatile unsigned int cpsr;
	asm volatile ("MRS %0, cpsr" : "=r" (cpsr) : );
	cpsr &= ~0xC0;
	asm volatile ("MSR  CPSR_c, %0" :  : "r" (cpsr));
}

void __disable_interrupt() {
	volatile unsigned int cpsr;
	asm volatile ("MRS %0, cpsr" : "=r" (cpsr) : );
	cpsr |= 0xC0;
	asm volatile ("MSR  CPSR_c, %0" :  : "r" (cpsr));
}
