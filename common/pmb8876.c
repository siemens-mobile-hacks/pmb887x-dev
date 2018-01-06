#include "pmb8876.h"


struct watchdog {
	unsigned int addr;
	unsigned int time;
	unsigned int cnt;
} __g_watchdog;

__attribute__ ((weak))
unsigned int max_wd_time = 0;


void pmb8876_serial_set_speed(unsigned int speed) {
	REG(PMB8876_USART0_BG) = ((speed >> 16));
	REG(PMB8876_USART0_FDV) = ((speed << 16) >> 16);
}

void pmb8876_serial_print(const char *data) {
	while (*data)
		pmb8876_serial_putc(*data++);
}

char pmb8876_serial_has_byte() {
	return REG(PMB8876_USART0_FCSTAT) & 4;
}

void pmb8876_serial_putc(char c) {
	REG(PMB8876_USART0_TXB) = c;
	while (!(REG(PMB8876_USART0_FCSTAT) & 2));
	REG(PMB8876_USART0_ICR) |= 2;
}

char pmb8876_serial_getc() {
	while (!(REG(PMB8876_USART0_FCSTAT) & 4));
	REG(PMB8876_USART0_ICR) |= 4;
	return REG(PMB8876_USART0_RXB) & 0xFF;
}

unsigned int get_cpsr() {
	volatile unsigned int r;
	asm volatile (" mrs %0, CPSR":"=r" (r): /* no inputs */ );
	return r;
}

unsigned int set_cpsr(volatile unsigned int r) {
	asm volatile (" msr CPSR, %0": /* no outputs */ :"r" (r));
}

const char *get_cpu_mode(unsigned int cpsr) {
	switch (cpsr & 0x1f) {
		case ARM_CPU_MODE_USR:	return "USR\n";
		case ARM_CPU_MODE_FIQ:	return "FIQ\n";
		case ARM_CPU_MODE_IRQ:	return "IRQ\n";
		case ARM_CPU_MODE_SVC:	return "SVC\n";
		case ARM_CPU_MODE_ABT:	return "ABT\n";
		case ARM_CPU_MODE_UND:	return "UND\n";
		case ARM_CPU_MODE_SYS:	return "SYS\n";
		default:				return "UNK\n";
	}
}

void init_sdram() {
	// Инициализация SDRAM
	REG(EBU_ADDRSEL1) = 0xA8000031;
	REG(EBU_BUSCON1) = 0x30720200;
	
	REG(EBU_SDRMREF0) = 0x00000019;
	
	REG(EBU_SDRMCON0) = 0x0FFFA870;
	
	REG(EBU_SDRMOD0) = 0x00000033;
	
	REG(EBU_ADDRSEL0) = 0xA0000011;
	REG(EBU_ADDRSEL4) = 0xA0000011;
	
	REG(EBU_BUSCON0) = 0xA2520E00;
	REG(EBU_BUSCON4) = 0x80522600;
}

static void _init_watchdog(int noinit) {
	unsigned int r0 = (REG(SCU_CHIPID) >> 8) & 0xFF;
	
	unsigned int r1 = GPIO_DSPOUT1_PM_WADOG;
	unsigned int r2 = GPIO_DSPOUT1_PM_WADOG - 0xCC;
	
	if (r0 == 0x14) {
		r1 += 0x60;
		r2 += 0x04;
	}
	unsigned int r3 = r2 - 0x4;
	unsigned int r4 = r3 - 0x0c;
	
	if( !noinit ) {
	    REG(r2) = 1;
	    REG(r3) = 0x10;
	    REG(r1) = 0x500;
	    REG(r4) = 0x4000 | 0x510;
	}
	
	__g_watchdog.time = noinit? 0 : REG(STM_4);
	__g_watchdog.addr = r1;
	__g_watchdog.cnt  = 0;
	
	if( !noinit )
	    switch_watchdog();
}

void init_watchdog() {
    _init_watchdog(0);
}

void init_watchdog_noinit() {
    _init_watchdog(1);
}

void switch_watchdog() {
	if( max_wd_time && __g_watchdog.cnt > max_wd_time )
		return;
	
	unsigned int r2 = REG(__g_watchdog.addr);
	unsigned int r0 = r2 << 22;
	r0 = r0 >> 31;
	r0 = ~r0;
	r0 = r0 & 1;
	r0 = r0 << 9;
	
	r2 = r2 & ~0x200;
	REG(__g_watchdog.addr) = r0 | r2;
	
	__g_watchdog.cnt++;
}

void serve_watchdog() {
	unsigned int now = REG(STM_4);
	if (now - __g_watchdog.time < 0x200)
		return;
	switch_watchdog();
	 __g_watchdog.time = now;
}

void hexnum(void *d, unsigned int len) {
	unsigned char *data = (unsigned char *) d;
	unsigned int i;
	for (i = len; i-->0; ) {
		pmb8876_serial_putc(to_hex((data[i] >> 4) & 0xF));
		pmb8876_serial_putc(to_hex(data[i] & 0xF));
	}
}

