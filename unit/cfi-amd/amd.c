#include <pmb887x.h>

#include "amd.h"
#include "test.h"

#define PRI_MAJOR_VERSION 0x03
#define PRI_MINOR_VERSION 0x04
#define PRI_SILICON_REVISION 0x05
#define PRI_BANK_ORGANIZATION 0x17
#define PRI_ADDRESS_SENSITIVE_UNLOCK_MASK (BIT(1) | BIT(0))
#define PRI_BANK_ORGANIZATION_MINOR_VERSION '4'
#define CFI_WORD_PROGRAM_TYPICAL_US 0x1F
#define CFI_BUFFER_PROGRAM_TYPICAL_US 0x20
#define CFI_BLOCK_ERASE_TYPICAL_MS 0x21
#define CFI_WORD_PROGRAM_MAX_FACTOR 0x23
#define CFI_BUFFER_PROGRAM_MAX_FACTOR 0x24
#define CFI_BLOCK_ERASE_MAX_FACTOR 0x25

void amd_reset(uint32_t address) {
	MMIO16(address) = AMD_CMD_RESET;
}

void amd_unlock(uint32_t bank) {
	MMIO16(bank + AMD_UNLOCK1_ADDRESS * 2) = AMD_CMD_UNLOCK1;
	MMIO16(bank + AMD_UNLOCK2_ADDRESS * 2) = AMD_CMD_UNLOCK2;
}

void amd_command(uint32_t bank, uint16_t command) {
	amd_unlock(bank);
	MMIO16(bank + AMD_UNLOCK1_ADDRESS * 2) = command;
}

void amd_exit_command_set(uint32_t bank) {
	MMIO16(bank) = AMD_CMD_COMMAND_SET_EXIT;
	MMIO16(bank) = AMD_CMD_COMMAND_SET_EXIT_CONFIRM;
}

static void amd_read_pri(struct amd_flash *flash) {
	uint32_t pri = flash->cfi.primary_address;
	flash->pri_major = cfi_read_word(flash->base, pri + PRI_MAJOR_VERSION);
	flash->pri_minor = cfi_read_word(flash->base, pri + PRI_MINOR_VERSION);
	uint16_t silicon_revision = cfi_read_word(flash->base, pri + PRI_SILICON_REVISION);
	flash->address_sensitive_unlock = (silicon_revision & PRI_ADDRESS_SENSITIVE_UNLOCK_MASK) == 0;

	flash->banks = 1;
	flash->bank_sectors[0] = amd_sector_count(flash);
	if (flash->pri_major != '1' || flash->pri_minor < PRI_BANK_ORGANIZATION_MINOR_VERSION)
		return;

	uint32_t banks = cfi_read_word(flash->base, pri + PRI_BANK_ORGANIZATION);
	if (banks == 0 || banks > AMD_MAX_BANKS)
		return;
	flash->banks = banks;
	for (uint32_t bank = 0; bank < banks; bank++)
		flash->bank_sectors[bank] = cfi_read_word(flash->base, pri + PRI_BANK_ORGANIZATION + 1 + bank);
}

static uint32_t read_max_timeout(uint32_t base, uint32_t typical_offset, uint32_t max_factor_offset) {
	uint32_t exponent = cfi_read_word(base, typical_offset) + cfi_read_word(base, max_factor_offset);
	return exponent < 31 ? 1u << exponent : 0;
}

static void amd_read_timeouts(struct amd_flash *flash) {
	uint32_t word_us = read_max_timeout(flash->base, CFI_WORD_PROGRAM_TYPICAL_US, CFI_WORD_PROGRAM_MAX_FACTOR);
	uint32_t buffer_us = read_max_timeout(flash->base, CFI_BUFFER_PROGRAM_TYPICAL_US, CFI_BUFFER_PROGRAM_MAX_FACTOR);
	uint32_t program_us = buffer_us > word_us ? buffer_us : word_us;
	flash->program_timeout_ms = program_us / 1000 + 1;
	flash->erase_timeout_ms = read_max_timeout(flash->base, CFI_BLOCK_ERASE_TYPICAL_MS, CFI_BLOCK_ERASE_MAX_FACTOR);
}

bool amd_probe(uint32_t cs, struct amd_flash *flash) {
	cfi_disable_chip_selects();
	cfi_map(cs, FLASH_WINDOW_BASE, 0);
	amd_reset(FLASH_WINDOW_BASE);
	cfi_enter_query(FLASH_WINDOW_BASE);
	struct cfi_geometry geometry = { 0 };
	bool has_cfi = cfi_has_query_signature(FLASH_WINDOW_BASE);
	if (has_cfi)
		cfi_read_geometry(FLASH_WINDOW_BASE, &geometry);
	bool usable = (
		has_cfi &&
		geometry.command_set == P_ID_AMD_STD &&
		geometry.size >= 1u << 20 &&
		geometry.size <= FLASH_WINDOW_END - FLASH_WINDOW_BASE &&
		geometry.regions > 0 &&
		geometry.regions <= CFI_MAX_ERASE_REGIONS
	);
	if (!usable) {
		if (has_cfi)
			printf("# CS%lu: command set %04lX is not usable as AMD flash, skipped\n", cs, geometry.command_set);
		amd_reset(FLASH_WINDOW_BASE);
		EBU_ADDRSEL(cs) = 0;
		EBU_BUSCON(cs) = 0;
		return false;
	}

	*flash = (struct amd_flash) { 0 };
	flash->cs = cs;
	flash->base = FLASH_WINDOW_BASE;
	flash->cfi = geometry;
	amd_read_pri(flash);
	amd_read_timeouts(flash);
	amd_reset(flash->base);

	flash->manufacturer = amd_read_autoselect(flash->base, AMD_AUTOSELECT_MANUFACTURER);
	flash->device[0] = amd_read_autoselect(flash->base, AMD_AUTOSELECT_DEVICE1);
	flash->device[1] = amd_read_autoselect(flash->base, AMD_AUTOSELECT_DEVICE2);
	flash->device[2] = amd_read_autoselect(flash->base, AMD_AUTOSELECT_DEVICE3);
	return true;
}

