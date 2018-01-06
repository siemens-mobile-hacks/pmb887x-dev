#include <pmb8876.h>

#include "main.h"

void send_flash_cmd(unsigned char cmd);
void send_flash_reset();
unsigned short read_otp_reg(unsigned int reg);

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
	
	// Инифицализируем флеш
	REG(EBU_ADDRSEL0) = 0xA0000011;
	REG(EBU_ADDRSEL4) = 0xA0000011;
	REG(EBU_BUSCON0) = 0x00522600;
	REG(EBU_BUSCON4) = 0x00522600;
	
	for (i = 0; i < 0xFF; ++i) {
		unsigned short v = read_otp_reg(i);
		pmb8876_serial_print("0x");
		hexnum(&v, 2);
		pmb8876_serial_print(", ");
		pmb8876_serial_print("// ");
		hexnum(&i, 1);
		pmb8876_serial_print("\n");
	}
	
	while (1);
}

void send_flash_cmd(unsigned char cmd) {
	REG(0xA0000AAA) = 0xAA;
	REG(0xA0000554) = 0x55;
	REG(0xA0000AAA) = cmd;
}

void send_flash_reset() {
	REG(0xA0000AAA) = 0xFF;
}

unsigned short read_otp_reg(unsigned int reg) {
	unsigned short v;
	send_flash_cmd(0x90);
	v = REG(0xA0000000 + reg * 2);
	send_flash_reset();
	return v;
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
