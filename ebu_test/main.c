#include <pmb8876.h>
#include <i2c.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <printf.h>

void inf_loop();

void main() {
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	
	enable_irq(0);
	enable_fiq(0);
	init_watchdog();
	
	int i;
	void **vectors = (void **) 0;
	vectors[8] = inf_loop; // reset
	vectors[9] = inf_loop; // undef
	vectors[10] = inf_loop; // svc
	vectors[11] = inf_loop; // prefetch
	vectors[12] = inf_loop; // abort
	vectors[13] = inf_loop; // not used
	vectors[14] = inf_loop; // irq
	vectors[15] = inf_loop; // fiq
	
	while (true)
		ee_printf("xuj: %08X\r\n", 1);
}

void __IRQ inf_loop() {
	while (true);
}