void hexdump(void *d, unsigned int len) {
	unsigned char *data = (unsigned char *) d;
	unsigned int i;
	for (i = 0; i < len; ++i) {
		pmb8876_serial_putc(to_hex((data[i] >> 4) & 0xF));
		pmb8876_serial_putc(to_hex(data[i] & 0xF));
		pmb8876_serial_putc(' ');
	}
}

void *memset(void *a, int c, int size) {
    char *s = (char *)a;
    while(size--)
	*s++ = c;
    return s;
}

void *memcpy(void *a, const void *b, unsigned int size) {
	unsigned char *ap = (unsigned char *) a;
	const unsigned char *bp = (const unsigned char *) b;
	
	unsigned int i = 0;
	for (i = 0; i < size; ++i)
		ap[i] = bp[i];

	return ((char *)a) + size;
}

char to_hex(unsigned char b) {
	if (b < 0xA)
		return '0' + b;
	return 'A' + (b - 10);
}

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

void disable_interrapts() {
	unsigned int cpsr;
	asm volatile ("MRS %0, cpsr" : "=r" (cpsr) : );
	cpsr |= 0xC0;
	asm volatile ("MSR  CPSR_c, %0" :  : "r" (cpsr));
}

void disable_first_whatchdog() {
	REG(SCU_WDTCON1) = 0x8;
}

void set_einit(char flag) {
	
	// ldr r3, =0xf4400000
	// ldr r1, [r3, #0x24] ;SCU_WDTCON0
	unsigned int wdc0 = REG(SCU_WDTCON0);
	
	//  bic r1, r1, #0x0e
	//  orr r1, r1, #0xf0
	wdc0 &= ~0x0E;
	wdc0 |= 0xf0;
	
	// ldr r2, [r3, #0x28] ;SCU_WDTCON1
	// and r2, r2, #0x0c
	unsigned int wdc1 = REG(SCU_WDTCON1);
	wdc1 &= 0x0c;
	
	// orr r1, r1, r2
	// str r1, [r3, #0x24] ;SCU_WDTCON0
	wdc0 |= wdc1; 
	REG(SCU_WDTCON0) = wdc0;
	
	// bic r1, r1, #0x0d
	// orr r1, r1, #2
	// orr r0, r0, r1
	wdc0 &= ~0x0d;
	wdc0 |= 2;
	wdc0 |= flag;
	REG(SCU_WDTCON0) = wdc0;
}

const char *itoa(unsigned int val, unsigned int base) {
	static char buf[32] = {0};
	unsigned int i = 30;
	for (; val && i ; --i, val /= base)
		buf[i] = "0123456789ABCDEF"[val % base];
	return &buf[i + 1];
}

// (c) Dimadze
// http://forum.allsiemens.com/viewtopic.php?t=61096&start=330
unsigned int get_cpu_freq() {
	unsigned char mul, div, pdiv, fsys, divh, mulh;
	unsigned int  pll_osc, pll_con0, pll_con1, pll_con2;
	
	pll_osc  = REG(PLL_OSC);
	pll_con0 = REG(PLL_CON0);
	pll_con1 = REG(PLL_CON1);
	pll_con2 = REG(PLL_CON2);
	
	div  = (pll_osc  & 0x0F000000) >> 24;
	mul  = (pll_osc  & 0x003F0000) >> 16;
	pdiv = (pll_con2 & 0x00000300) >> 8;
	
	divh = (pll_con0 & 0x00007800) >> 11;
	mulh = (pll_con1 & 0x00600000) >> 21;
	
	fsys = ((26 * (mul + 1) ) / ( div + 1)) / 4;
  
	if (mulh > 1) { // Alt Check
		if (divh > 0) {
			if (pll_con2 & 0x00001000) { // High freq
				return  ((fsys * 4 *  mulh) / ( pdiv + 1)) / divh;
			} else {
				switch (pdiv) {
					case 0:
						return  ((fsys * 4 *  mulh) * 1)   / divh;
					case 1:
						return  (((fsys * 4 *  mulh) * 7500) / 10000) / divh;
					case 2:
						return  (((fsys * 4 *  mulh) * 6250) / 10000) / divh;
					case 3:
						return  (((fsys * 4 *  mulh) * 5625) / 10000) / divh;
					default:
						return fsys;
				} 
			}
		} else  {
			// Low freq
			return fsys;
		}
	} else {
		// Normal freq
		if (pll_con2 & 0x00001000) {
			return  ((fsys * 4 *  mulh) / ( pdiv + 1));
		} else {
			switch (pdiv) {
				case 0:
					return  ( fsys * 4 *  mulh) * 1;
				case 1:
					return  ((fsys * 4 *  mulh) * 7500 ) / 10000;
				case 2:
					return  ((fsys * 4 *  mulh) * 6250 ) / 10000;
				case 3:
					return  ((fsys * 4 *  mulh) * 5625 ) / 10000;
				default:
					return fsys;
			}
		}
	}
}
