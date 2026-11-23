#include <pmb887x.h>

#include <string.h>

#include "amd.h"
#include "test.h"

#define TEST_WORD 0x4BD2

static void write_all_dyb(const struct amd_flash *flash, enum amd_command command) {
	uint32_t address = flash->base;
	for (uint32_t region = 0; region < flash->cfi.regions; region++) {
		const struct erase_region *erase = &flash->cfi.erase[region];
		for (uint32_t block = 0; block < erase->blocks; block++) {
			amd_write_dyb(flash, address, command);
			address += erase->block_size;
		}
	}
}

static void program_word(const struct amd_flash *flash, uint32_t address, uint16_t value) {
	uint32_t toggles;
	amd_start_program_word(flash, address, value);
	amd_wait_ready(address, flash->program_timeout_ms, &toggles);
}

static void test_chip_erase(
	const struct amd_flash *flash,
	uint32_t erased_sector,
	uint32_t erased_size,
	uint32_t protected_sector
) {
	uint32_t bank = amd_bank_of(flash, erased_sector);
	uint16_t first_bank[AMD_SAMPLE_WORDS];
	memcpy(first_bank, (const void *) flash->base, sizeof(first_bank));

	amd_command(flash->base, AMD_CMD_ERASE_SETUP);
	amd_unlock(flash->base);
	MMIO16(flash->base + AMD_UNLOCK1_ADDRESS * 2) = AMD_CMD_CHIP_ERASE;
	uint16_t erasing_first = MMIO16(erased_sector);
	uint16_t erasing_second = MMIO16(erased_sector);
	MMIO16(bank) = AMD_CMD_ERASE_SUSPEND;
	uint16_t suspend_first = MMIO16(erased_sector);
	uint16_t suspend_second = MMIO16(erased_sector);
	uint32_t toggles;
	uint32_t timeout_ms = flash->erase_timeout_ms * amd_sector_count(flash);
	enum amd_operation_result result = amd_wait_ready(erased_sector, timeout_ms, &toggles);
	printf("# chip erase: status %04X %04X, after suspend %04X %04X, %lu toggles\n", erasing_first,
		erasing_second, suspend_first, suspend_second, toggles);

	test_eq_u32("data polling reads DQ7 low during chip erase", 0, erasing_first & AMD_STATUS_DATA_POLLING);
	test_check("DQ6 toggles during chip erase", ((erasing_first ^ erasing_second) & AMD_STATUS_TOGGLE) != 0);
	test_check("erase suspend is ignored during chip erase",
		((suspend_first ^ suspend_second) & AMD_STATUS_TOGGLE) != 0);
	test_eq_u32("chip erase completes", AMD_OPERATION_DONE, result);
	test_check("chip erase blanks the unprotected sector", cfi_range_is_blank(erased_sector, erased_size));
	test_eq_u32("chip erase keeps the DYB-protected sector", TEST_WORD, MMIO16(protected_sector));
	test_eq_memory("chip erase keeps the protected first bank", first_bank, (const void *) flash->base,
		sizeof(first_bank));
}

int main(void) {
	test_start("AMD CFI flash chip erase test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct amd_flash flash;
	if (!amd_find_flash(&flash)) {
		test_skip("AMD CFI flash", "no AMD CFI flash found");
		return test_finish();
	}
	uint32_t erased_sector;
	uint32_t erased_size;
	uint32_t protected_sector;
	uint32_t protected_size;
	bool sectors_found = (
		amd_find_blank_sector(&flash, flash.base, &erased_sector, &erased_size) &&
		amd_find_blank_sector(&flash, erased_sector + erased_size, &protected_sector, &protected_size)
	);
	if (!sectors_found) {
		test_skip("AMD chip erase", "no two blank sectors outside the first bank");
		return test_finish();
	}
	printf("# erased sector %08lX, protected sector %08lX\n", erased_sector, protected_sector);

	write_all_dyb(&flash, AMD_CMD_DYB_SET);
	amd_write_dyb(&flash, erased_sector, AMD_CMD_DYB_CLEAR);
	amd_write_dyb(&flash, protected_sector, AMD_CMD_DYB_CLEAR);
	program_word(&flash, erased_sector, TEST_WORD);
	program_word(&flash, protected_sector, TEST_WORD);
	amd_write_dyb(&flash, protected_sector, AMD_CMD_DYB_SET);

	test_category("Chip erase");
	test_chip_erase(&flash, erased_sector, erased_size, protected_sector);

	uint32_t toggles;
	amd_write_dyb(&flash, protected_sector, AMD_CMD_DYB_CLEAR);
	amd_start_erase_sector(&flash, protected_sector);
	amd_wait_ready(protected_sector, flash.erase_timeout_ms, &toggles);
	test_check("cleanup erase blanks the protected sector", cfi_range_is_blank(protected_sector, protected_size));
	amd_write_dyb(&flash, protected_sector, AMD_CMD_DYB_SET);
	amd_write_dyb(&flash, erased_sector, AMD_CMD_DYB_SET);
	return test_finish();
}
