#include "main.h"

extern void asm_irq_handler();

void reset_addr();
void undef_addr();
void swi_addr();
void prefetch_addr();
void abort_addr();
void reserved_addr();
void irq_test();
void fiq_test();
void __IRQ c_irq_handler();
void main();

void irq_handler();

void _start() {
	init_sdram();
	enable_irq(0);
	enable_fiq(0);
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
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
	vectors[14] = c_irq_handler; // asm_irq_handler;
	vectors[15] = fiq_test;
	
	pmb8876_serial_print("init!\n");
	unsigned int addr;
	for (addr = 0xf2800030; addr <= 0xf28002a8; ++addr) {
		REG(addr) = 0;
	}
	
	#define XUJ(xxx) {\
		pmb8876_serial_print(#xxx " = ");\
		addr = REG(xxx);\
		hexdump((const char *) &addr, 4);\
		pmb8876_serial_print("\n");\
	}
	
	enable_irq(1);
	enable_fiq(1);
	
//	pmb8876_serial_print("CPU F: ");
//	pmb8876_serial_print(itoa(get_cpu_freq(), 10));
//	pmb8876_serial_print("\n");
	
	REG(0xF6400000) = 0x100;
	REG(0xF6400020) = 9999; // 9999
	REG(0xF6400024) = 0x00000000;
	REG(0xF6400028) = 0x00007530;
	REG(0xF640002C) = 0x00000000;
	REG(0xF640003C) = 0x00000006;
	REG(0xF6400040) = 0x00000000;
	REG(0xF6400044) = 0x80000000;
	REG(0xF640005C) = 0x00000003;
	REG(0xF6400068) = 0x00000001;
	REG(0xF640006C) = 0x00000004;
	REG(0xF6400070) = 0x00000002;
	REG(0xF64000F8) = 0x00004000;
	REG(0xF64000F8) = 0x00005000;
	
	PMB8876_IRQ(0x77) = 1;
	for (i = 0; i < 1000000; ++i) {
		if (i % 1000 == 0)
			pmb8876_serial_print(".");
		serve_watchdog();
	}
	pmb8876_serial_print("END :(\n");
}

void __IRQ reset_addr() {
	pmb8876_serial_print("\n***** reset_addr! *****\n");
}
void __IRQ undef_addr() {
	pmb8876_serial_print("\n***** undef_addr! *****\n");
}
void __IRQ swi_addr() {
	
}
void __IRQ prefetch_addr() {
	pmb8876_serial_print("\n***** prefetch_addr! *****\n");
	while (1);
}
void __IRQ abort_addr() {
	pmb8876_serial_print("\n***** abort_addr! *****\n");
	while (1);
}
void __IRQ reserved_addr() {
	
}

void __IRQ c_irq_handler() {
	REG(0xF2800014) = 1;
}

void __IRQ fiq_test() {
	pmb8876_serial_print("fiq_test!\n");
	while (1);
}
