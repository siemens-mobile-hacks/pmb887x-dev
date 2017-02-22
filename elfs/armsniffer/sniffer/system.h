#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define __MRC(processor, op1, reg, crn, crm, op2) \
	__asm__ __volatile__ ( \
	"   mrc   p"   #processor "," #op1 ", %0,c"  #crn ",c" #crm "," #op2 "\n" \
	: "=r" (reg))
#define __MCR(processor, op1, reg, crn, crm, op2) \
	__asm__ __volatile__ ( \
	"   mcr   p"   #processor "," #op1 ", %0,c"  #crn ",c" #crm "," #op2 "\n" \
	: : "r" (reg))

__arm void SetDomainAccess(unsigned int domains);
__arm void SetMemoryAccess(unsigned int domains);
__arm void UnlockAllMemoryAccess();
__arm void DisableInterrupt();
__arm void EnableInterrupt();
__arm void EnableModeForOSWork();
__arm void DisableModeForOSWork();

// IAR

void system_mode_sg();
void system_mode_nsg();
void __enable_interrupt();
void __disable_interrupt();

#endif // __SYSTEM_H__