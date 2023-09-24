#include <pmb887x.h>
#include <printf.h>

void send_flash_cmd(uint32_t addr, uint8_t cmd);
void send_flash_reset(void);
uint16_t read_cfi(uint32_t reg);

uint32_t base_flash = 0;

void dump_flash(void) {
	printf("BASE %08X\n", base_flash);
	printf("========= CFI =========\n");
	
	printf("flash ID: %04X:%04X\n", read_cfi(0), read_cfi(1));
	
	uint32_t total_size = 0;
	uint16_t erase_regions = read_cfi(0x2C);
	
	for (uint16_t i = 0; i < erase_regions; i++) {
		uint32_t blocks = ((read_cfi(0x2E + i * 4) << 8) | read_cfi(0x2D + i * 4)) + 1;
		uint32_t sector_len = (read_cfi(0x30 + i * 4) << 16) | (read_cfi(0x2F + i * 4) << 8);
		
		printf("region %d\n", i);
		printf("  blocks: %d\n", blocks);
		printf("  sector: 0x%X\n", sector_len);
		
		total_size += sector_len * blocks;
	}
	
	printf("flash size (CFI regions): 0x%X [%dM]\n", total_size, total_size / 1024 / 1024);
	
	uint32_t pri_addr = ((read_cfi(0x16)) << 8) | read_cfi(0x15);
	
	if (read_cfi(0) == 0x0089) {
		printf("========= Intel PRI info =========\n");
		
		uint32_t otp0_addr = ((read_cfi(pri_addr + 0x10)) << 8) | read_cfi(pri_addr + 0x0F);
		uint32_t otp0_size_f = 1 << read_cfi(pri_addr + 0x11);
		uint32_t otp0_size_u = 1 << read_cfi(pri_addr + 0x12);
		
		uint32_t otp1_addr = ((read_cfi(pri_addr + 0x16)) << 8) | ((read_cfi(pri_addr + 0x15)) << 8) | ((read_cfi(pri_addr + 0x14)) << 8) | read_cfi(pri_addr + 0x13);
		uint32_t otp1_groups_f = ((read_cfi(pri_addr + 0x18)) << 8) | read_cfi(pri_addr + 0x17);
		uint32_t otp1_groups_u = ((read_cfi(pri_addr + 0x1B)) << 8) | read_cfi(pri_addr + 0x1A);
		uint32_t otp1_size_f = 1 << read_cfi(pri_addr + 0x19);
		uint32_t otp1_size_u = 1 << read_cfi(pri_addr + 0x1C);
		
		printf("PRI addr: 0x%X\n", pri_addr);
		printf("OTP0 addr: 0x%X [%db factory / %db user]\n", otp0_addr, otp0_size_f, otp0_size_u);
		printf("OTP0 size: %d / %d\n", otp0_size_f, otp0_size_u);
		
		printf("OTP1 addr: 0x%X\n", otp1_addr);
		printf("OTP1 groups: %d / %d\n", otp1_groups_f, otp1_groups_u);
		printf("OTP1 group size: %d / %d\n", otp1_size_f, otp1_size_u);
		printf("OTP1 size: %d / %d\n", otp1_groups_f * otp1_size_f, otp1_groups_u * otp1_size_u);
		
		uint32_t hw_regions = read_cfi(pri_addr + 0x23);
		printf("hw regions: %d\n", hw_regions);
		
		uint32_t total_size2 = 0;
		uint32_t pri_regions_off = 0;
		for (uint16_t i = 0; i < hw_regions; i++) {
			uint32_t hw_addr = pri_addr + pri_regions_off;
			
			uint16_t identical_cnt = (read_cfi(hw_addr + 0x25) << 8) | read_cfi(hw_addr + 0x24);
			printf("  identical parts: %d\n", identical_cnt);
			
			uint32_t erase_types = read_cfi(hw_addr + 0x29);
			printf("  erase types: %d\n", erase_types);
			
			uint32_t part_size = 0;
			for (uint16_t j = 0; j < erase_types; j++) {
				uint32_t blocks = ((read_cfi(hw_addr + 0x2B + j * 8) << 8) | read_cfi(hw_addr + 0x2A + j * 8)) + 1;
				uint32_t sector_len = (read_cfi(hw_addr + 0x2D + j * 8) << 16) | (read_cfi(hw_addr + 0x2C + j * 8) << 8);
				
				printf("  erase type %d\n", j);
				printf("    blocks: %d\n", blocks);
				printf("    sector: 0x%X\n", sector_len);
				
				part_size += sector_len * blocks;
			}
			
			printf("  part size: 0x%0X\n", part_size);
			printf("  region size: 0x%0X\n", part_size * identical_cnt);
			
			total_size2 += part_size * identical_cnt;
			
			pri_regions_off += 0x6 + 0x8 * erase_types;
			
			printf("\n");
		}
		
		printf("flash size (PRI hw regions): 0x%X [%dM]\n", total_size2, total_size2 / 1024 / 1024);
	} else if (read_cfi(0) == 0x0020) {
		printf("========= ST PRI info =========\n");
		
		uint32_t otp0_addr = ((read_cfi(pri_addr + 0x10)) << 8) | read_cfi(pri_addr + 0x0F);
		uint32_t otp0_size_f = 1 << read_cfi(pri_addr + 0x11);
		uint32_t otp0_size_u = 1 << read_cfi(pri_addr + 0x12);
		
		uint32_t otp1_addr = ((read_cfi(pri_addr + 0x16)) << 8) | ((read_cfi(pri_addr + 0x15)) << 8) | ((read_cfi(pri_addr + 0x14)) << 8) | read_cfi(pri_addr + 0x13);
		uint32_t otp1_groups_f = ((read_cfi(pri_addr + 0x18)) << 8) | read_cfi(pri_addr + 0x17);
		uint32_t otp1_groups_u = ((read_cfi(pri_addr + 0x1B)) << 8) | read_cfi(pri_addr + 0x1A);
		uint32_t otp1_size_f = 1 << read_cfi(pri_addr + 0x19);
		uint32_t otp1_size_u = 1 << read_cfi(pri_addr + 0x1C);
		
		printf("PRI addr: 0x%X\n", pri_addr);
		printf("OTP0 addr: 0x%X [%db factory / %db user]\n", otp0_addr, otp0_size_f, otp0_size_u);
		printf("OTP0 size: %d / %d\n", otp0_size_f, otp0_size_u);
		
		printf("OTP1 addr: 0x%X\n", otp1_addr);
		printf("OTP1 groups: %d / %d\n", otp1_groups_f, otp1_groups_u);
		printf("OTP1 group size: %d / %d\n", otp1_size_f, otp1_size_u);
		printf("OTP1 size: %d / %d\n", otp1_groups_f * otp1_size_f, otp1_groups_u * otp1_size_u);
		
		uint32_t hw_regions = read_cfi(pri_addr + 0x22);
		printf("hw regions: %d\n", hw_regions);
		
		uint32_t total_size2 = 0;
		uint32_t pri_regions_off = 0;
		for (uint16_t i = 0; i < hw_regions; i++) {
			uint32_t hw_addr = pri_addr + pri_regions_off;
			
			uint16_t self_size = (read_cfi(hw_addr + 0x24) << 8) | read_cfi(hw_addr + 0x23);
			
			printf("  info section size: 0x%02X\n", self_size);
			
			printf("  addr: %02X\n", hw_addr + 0x24 - pri_addr);
			
			uint16_t identical_cnt = (read_cfi(hw_addr + 0x26) << 8) | read_cfi(hw_addr + 0x25);
			printf("  identical parts: %d\n", identical_cnt);
			
			uint32_t erase_types = read_cfi(hw_addr + 0x2A);
			printf("  erase types: %d\n", erase_types);
			
			uint32_t part_size = 0;
			for (uint16_t j = 0; j < erase_types; j++) {
				uint32_t blocks = ((read_cfi(hw_addr + 0x2C + j * 8) << 8) | read_cfi(hw_addr + 0x2B + j * 8)) + 1;
				uint32_t sector_len = (read_cfi(hw_addr + 0x2E + j * 8) << 16) | (read_cfi(hw_addr + 0x2D + j * 8) << 8);
				
				printf("  erase type %d\n", j);
				printf("    blocks: %d\n", blocks);
				printf("    sector: 0x%X\n", sector_len);
				
				part_size += sector_len * blocks;
			}
			
			printf("  part size: 0x%0X\n", part_size);
			printf("  region size: 0x%0X\n", part_size * identical_cnt);
			
			total_size2 += part_size * identical_cnt;
			
			pri_regions_off += self_size;
			
			printf("\n");
		}
		
		printf("flash size (PRI hw regions): 0x%X [%dM]\n", total_size2, total_size2 / 1024 / 1024);
	} else {
		printf("ERROR: unknown vendor\n");
	}
	
	printf("========= DUMP =========\n");
	
	// ESN
	printf("ESN: ");
	for (int i = 0x81; i <= 0x84; i++) {
		uint16_t v = read_cfi(i);
		
		uint8_t hi = v & 0xFF;
		uint8_t lo = (v >> 8) & 0xFF;
		
		printf("%02X%02X", hi, lo);
	}
	printf("\n");
	
	// IMEI
	printf("IMEI: ");
	for (int i = 0x8A; i <= 0x8D; i++) {
		uint16_t v = read_cfi(i);
		
		uint8_t hi = v & 0xFF;
		uint8_t lo = (v >> 8) & 0xFF;
		
		printf("%02X%02X", hi, lo);
	}
	printf("\n");
	
	// ESN
	printf("OTP0: ");
	for (int i = 0x80; i < 0x89; i++) {
		uint16_t v = read_cfi(i);
		
		uint8_t hi = v & 0xFF;
		uint8_t lo = (v >> 8) & 0xFF;
		
		printf("%02X%02X", hi, lo);
	}
	printf("\n");
	
	// IMEI
	printf("IMEI: ");
	for (int i = 0x8A; i <= 0x8D; i++) {
		uint16_t v = read_cfi(i);
		
		uint8_t hi = v & 0xFF;
		uint8_t lo = (v >> 8) & 0xFF;
		
		printf("%02X%02X", hi, lo);
	}
	printf("\n");
	
	// TEST
	printf("TEST:\n");
	for (int i = 0x102; i <= 0x109; i++) {
		send_flash_cmd(base_flash + 0xAAA, 0x90);
		printf("%08X: %02X\n", i, REG_BYTE(base_flash + i));
		send_flash_reset();
	}
	printf("\n");

	// UNK
	printf("UNK:\n");
	for (int i = 0; i <= 0x10; i++) {
		uint16_t v = read_cfi(i);
		printf("0x%02X, ", v & 0xFF);
	}
	printf("\n");

	// CFI
	printf("CFI:\n");
	for (int i = 0x00; i < 0x32; i++) {
		uint16_t v = read_cfi(i);
		if (i < 0x10)
			v = 0xFF;
		printf("0x%02X, ", v & 0xFF);
	}
	printf("\n");
	
	// PRI
	printf("PRI at %04X\n", pri_addr);
	for (uint32_t i = pri_addr; i < pri_addr + 0x50; i++) {
		uint16_t v = read_cfi(i);
		printf("0x%02X, ", v & 0xFF);
	}
	printf("\n");
	
	printf("\n\n\n\n\n\n\n\n\n\n\n\n");
}

