#include <pmb887x.h>
#include <printf.h>

void send_flash_cmd(uint32_t addr, uint8_t cmd);
void send_flash_reset(void);
uint16_t read_otp_reg(uint32_t reg);

int main(void) {
	wdt_init();
	
	// Инифицализируем флеш
	EBU_ADDRSEL(0) = 0xA0000011;
	EBU_ADDRSEL(4) = 0xA0000011;
	EBU_BUSCON(0) = 0x00522600;
	EBU_BUSCON(4) = 0x00522600;
	
	// ESN
	printf("ESN: ");
	for (int i = 0x81; i <= 0x84; i++) {
		uint16_t v = read_otp_reg(i);
		
		uint8_t hi = v & 0xFF;
		uint8_t lo = (v >> 8) & 0xFF;
		
		printf("%02X%02X", hi, lo);
	}
	printf("\n");
	
	// IMEI
	printf("IMEI: ");
	for (int i = 0x8A; i <= 0x8D; i++) {
		uint16_t v = read_otp_reg(i);
		
		uint8_t hi = v & 0xFF;
		uint8_t lo = (v >> 8) & 0xFF;
		
		printf("%02X%02X", hi, lo);
	}
	printf("\n");
	
	// CFI
	printf("CFI:\n");
	for (int i = 0x10; i <= 0x32; i++) {
		uint16_t v = read_otp_reg(i);
		printf("pfl->cfi_table[0x%02X] = 0x%02X;\n", i, v & 0xFF);
	}
	printf("\n");
	
	uint32_t pri_addr = ((read_otp_reg(0x16) & 0xFF) << 8) | (read_otp_reg(0x15) & 0xFF);
	
	// PRI
	printf("PRI at %04X\n", pri_addr);
	for (uint32_t i = pri_addr; i <= pri_addr + 0x50; i++) {
		uint16_t v = read_otp_reg(i);
		printf("pfl->cfi_table[0x%02X] = 0x%02X;\n", i - pri_addr, v & 0xFF);
	}
	printf("\n");
	
	while (1);
}

void send_flash_cmd(uint32_t addr, uint8_t cmd) {
	REG_SHORT(0xA0000AAA) = 0xAA;
	REG_SHORT(0xA0000554) = 0x55;
	REG_SHORT(addr) = cmd;
}

void send_flash_reset(void) {
	REG_SHORT(0xA0000AAA) = 0xFF;
}

uint16_t read_otp_reg(uint32_t reg) {
	uint16_t v;
	send_flash_cmd(0xA0000AAA, 0x90);
	v = REG_SHORT(0xA0000000 + reg * 2);
	send_flash_reset();
	return v;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
