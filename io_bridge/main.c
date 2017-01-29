#include "main.h"

void reset_addr();
void undef_addr();
void swi_addr();
void prefetch_addr();
void abort_addr();
void reserved_addr();
void irq_test();
void fiq_test();
void main();

unsigned int irq_n;
unsigned int irq_n2;
void _start() {
	irq_n = 0;
	irq_n2 = 0;
	
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	
	enable_irq(0);
	enable_fiq(0);
	init_watchdog();
	
	int i;
	void **vectors = (void **) 0;
	for (i = 0; i < 8; ++i)
		vectors[i] = (&_cpu_vectors)[i];
	vectors[8] = reset_addr;
	vectors[9] = undef_addr;
	vectors[10] = swi_addr;
	vectors[11] = prefetch_addr;
	vectors[12] = abort_addr;
	vectors[13] = reserved_addr;
	vectors[14] = irq_test;
	vectors[15] = fiq_test;
	
	enable_irq(1);
	enable_fiq(1);
	
	main();
}

void main() {
	unsigned int value, addr;
	while (1) {
		char c = pmb8876_serial_getc();
		if (c == '.') {
			serve_watchdog();
			pmb8876_serial_putc('.');
		} else if (c == 'R') {
			serve_watchdog();
			addr = pmb8876_serial_getc() << 24 | pmb8876_serial_getc() << 16 | pmb8876_serial_getc() << 8 | pmb8876_serial_getc();
			value = REG(addr);
			
			pmb8876_serial_putc((value >> 0 ) & 0xFF);
			pmb8876_serial_putc((value >> 8 ) & 0xFF);
			pmb8876_serial_putc((value >> 16) & 0xFF);
			pmb8876_serial_putc((value >> 24) & 0xFF);
			pmb8876_serial_putc(';');
			serve_watchdog();
		} else if (c == 'W') {
			serve_watchdog();
			addr = pmb8876_serial_getc() << 24 | pmb8876_serial_getc() << 16 | pmb8876_serial_getc() << 8 | pmb8876_serial_getc();
			value = pmb8876_serial_getc() << 24 | pmb8876_serial_getc() << 16 | pmb8876_serial_getc() << 8 | pmb8876_serial_getc();
			
			REG(addr) = value;
			pmb8876_serial_putc(';');
			serve_watchdog();
		}
	}
}
void onerr(char c) {
	while (1) {
		pmb8876_serial_putc(c);
	}
}
void reset_addr() {
	onerr(0xAA);
}
void undef_addr() {
	onerr(0xBB);
}
void swi_addr() {
	
}
void prefetch_addr() {
	onerr(0xCC);
}
void abort_addr() {
	onerr(0xEE);
}
void reserved_addr() {
	
}
void __IRQ irq_test() {
	// REG(0xF2800014) = 1;
	onerr(REG(0xF280001C) & 0xFF);
}
void fiq_test() {
	onerr(0xDD);
}
