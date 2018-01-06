#include <pmb8876.h>
#include "main.h"

void main() {
	init_sdram();
	enable_irq(0);
	enable_fiq(0);
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	init_watchdog();
	
	int i;
	void **vectors = (void **) 0;
	vectors[8] = reset_addr;
	vectors[9] = undef_addr;
	vectors[10] = loop;
	vectors[11] = prefetch_addr;
	vectors[12] = abort_addr;
	vectors[13] = loop;
	vectors[14] = loop;
	vectors[15] = loop;
	
	i = 0;
	
	REG(0xF43000D0) = 0x700;
	
	int state;
	while (1) {
		if (i % 200000 == 0) {
			REG(0xF4300064) = state ? 0x700 : 0x500;
			state = !state;
		}
		serve_watchdog();
		++i;
	}
}

void __IRQ reset_addr() {
	pmb8876_serial_print("\n***** reset_addr! *****\n");
}
void __IRQ undef_addr() {
	pmb8876_serial_print("\n***** undef_addr! *****\n");
}
void __IRQ prefetch_addr() {
	pmb8876_serial_print("\n***** prefetch_addr! *****\n");
	while (1);
}
void __IRQ abort_addr() {
	pmb8876_serial_print("\n***** abort_addr! *****\n");
	while (1);
}
void __IRQ loop() {
	while (1);
}
