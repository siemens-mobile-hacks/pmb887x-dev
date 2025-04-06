#include <pmb887x.h>
#include <printf.h>
#include <stdint.h>

#ifdef BOOT_CUSTOM
#undef printf
#define printf(...)
#endif

enum {
	RESP_DONE					= 0,
	RESP_BOOT_ALREADY_OPEN		= 1,
	RESP_UNKNOWN_FLASH			= 2,
	RESP_FLASH_BUSY				= 3,
	RESP_ERASE_ERROR			= 4,
	RESP_PROGRAM_ERROR			= 5,
};

typedef struct FlashEraseRegion FlashEraseRegion;

struct FlashEraseRegion {
	uint32_t addr;
	uint32_t size;
	uint32_t blocks_count;
	uint32_t erase_size;
};

static void send_flash_cmd(uint8_t cmd, int unlock);
static void send_flash_reset(void);
static int detect_flash(void);
static int check_qry(void);
static int probe_flash(void);
static uint16_t read_reg(uint32_t cmd, uint32_t reg, int unlock);
static void unlock_flash_access(void);
static void lock_flash_access(void);
static volatile uint32_t *get_mmu_table(void);
static uint32_t get_domain_access(void);
static uint32_t set_domain_access(volatile uint32_t domains);

static int flash_program(uint32_t addr);
static int flash_erase(uint32_t addr);
static FlashEraseRegion *find_flash_region(uint32_t addr);
static int read_flash_regions(void);

#define CHECK_CRC					0

#define CFI_ADDR					0x10

#define FLASH_BASE					0xA0000000
#define PATCHER_ADDR				(0xB0FC0000)
#define TCM_START					(0xFFFF0000)
#define TCM_END						(0xFFFF0000 + 16 * 1024)
#define PRAM_IRQ_HANDLER			(TCM_START + 0x38)
#define BOOT_CONFIG_ADDR 			(0xA000000C)

#define PATCHER_END					(PATCHER_ADDR + 1024 * 2)
#define PARAM_OLD_IRQ_HANDLER		(PATCHER_END - 4)
#define PARAM_RESPONSE_CODE			(PATCHER_END - 8)
#define PARAM_RESPONSE_FLASH_ID		(PATCHER_END - 12)

#define CFI_FLASH_SIZE				(CFI_ADDR + 23)
#define CFI_ERASE_REGIONS_CNT		(CFI_ADDR + 28)
#define CFI_ERASE_REGIONS			(CFI_ADDR + 29)

#define FLASH_CMD_CFI				0x98
#define FLASH_CMD_READ_ID			0x90
#define FLASH_CMD_RESET1			0xF0 // AMD
#define FLASH_CMD_RESET2			0xFF // AMD or Intel/ST
#define FLASH_CMD_AMD_UNLOCK1		0xAA
#define FLASH_CMD_AMD_UNLOCK2		0x55
#define FLASH_CMD_READ_STATUS		0x70

#define FLASH_CMD_PROTECT			0x60
#define FLASH_CMD_PROTECT_SET		0x01
#define FLASH_CMD_PROTECT_CLEAR		0xD0

#define FLASH_CMD_ERASE				0x20
#define FLASH_CMD_ERASE_CONFIRM		0xD0

#define FLASH_CMD_WRITE				0x40

#define FLASH_STATUS_DONE			0x80

static uint16_t flash_vid;
static uint16_t flash_pid;
static uint32_t flash_unlock_addr1;
static uint32_t flash_unlock_addr2;
static uint32_t old_domain_access;
static uint32_t old_flash_mmu;
static uint32_t old_ebu_buscon;
static FlashEraseRegion erase_regions[8];
static uint32_t erase_regions_cnt;
static uint32_t max_erase_size;
static uint8_t write_buffer[128 * 1024];