uint32_t amd_sector_count(const struct amd_flash *flash) {
	uint32_t sectors = 0;
	for (uint32_t region = 0; region < flash->cfi.regions; region++)
		sectors += flash->cfi.erase[region].blocks;
	return sectors;
}

uint32_t amd_bank_base(const struct amd_flash *flash, uint32_t bank) {
	uint32_t sectors_before = 0;
	for (uint32_t i = 0; i < bank; i++)
		sectors_before += flash->bank_sectors[i];

	uint32_t offset = 0;
	for (uint32_t region = 0; region < flash->cfi.regions && sectors_before > 0; region++) {
		const struct erase_region *erase = &flash->cfi.erase[region];
		uint32_t blocks = sectors_before < erase->blocks ? sectors_before : erase->blocks;
		offset += blocks * erase->block_size;
		sectors_before -= blocks;
	}
	return flash->base + offset;
}

uint16_t amd_read_autoselect(uint32_t bank, enum amd_autoselect_word word) {
	amd_command(bank, AMD_CMD_AUTOSELECT);
	uint16_t value = cfi_read_word(bank, word);
	amd_reset(bank);
	return value;
}

uint32_t amd_bank_of(const struct amd_flash *flash, uint32_t address) {
	uint32_t bank = 0;
	while (bank + 1 < flash->banks && amd_bank_base(flash, bank + 1) <= address)
		bank++;
	return amd_bank_base(flash, bank);
}

bool amd_find_blank_sector(const struct amd_flash *flash, uint32_t *sector, uint32_t *size) {
	uint32_t first_bank_end = flash->banks > 1 ? amd_bank_base(flash, 1) : flash->base;
	uint32_t address = flash->base;
	for (uint32_t region = 0; region < flash->cfi.regions; region++) {
		const struct erase_region *erase = &flash->cfi.erase[region];
		for (uint32_t block = 0; block < erase->blocks; block++) {
			test_watchdog_reset();
			bool has_neighbors = (
				address > first_bank_end &&
				address + erase->block_size < flash->base + flash->cfi.size
			);
			if (has_neighbors && cfi_range_is_blank(address, erase->block_size)) {
				*sector = address;
				*size = erase->block_size;
				return true;
			}
			address += erase->block_size;
		}
	}
	return false;
}

enum amd_operation_result amd_wait_ready(uint32_t address, uint32_t timeout_ms, uint32_t *toggles) {
	stopwatch_t started = stopwatch_get();
	uint16_t previous = MMIO16(address);
	*toggles = 0;
	while (true) {
		uint16_t status = MMIO16(address);
		if (((status ^ previous) & AMD_STATUS_TOGGLE) == 0)
			return AMD_OPERATION_DONE;
		(*toggles)++;
		if (status & AMD_STATUS_EXCEEDED_TIMING_LIMITS) {
			previous = MMIO16(address);
			status = MMIO16(address);
			bool toggling = ((status ^ previous) & AMD_STATUS_TOGGLE) != 0;
			return toggling ? AMD_OPERATION_FAILED : AMD_OPERATION_DONE;
		}
		if (stopwatch_elapsed_ms(started) > timeout_ms)
			return AMD_OPERATION_TIMEOUT;
		previous = status;
		test_watchdog_serve();
	}
}

uint16_t amd_read_dyb(const struct amd_flash *flash, uint32_t sector) {
	uint32_t bank = amd_bank_of(flash, sector);
	amd_command(bank, AMD_CMD_DYB_ENTRY);
	uint16_t status = MMIO16(sector);
	amd_exit_command_set(bank);
	return status;
}

void amd_write_dyb(const struct amd_flash *flash, uint32_t sector, enum amd_command command) {
	uint32_t bank = amd_bank_of(flash, sector);
	amd_command(bank, AMD_CMD_DYB_ENTRY);
	MMIO16(bank) = AMD_CMD_PROGRAM;
	MMIO16(sector) = command;
	amd_exit_command_set(bank);
}

void amd_start_program_word(const struct amd_flash *flash, uint32_t address, uint16_t value) {
	amd_command(amd_bank_of(flash, address), AMD_CMD_PROGRAM);
	MMIO16(address) = value;
}

void amd_start_program_buffer(const struct amd_flash *flash, uint32_t sector, uint32_t address,
	const uint16_t *data, uint32_t words) {
	amd_unlock(amd_bank_of(flash, sector));
	MMIO16(address) = AMD_CMD_WRITE_TO_BUFFER;
	MMIO16(address) = words - 1;
	for (uint32_t word = 0; word < words; word++)
		MMIO16(address + word * 2) = data[word];
	MMIO16(address + (words - 1) * 2) = AMD_CMD_PROGRAM_BUFFER_TO_FLASH;
}

void amd_start_erase_sector(const struct amd_flash *flash, uint32_t sector) {
	uint32_t bank = amd_bank_of(flash, sector);
	amd_command(bank, AMD_CMD_ERASE_SETUP);
	amd_unlock(bank);
	MMIO16(sector) = AMD_CMD_SECTOR_ERASE;
}
