#include <pmb8876.h>

#include "main.h"

void send_flash_cmd(unsigned int addr, unsigned char cmd);
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
	
	/*
	unsigned int before, after, after2;
	
	unsigned int seg = 0xA2000000;
	
	for (unsigned int i = seg; i < seg + 0x1000000; i += 4) {
		before = REG(i);
		
		send_flash_cmd(seg, 0x70);
		after = REG(i);
		send_flash_reset();
		
		send_flash_cmd(seg, 0x90);
		after2 = REG(i);
		send_flash_reset();
		
		if (before == after && before == after2) {
			hexnum(&before, 4);
			pmb8876_serial_print(" == ");
			hexnum(&after, 4);
			pmb8876_serial_print(" == ");
			hexnum(&after2, 4);
			pmb8876_serial_print("\n");
			
			hexnum(&i, 4);
			pmb8876_serial_print(" - MATCH");
			pmb8876_serial_print("\n");
			break;
		} else {
			if (i % 0x10000 == 0) {
				hexnum(&i, 4);
				pmb8876_serial_print("\n");
			}
			serve_watchdog();
		}
	}
	*/
	
	// ESN
	pmb8876_serial_print("ESN: ");
	for (i = 0x81; i <= 0x84; i++) {
		unsigned short v = read_otp_reg(i);
		
		unsigned char hi = v & 0xFF;
		unsigned char lo = (v >> 8) & 0xFF;
		
		hexnum(&hi, 1);
		hexnum(&lo, 1);
	}
	pmb8876_serial_print("\n");
	
	// IMEI
	pmb8876_serial_print("IMEI: ");
	for (i = 0x8A; i <= 0x8D; i++) {
		unsigned short v = read_otp_reg(i);
		
		unsigned char hi = v & 0xFF;
		unsigned char lo = (v >> 8) & 0xFF;
		
		hexnum(&hi, 1);
		hexnum(&lo, 1);
	}
	pmb8876_serial_print("\n");
	
	// CFI
	pmb8876_serial_print("CFI:\n");
	for (i = 0x10; i <= 0x32; i++) {
		unsigned short v = read_otp_reg(i);
		
		unsigned char hi = v & 0xFF;
		pmb8876_serial_print("pfl->cfi_table[0x");
		hexnum(&i, 1);
		pmb8876_serial_print("] = 0x");
		hexnum(&hi, 1);
		pmb8876_serial_print(";\n");
	}
	pmb8876_serial_print("\n");
	
	unsigned int pri_addr = ((read_otp_reg(0x16) & 0xFF) << 8) | (read_otp_reg(0x15) & 0xFF);
	
	// PRI
	pmb8876_serial_print("PRI at ");
	hexnum(&pri_addr, 4);
	pmb8876_serial_print("\n");
	for (i = pri_addr; i <= pri_addr + 0x50; i++) {
		unsigned short v = read_otp_reg(i);
		
		unsigned int off = i - pri_addr;
		
		unsigned char hi = v & 0xFF;
		pmb8876_serial_print("pfl->pri_table[0x");
		hexnum(&off, 1);
		pmb8876_serial_print("] = 0x");
		hexnum(&hi, 1);
		pmb8876_serial_print(";\n");
	}
	pmb8876_serial_print("\n");
	
	while (1);
}

void send_flash_cmd(unsigned int addr, unsigned char cmd) {
	REG_SHORT(0xA0000AAA) = 0xAA;
	REG_SHORT(0xA0000554) = 0x55;
	REG_SHORT(addr) = cmd;
}

void send_flash_reset() {
	REG_SHORT(0xA0000AAA) = 0xFF;
}

unsigned short read_otp_reg(unsigned int reg) {
	unsigned short v;
	send_flash_cmd(0xA0000AAA, 0x90);
	v = REG_SHORT(0xA0000000 + reg * 2);
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