#ifndef BOOT_CUSTOM
// FOR TEST ON EMULATOR
int main() {
	wdt_init();

	printf("MAGIC: %08X\n", MMIO32(0xA000003C));

	if (read_flash_regions() != 0) {
		printf("ERROR: Bad flash!\n");
		return 0;
	}

	if (flash_erase(0xA0000000) < 0) {
		printf("ERROR: Can't erase flash block!\n");
		return 0;
	}

	write_buffer[0x3C] = 0xFF;
	write_buffer[0x3C + 1] = 0xFF;
	write_buffer[0x3C + 2] = 0xFF;
	write_buffer[0x3C + 3] = 0xFF;

	if (flash_program(0xA0000000) < 0) {
		printf("ERROR: Can't write flash block!\n");
		return 0;
	}

	printf("MAGIC: %08X\n", MMIO32(0xA000003C));

	return 0;
}
#endif

#ifdef BOOT_CUSTOM
// FOR REAL DEVICE
__attribute__((interrupt("irq"), used))
void _start(void) {
	MMIO32(PRAM_IRQ_HANDLER) = MMIO32(PARAM_OLD_IRQ_HANDLER);
	MMIO32(PARAM_RESPONSE_CODE) = 0xFFFFFFFF;
	MMIO32(PARAM_RESPONSE_FLASH_ID) = 0xFFFFFFFF;

	if (MMIO32(BOOT_CONFIG_ADDR) == 0xFFFFFFFF) {
		MMIO32(PARAM_RESPONSE_CODE) = RESP_BOOT_ALREADY_OPEN;
		return;
	}

	unlock_flash_access();

	if (read_flash_regions() != 0) {
		MMIO32(PARAM_RESPONSE_FLASH_ID) = (flash_vid << 16) | flash_pid;
		MMIO32(PARAM_RESPONSE_CODE) = RESP_UNKNOWN_FLASH;
		lock_flash_access();
		return;
	}

	MMIO32(PARAM_RESPONSE_FLASH_ID) = (flash_vid << 16) | flash_pid;

	if (flash_erase(0xA0000000) < 0) {
		MMIO32(PARAM_RESPONSE_CODE) = RESP_ERASE_ERROR;
		lock_flash_access();
		return;
	}

	// PATCH: A000000C -> FFFFFFFF (open bootcore)
	write_buffer[0x0C] = 0xFF;
	write_buffer[0x0C + 1] = 0xFF;
	write_buffer[0x0C + 2] = 0xFF;
	write_buffer[0x0C + 3] = 0xFF;

	if (flash_program(0xA0000000) < 0) {
		MMIO32(PARAM_RESPONSE_CODE) = RESP_PROGRAM_ERROR;
		lock_flash_access();
		return;
	}

	MMIO32(PARAM_RESPONSE_CODE) = RESP_DONE;

	lock_flash_access();
}

#endif

