#include <pmb887x.h>
#include <printf.h>

#define CFI_ADDR	0x10
#define CFI_SIZE	0x22
#define PRI_SIZE	0x50

void send_flash_cmd(uint32_t base, uint32_t addr, uint8_t cmd);
void send_flash_reset(uint32_t base);
uint16_t read_cfi(uint32_t base, uint32_t reg);
void detect_flash(uint32_t base);

void detect_flash(uint32_t base) {
	uint16_t Q = read_cfi(base, CFI_ADDR);
	uint16_t R = read_cfi(base, CFI_ADDR + 1);
	uint16_t Y = read_cfi(base, CFI_ADDR + 2);
	
	if (Q != 0x51 || R != 0x52 || Y != 0x59) {
		printf("CFI not found! Bad QRY: %02X%02X%02X\n\n", Q, R, Y);
		return;
	}
	
	const char *model = (const char *) (base + 0x3E000);
	const char *vendor = (const char *) (base + 0x3E010);
	
	if (vendor[0] == 'S' && vendor[1] == 'I' && vendor[2] == 'E')
		printf("Phone: %s %s\n", vendor, model);
	
	uint16_t vid = read_cfi(base, 0);
	uint16_t pid = read_cfi(base, 1);
	
	printf("Base: %08X\n", base);
	printf("Flash ID: %04X:%04X\n", vid, pid);
	
	// Dump all address space
	printf("CFI/PRI/OTP DUMP:\n");
	for (uint32_t i = 0; i < 1024; i++) {
		uint16_t v = read_cfi(base, i);
		if (i && (i % 64) == 0) {
			printf("\n");
		}
		printf("%02X", v & 0xFF);
	}
	printf("\n");
	
}

int main(void) {
	wdt_init();
	
	for (int j = 0; j < 4; j++) {
		uint32_t base = 0xA0000000 + 0x10000000 * j;
		
		EBU_ADDRSEL(j) = EBU_ADDRSEL_REGENAB | (0x01 << EBU_ADDRSEL_MASK_SHIFT) | ((base >> 12) << EBU_ADDRSEL_BASE_SHIFT);
		EBU_BUSCON(j) = EBU_BUSCON_AALIGN | (0x01 << EBU_BUSCON_CTYPE_SHIFT) | (0x01 << EBU_BUSCON_CMULT_SHIFT) | EBU_BUSCON_DLOAD |
			(0x01 << EBU_BUSCON_BCGEN_SHIFT) | (0x01 << EBU_BUSCON_PORTW_SHIFT);
		
		printf("Flash on CS%d\n", j);
		detect_flash(base);
		printf("\n");
		
		wdt_serve();
		
		EBU_ADDRSEL(j) = 0;
		EBU_BUSCON(j) = 0;
	}
	
	while (1);
}

void send_flash_cmd(uint32_t base, uint32_t addr, uint8_t cmd) {
	REG_SHORT(base + 0xAAA) = 0xAA;
	REG_SHORT(base + 0x554) = 0x55;
	REG_SHORT(addr) = cmd;
}

void send_flash_reset(uint32_t base) {
	REG_SHORT(base + 0xAAA) = 0xFF;
}

uint16_t read_cfi(uint32_t base, uint32_t reg) {
	uint16_t v;
	send_flash_cmd(base, base + 0xAAA, 0x90);
	v = REG_SHORT(base + reg * 2);
	send_flash_reset(base);
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