int main(void) {
	wdt_init();
	
	// Инифицализируем флеш
	EBU_ADDRSEL(0) = 0xA0000011;
	EBU_ADDRSEL(4) = 0xA0000011;
	EBU_BUSCON(0) = 0x00522600;
	EBU_BUSCON(4) = 0x00522600;
	
	/*
	// SL75
	EBU_ADDRSEL(0) = 0xA0000021;
	EBU_ADDRSEL(2) = 0xA4000021;
	EBU_ADDRSEL(3) = 0xA6000021;
	EBU_ADDRSEL(5) = 0xA8000030;
	EBU_ADDRSEL(6) = 0xA4000020;
	EBU_ADDRSEL(4) = 0xA0000021;
	
	EBU_BUSCON(0) = 0xA2520E00;
	EBU_BUSCON(2) = 0x00522601;
	EBU_BUSCON(3) = 0x00522601;
	EBU_BUSCON(5) = 0x80520600;
	EBU_BUSCON(6) = 0x30420200;
	EBU_BUSCON(4) = 0x80520637;
	*/
	
	base_flash = 0xA0000000;
	dump_flash();
	
	base_flash = 0xA4000000;
	dump_flash();
	
	base_flash = 0xA6000000;
	dump_flash();
	
	while (1);
}

void send_flash_cmd(uint32_t addr, uint8_t cmd) {
	REG_SHORT(base_flash + 0xAAA) = 0xAA;
	REG_SHORT(base_flash + 0x554) = 0x55;
	REG_SHORT(addr) = cmd;
}

void send_flash_reset(void) {
	REG_SHORT(base_flash + 0xAAA) = 0xFF;
}

uint16_t read_cfi(uint32_t reg) {
	uint16_t v;
	send_flash_cmd(base_flash + 0xAAA, 0x90);
	v = REG_SHORT(base_flash + reg * 2);
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
