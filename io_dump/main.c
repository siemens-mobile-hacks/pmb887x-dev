#include <pmb8876.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "main.h"

extern void da_handler();
const char *get_mod_name(unsigned int id);

static void debug(uint32_t addr) {
	hexnum(&addr, 4);
	pmb8876_serial_print(": ");
	
	uint32_t v = REG(addr);
	hexnum(&v, 4);
	pmb8876_serial_print("\n");
}

void main() {
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	init_watchdog();
	
	void **vectors = (void **) 0;
	vectors[8] = (void *) da_handler;
	vectors[9] = (void *) da_handler;
	vectors[10] = (void *) loop;
	vectors[11] = (void *) da_handler;
	vectors[12] = (void *) da_handler;
	vectors[13] = (void *) loop;
	vectors[14] = (void *) loop;
	vectors[15] = (void *) loop;
	
	REG(EBU_ADDRSEL0) = 0xA0000001;//w
	REG(EBU_ADDRSEL1) = 0;//nw
	REG(EBU_ADDRSEL2) = 0;//nw
	REG(EBU_ADDRSEL3) = 0;//nw
	REG(EBU_ADDRSEL4) = 0;//w
	REG(EBU_ADDRSEL5) = 0;//nw
	REG(EBU_ADDRSEL6) = 0;//nw
	
	REG(EBU_BUSCON0) = 0xA2520E00;
	REG(EBU_BUSCON4) = 0x80522600;
	
	debug(0xA000003C);
	debug(0xa3e9154c);
	
	pmb8876_serial_print("Done\n");
	while (1);
}

void __IRQ loop() {
	while (1);
}