static int read_flash_regions(void) {
	flash_pid = 0xFFFF;
	flash_vid = 0xFFFF;

	if (probe_flash() == 0) {
		printf("ERROR: Flash not found!\n");
		return 0;
	}

	flash_vid = read_reg(FLASH_CMD_READ_ID, 0, 1);
	flash_pid = read_reg(FLASH_CMD_READ_ID, 0, 1);

	return -1;

	printf("Found flash: %04X:%04X\n", flash_vid, flash_pid);

	if (flash_vid != 0x89 && flash_vid != 0x20) { // thanks Viktor89
		printf("ERROR: unsupported flash\n");
		return 0;
	}

	max_erase_size = 0;

	uint32_t flash_size = 1 << read_reg(FLASH_CMD_CFI, CFI_FLASH_SIZE, 0);
	printf("Flash size: %dM\n", flash_size / 1024 / 1024);

	erase_regions_cnt = read_reg(FLASH_CMD_CFI, CFI_ERASE_REGIONS_CNT, 0);
	printf("Erase regions: %d\n", erase_regions_cnt);

	if (erase_regions_cnt > ARRAY_SIZE(erase_regions)) {
		printf("Invalid flash regions count: %d\n", erase_regions_cnt);
		return -1;
	}

	uint32_t cursor = FLASH_BASE;
	uint32_t check_size = 0;
	for (int i = 0; i < erase_regions_cnt; i++) {
		uint32_t region_offset = CFI_ERASE_REGIONS + i * 4;
		uint32_t blocks_count = (read_reg(FLASH_CMD_CFI, region_offset, 0) | (read_reg(FLASH_CMD_CFI, region_offset + 1, 0) << 8)) + 1;
		uint32_t erase_size = (read_reg(FLASH_CMD_CFI, region_offset + 2, 0) | (read_reg(FLASH_CMD_CFI, region_offset + 3, 0) << 8)) * 256;

		erase_regions[i].addr = cursor;
		erase_regions[i].size = blocks_count * erase_size;
		erase_regions[i].blocks_count = blocks_count;
		erase_regions[i].erase_size = erase_size;

		check_size += blocks_count * erase_size;
		cursor += blocks_count * erase_size;

		if (max_erase_size < erase_size)
			max_erase_size = erase_size;
	}

	if (flash_size != check_size) {
		printf("ERROR: Bad flash regions!\n");
		return -1;
	}

	for (int i = 0; i < erase_regions_cnt; i++) {
		FlashEraseRegion *region = &erase_regions[i];
		printf("REGION #%d: %08X-%08X [%d x %dk]\n", i, region->addr, region->addr + region->size - 1, region->blocks_count, region->erase_size / 1024);
	}

	printf("Max. erase size: %dk\n", max_erase_size / 1024);

	if (max_erase_size > ARRAY_SIZE(write_buffer)) {
		printf("ERROR: erase region is too big: %dk\n", max_erase_size);
		return -1;
	}

	return 0;
}

static FlashEraseRegion *find_flash_region(uint32_t addr) {
	for (int i = 0; i < erase_regions_cnt; i++) {
		FlashEraseRegion *region = &erase_regions[i];
		if (region->addr >= addr && addr < region->addr + region->size)
			return region;
	}
	return NULL;
}

static int flash_erase(uint32_t addr) {
	printf("flash_erase(%08X)\n", addr);

	FlashEraseRegion *region = find_flash_region(addr);
	if (!region) {
		printf("ERROR: Erase region not found for %08X!\n", addr);
		return -1;
	}

	if ((addr % region->erase_size) != 0) {
		printf("ERROR: Addr %08X is not aligned to %08X!\n", addr, region->erase_size);
		return -1;
	}

	printf("Found region: %08X-%08X\n", region->addr, region->addr + region->size + 1);

	send_flash_reset();

	#if CHECK_CRC
	uint32_t crc = 0;
	for (uint32_t i = 0; i < region->erase_size / 4; i++) {
		crc += MMIO32(addr + i * 4);
	}
	printf("CRC: %08X\n", crc);
	#endif

	for (uint32_t i = 0; i < region->erase_size; i++)
		write_buffer[i] = MMIO8(addr + i);

	// Unlock block
	MMIO16(addr) = FLASH_CMD_PROTECT;
	MMIO16(addr) = FLASH_CMD_PROTECT_CLEAR;

	// Erase
	MMIO16(addr) = FLASH_CMD_ERASE;
	MMIO16(addr) = FLASH_CMD_ERASE_CONFIRM;

	// Wait for erase
	while ((MMIO16(addr) & FLASH_STATUS_DONE) == 0);

	// Lock block
	MMIO16(addr) = FLASH_CMD_PROTECT;
	MMIO16(addr) = FLASH_CMD_PROTECT_SET;

	send_flash_reset();

	printf("Block erased!!!\n");

	return region->size;
}

