#include <pmb887x.h>
#include <printf.h>
#include <stdint.h>

#define CFI_ADDR	0x10
#define CFI_SIZE	0x22
#define PRI_SIZE	0x50

volatile uint32_t flash_base = 0;
volatile uint32_t flash_unlock_addr1 = 0;
volatile uint32_t flash_unlock_addr2 = 0;

void send_flash_cmd(uint8_t cmd, int unlock);
void send_flash_reset(void);
int detect_flash(void);
int check_qry(void);
int probe_flash(void);
uint16_t read_reg(uint32_t cmd, uint32_t reg, int unlock);
void memory_hexdump(uint32_t cmd, uint32_t size, int unlock);

void memory_hexdump(uint32_t cmd, uint32_t size, int unlock) {
	for (uint32_t i = 0; i < size; i++) {
		uint16_t v = read_reg(cmd, i, unlock);
		if (i && (i % 32) == 0) {
			printf("\r\n");
		}
		printf("%04X ", v);
	}
	printf("\r\n");
}

int detect_flash(void) {
	if (!probe_flash()) {
		printf("NOR flash not found.\r\n");
		return 0;
	}

	const char *model = (const char *) (flash_base + 0x3E000);
	const char *vendor = (const char *) (flash_base + 0x3E010);
	
	if (vendor[0] == 'S' && vendor[1] == 'I' && vendor[2] == 'E')
		printf("Phone: %s %s\r\n", vendor, model);

	const char *model2 = (const char *) (flash_base + 0x210);
	const char *vendor2 = (const char *) (flash_base + 0x220);

	if (vendor2[0] == 'S' && vendor2[1] == 'I' && vendor2[2] == 'E')
		printf("Phone: %s %s\r\n", vendor2, model2);

	uint16_t vid = read_reg(0x90, 0, 1);
	uint16_t pid = read_reg(0x90, 1, 1);

	if (vid == 0x0001) {
		uint16_t pid2 = read_reg(0x90, 0xE, 1);
		uint16_t pid3 = read_reg(0x90, 0xF, 1);
		printf("Flash ID: %04X:%04X-%04X-%04X\r\n", vid, pid, pid2, pid3);
		printf("AMD unlock1: %04X\r\n", flash_unlock_addr1);
		printf("AMD unlock2: %04X\r\n", flash_unlock_addr2);

		printf("AMD AUTOSELECT:\r\n");
		memory_hexdump(0x90, 256, 1);

		printf("AMD CFI/PRI:\r\n");
		memory_hexdump(0x98, 128, 0);

		printf("AMD SecSi:\r\n");
		memory_hexdump(0x88, 256, 1);
	} else {
		printf("Flash ID: %04X:%04X\n", vid, pid);
		printf("CFI/PRI/OTP:\r\n");
		memory_hexdump(0x90, 1024, 0);
	}

	return 1;
}

int main(void) {
	register uint32_t r12 __asm__("r12");
	uint32_t LR = r12;

	wdt_init();

	printf("LR=%08X\n", LR);
	printf("SCU_CHIPID=%08X\r\n", SCU_CHIPID);
	printf("----\r\n");
	for (int j = 0; j < 4; j++) {
		printf("EBU_ADDRSEL%d: %08X\r\n", j, EBU_ADDRSEL(j));
		printf("EBU_BUSCON%d: %08X\r\n", j, EBU_BUSCON(j));
	}
	printf("----\r\n");

	flash_base = 0xA0000000;
	for (int j = 0; j < 4; j++) {
		EBU_ADDRSEL(j) = EBU_ADDRSEL_REGENAB | (0x01 << EBU_ADDRSEL_MASK_SHIFT) | ((flash_base >> 12) << EBU_ADDRSEL_BASE_SHIFT);
		EBU_BUSCON(j) = EBU_BUSCON_AALIGN | (0x01 << EBU_BUSCON_CTYPE_SHIFT) | (0x01 << EBU_BUSCON_CMULT_SHIFT) | EBU_BUSCON_DLOAD |
			(0x01 << EBU_BUSCON_BCGEN_SHIFT) | (0x01 << EBU_BUSCON_PORTW_SHIFT);

		printf("Flash on CS%d\n", j);
		detect_flash();
		printf("\n");
		
		wdt_serve();
		
		EBU_ADDRSEL(j) = 0;
		EBU_BUSCON(j) = 0;
	}
	
	printf("Done!\n");
	usart_putc(USART0, 0);

	while (1);
}

int check_qry() {
	send_flash_reset();
	uint16_t Q = read_reg(0x98, CFI_ADDR, 0);
	uint16_t R = read_reg(0x98, CFI_ADDR + 1, 0);
	uint16_t Y = read_reg(0x98, CFI_ADDR + 2, 0);
	return (Q == 0x51 && R == 0x52 && Y == 0x59);
}

int probe_flash() {
	flash_unlock_addr1 = 0x555 << 1;
	flash_unlock_addr2 = 0x2AA << 1;
	if (check_qry())
		return 1;
	flash_unlock_addr1 = 0x5555 << 1;
	flash_unlock_addr2 = 0x2AAA << 1;
	if (check_qry())
		return 1;
	return 0;
}

void send_flash_cmd(uint8_t cmd, int unlock) {
	if (unlock) {
		MMIO16(flash_base + flash_unlock_addr1) = 0xAA;
		MMIO16(flash_base + flash_unlock_addr2) = 0x55;
	}
	MMIO16(flash_base + flash_unlock_addr1) = cmd;
}

void send_flash_reset(void) {
	MMIO16(flash_base) = 0xF0;
	MMIO16(flash_base) = 0xFF;
}

uint16_t read_reg(uint32_t cmd, uint32_t reg, int unlock) {
	uint16_t v;
	send_flash_cmd(cmd, unlock);
	v = MMIO16(flash_base + (reg << 1));
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
