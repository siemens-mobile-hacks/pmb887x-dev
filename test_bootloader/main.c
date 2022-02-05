#include "main.h"

void _start() {
	init_sdram();
	disable_interrapts();
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	init_watchdog();
	
	usart_putc('\x0B');
	usart_set_speed(UART_SPEED_115200);
	
	int i;
	while (1) {
		char cmd = usart_getc();
		switch (cmd) {
			case '.': // Keep alive
				serve_watchdog();
			break;
			
			case 'B': // Set speed
			{
				unsigned int speed = usart_getc() << 24 | usart_getc() << 16 | usart_getc() << 8 | usart_getc();
				usart_putc(0xCC);
				
				for (i = 0; i < 0xa000; ++i) asm("NOP");
				
				if (speed == 57600) {
					usart_set_speed(UART_SPEED_57600);
				} else if (speed == 115200) {
					usart_set_speed(UART_SPEED_115200);
				} else if (speed == 230400) {
					usart_set_speed(UART_SPEED_230400);
				} else if (speed == 460800) {
					usart_set_speed(UART_SPEED_460800);
				} else if (speed == 921600) {
					usart_set_speed(UART_SPEED_921600);
				} else if (speed == 1228800) {
					usart_set_speed(UART_SPEED_1228800);
				} else if (speed == 1600000) {
					usart_set_speed(UART_SPEED_1600000);
				} else if (speed == 1500000) {
					usart_set_speed(UART_SPEED_1500000);
				}
				while (usart_getc() != 'A');
				
				usart_putc(0xDD);
				
				serve_watchdog();
			}
			break;
			
			case 'X': // Exec addr
			{
				volatile unsigned int addr = usart_getc() << 24 | usart_getc() << 16 | usart_getc() << 8 | usart_getc();
				serve_watchdog();
				asm volatile ("mov PC, %0" : : "r"(addr));
				// Дальше уже точно не пойдёт ;)
			}
			break;
			
			case 'W': // Write to RAM
			{
				unsigned int addr = usart_getc() << 24 | usart_getc() << 16 | usart_getc() << 8 | usart_getc();
				unsigned int size = usart_getc() << 24 | usart_getc() << 16 | usart_getc() << 8 | usart_getc();
				unsigned char xor = usart_getc();
				serve_watchdog();
				
				unsigned int i;
				unsigned char *data = (unsigned char *) addr, xor2 = 0;
				for (i = 0; i < size; ++i) {
					data[i] = usart_getc();
					xor2 ^= data[i];
					serve_watchdog();
				}
				
				usart_putc(xor == xor2 ? 'O' : 'E');
				serve_watchdog();
			}
			break;
		}
	}
	
}

static void usart_set_speed(unsigned int speed) {
	REG(PMB8876_USART0_BG) = ((speed >> 16));
	REG(PMB8876_USART0_FDV) = ((speed << 16) >> 16);
}

static void usart_print(char *data) {
	while (*data)
		usart_putc(*data++);
}

static void usart_putc(char c) {
	REG(PMB8876_USART0_TXB) = c;
	while (!(REG(PMB8876_USART0_FCSTAT) & 2));
	REG(PMB8876_USART0_ICR) |= 2;
}

static char usart_getc() {
	while (!(REG(PMB8876_USART0_FCSTAT) & 4));
	REG(PMB8876_USART0_ICR) |= 4;
	return REG(PMB8876_USART0_RXB) & 0xFF;
}

void init_sdram() {
	// Инициализация SDRAM
	REG(EBU_ADDRSEL1) = 0xA8000041;
	REG(EBU_BUSCON1) = 0x30720200;
	
	REG(EBU_SDRMREF0) = 6;
	
	REG(EBU_SDRMCON0) = 0x891C70;
	
	REG(EBU_SDRMOD0) = 0x23;
	
	REG(EBU_ADDRSEL0) = 0xA0000011;
	REG(EBU_ADDRSEL4) = 0xA0000011;
	
	REG(EBU_BUSCON0) = 0x00522600;
	REG(EBU_BUSCON4) = 0x00522600;
}

static void init_watchdog() {
	unsigned int r0 = (REG(SCU_CHIPID) >> 8) & 0xFF;
	
	unsigned int r1 = PCL_62;
	unsigned int r2 = PCL_62 - 0xCC;
	
	if (r0 == 0x14) {
		r1 += 0x60;
		r2 += 0x04;
	}
	unsigned int r3 = r2 - 0x4;
	unsigned int r4 = r3 - 0x0c;
	
	REG(r2) = 1;
	REG(r3) = 0x10;
	REG(r1) = 0x500;
	REG(r4) = 0x4000 | 0x510;
	
	__g_watchdog.time = REG(STM_4);
	__g_watchdog.addr = r1;
	
	switch_watchdog();
}

static void switch_watchdog() {
	unsigned int r2 = REG(__g_watchdog.addr);
	unsigned int r0 = r2 << 22;
	r0 = r0 >> 31;
	r0 = ~r0;
	r0 = r0 & 1;
	r0 = r0 << 9;
	
	r2 = r2 & ~0x200;
	
	REG(__g_watchdog.addr) = r0 | r2;
}

static void serve_watchdog() {
	unsigned int now = REG(STM_4);
	if (now - __g_watchdog.time < 0x200)
		return;
	switch_watchdog();
	 __g_watchdog.time = now;
}

static void hexdump(unsigned char *data, unsigned int len) {
	unsigned int i;
	for (i = 0; i < len; ++i) {
		usart_putc(to_hex((data[i] >> 4) & 0xF));
		usart_putc(to_hex(data[i] & 0xF));
		usart_putc(' ');
	}
}

static char to_hex(unsigned char b) {
	if (b < 0xA)
		return '0' + b;
	return 'A' + (b - 10);
}

static void disable_interrapts() {
	unsigned int cpsr;
	asm volatile ("MRS %0, cpsr" : "=r" (cpsr) : );
	cpsr |= 0xC0;
	asm volatile ("MSR  CPSR_c, %0" :  : "r" (cpsr));
}

static void disable_first_whatchdog() {
	REG(SCU_ROMAMCR) &= ~1;
	REG(SCU_WDTCON1) = 0x8;
}

static void set_einit(char flag) {
	
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