static int flash_program(uint32_t addr) {
	printf("flash_program(%08X)\n", addr);

	FlashEraseRegion *region = find_flash_region(addr);
	if (!region) {
		printf("ERROR: Erase region not found for %08X!\n", addr);
		return -1;
	}

	printf("Found region: %08X-%08X\n", region->addr, region->addr + region->size + 1);

	// Unlock block
	MMIO16(addr) = FLASH_CMD_PROTECT;
	MMIO16(addr) = FLASH_CMD_PROTECT_CLEAR;

	for (uint32_t i = 0; i < region->erase_size / 2; i++) {
		// Program word
		MMIO16(addr + i * 2) = FLASH_CMD_WRITE;
		MMIO16(addr + i * 2) = (write_buffer[i * 2 + 1] << 8) | write_buffer[i * 2];

		// Wait for done
		MMIO16(addr + i * 2) = FLASH_CMD_READ_STATUS;
		while ((MMIO16(addr + i * 2) & FLASH_STATUS_DONE) == 0);
	}

	// Lock block
	MMIO16(addr) = FLASH_CMD_PROTECT;
	MMIO16(addr) = FLASH_CMD_PROTECT_SET;

	send_flash_reset();

	#if CHECK_CRC
	uint32_t crc = 0;
	for (uint32_t i = 0; i < region->erase_size / 4; i++) {
		crc += MMIO32(addr + i * 4);
	}
	printf("CRC: %08X\n", crc);
	#endif

	printf("Flash write done!!!\n");

	return 0;
}

static volatile uint32_t *get_mmu_table(void) {
	volatile uint32_t *addr;
	__asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r" (addr));
	return addr;
}

static uint32_t get_domain_access(void) {
	volatile uint32_t domains;
	__asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r" (domains));
	return domains;
}

static uint32_t set_domain_access(volatile uint32_t domains) {
	volatile uint32_t old_domains;
	__asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r" (old_domains));
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r" (domains));
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	return old_domains;
}

static void unlock_flash_access(void) {
	volatile uint32_t *mmu_table = get_mmu_table();
	old_domain_access = get_domain_access();
	old_flash_mmu = mmu_table[0xA0000000 >> 20];
	old_ebu_buscon = EBU_BUSCON(0);
	set_domain_access(0xFFFFFFFF);
	mmu_table[0xA0000000 >> 20] = 0xA0000C12;
	EBU_BUSCON(0) &= ~EBU_BUSCON_WRITE;
}

static void lock_flash_access(void) {
	volatile uint32_t *mmu_table = get_mmu_table();
	mmu_table[0xA0000000 >> 20] = old_flash_mmu;
	set_domain_access(old_domain_access);
	EBU_BUSCON(0) = old_ebu_buscon;
}

static int check_qry() {
	send_flash_reset();
	uint16_t Q = read_reg(FLASH_CMD_CFI, CFI_ADDR, 0);
	uint16_t R = read_reg(FLASH_CMD_CFI, CFI_ADDR + 1, 0);
	uint16_t Y = read_reg(FLASH_CMD_CFI, CFI_ADDR + 2, 0);
	return (Q == 0x51 && R == 0x52 && Y == 0x59);
}

static int probe_flash() {
	flash_unlock_addr1 = 0x555 << 1;
	flash_unlock_addr2 = 0x2AA << 1;
	if (check_qry())
		return 1;
	flash_unlock_addr1 = 0x5555 << 1;
	flash_unlock_addr2 = 0x2AAA << 1;
	if (check_qry())
		return 2;
	return 0;
}

static void send_flash_cmd(uint8_t cmd, int unlock) {
	if (unlock) {
		MMIO16(0xA0000000 + flash_unlock_addr1) = FLASH_CMD_AMD_UNLOCK1;
		MMIO16(0xA0000000 + flash_unlock_addr2) = FLASH_CMD_AMD_UNLOCK2;
	}
	MMIO16(0xA0000000 + flash_unlock_addr1) = cmd;
}

static void send_flash_reset(void) {
	MMIO16(0xA0000000) = FLASH_CMD_RESET1;
	MMIO16(0xA0000000) = FLASH_CMD_RESET2;
}

static uint16_t read_reg(uint32_t cmd, uint32_t reg, int unlock) {
	uint16_t v;
	send_flash_cmd(cmd, unlock);
	v = MMIO16(0xA0000000 + (reg << 1));
	send_flash_reset();
	return v;
}
