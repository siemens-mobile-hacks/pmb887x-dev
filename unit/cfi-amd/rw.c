#include <pmb887x.h>

#include <string.h>

#include "amd.h"
#include "test.h"

#define DYB_PROTECTED 0x0000
#define DYB_UNPROTECTED 0x0001
#define TEST_WORD 0x5AA5
#define PROTECTED_TEST_WORD 0x1234
#define ERASE_TEST_OFFSET 0x100

static bool find_flash(struct amd_flash *flash) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		if (amd_probe(cfi_chip_selects[i], flash)) {
			cfi_disable_chip_selects();
			cfi_map(flash->cs, FLASH_WINDOW_BASE, 27 - flash->cfi.size_exponent);
			return true;
		}
	}
	cfi_disable_chip_selects();
	return false;
}

static void test_dyb_protection(const struct amd_flash *flash, uint32_t sector) {
	uint16_t dyb = amd_read_dyb(flash, sector);
	printf("# DYB status at power-up: %04X\n", dyb);
	if (dyb == DYB_PROTECTED) {
		uint32_t toggles;
		amd_start_program_word(flash, sector, PROTECTED_TEST_WORD);
		enum amd_operation_result result = amd_wait_ready(sector, flash->program_timeout_ms, &toggles);
		amd_reset(sector);
		test_eq_u32("program of a DYB-protected sector completes", AMD_OPERATION_DONE, result);
		test_eq_u32("DYB-protected sector keeps erased data", 0xFFFF, MMIO16(sector));
	} else {
		test_skip("DYB-protected program", "sector is unprotected at power-up");
	}

	amd_write_dyb(flash, sector, AMD_CMD_DYB_CLEAR);
	test_eq_u32("DYB clear unprotects the sector", DYB_UNPROTECTED, amd_read_dyb(flash, sector));
}

static void test_program_word(const struct amd_flash *flash, uint32_t sector) {
	uint32_t toggles;
	amd_start_program_word(flash, sector, TEST_WORD);
	uint16_t status = MMIO16(sector);
	enum amd_operation_result result = amd_wait_ready(sector, flash->program_timeout_ms, &toggles);
	printf("# word program: first status %04X, %lu toggles\n", status, toggles);
	test_eq_u32("word program completes", AMD_OPERATION_DONE, result);
	test_eq_u32("data polling shows inverted DQ7 while programming",
		~TEST_WORD & AMD_STATUS_DATA_POLLING, status & AMD_STATUS_DATA_POLLING);
	test_check("DQ6 toggles while programming", toggles > 0);
	test_eq_u32("programmed word reads back", TEST_WORD, MMIO16(sector));
}

static void test_program_buffer(const struct amd_flash *flash, uint32_t sector) {
	uint32_t words = flash->cfi.write_buffer_size / 2;
	uint32_t address = sector + flash->cfi.write_buffer_size;
	uint16_t data[words];
	for (uint32_t word = 0; word < words; word++)
		data[word] = (uint16_t) ((word * 0x0101) ^ 0xC3A5);

	uint32_t toggles;
	amd_start_program_buffer(flash, sector, address, data, words);
	uint32_t last = address + (words - 1) * 2;
	enum amd_operation_result result = amd_wait_ready(last, flash->program_timeout_ms, &toggles);
	printf("# buffer program: %lu words, %lu toggles\n", words, toggles);
	test_eq_u32("buffer program completes", AMD_OPERATION_DONE, result);
	test_check("DQ6 toggles while buffer programming", toggles > 0);
	test_eq_memory("buffer data reads back", data, (const void *) address, sizeof(data));
}

static void test_buffer_abort(const struct amd_flash *flash, uint32_t sector) {
	uint32_t page = sector + 2 * flash->cfi.write_buffer_size;
	uint32_t bank = amd_bank_of(flash, sector);
	amd_unlock(bank);
	MMIO16(sector) = AMD_CMD_WRITE_TO_BUFFER;
	MMIO16(sector) = 0;
	MMIO16(page) = TEST_WORD;
	MMIO16(page) = AMD_CMD_PROGRAM_BUFFER_TO_FLASH;

	uint32_t toggles;
	enum amd_operation_result result = amd_wait_ready(page, flash->program_timeout_ms, &toggles);
	uint16_t status = MMIO16(page);
	printf("# buffer abort: status %04X\n", status);
	test_eq_u32("data outside the page of the buffer load command aborts", AMD_OPERATION_TIMEOUT, result);
	test_check("DQ1 reports the write buffer abort", (status & AMD_STATUS_WRITE_BUFFER_ABORT) != 0);
	amd_reset(page);
	test_check("reset command does not leave the abort state", (MMIO16(page) & AMD_STATUS_WRITE_BUFFER_ABORT) != 0);
	amd_command(bank, AMD_CMD_RESET);
	test_eq_u32("abort reset returns to array without programming", 0xFFFF, MMIO16(page));
}

static void test_erase_sector(const struct amd_flash *flash, uint32_t sector, uint32_t size) {
	uint16_t first_bank[AMD_SAMPLE_WORDS];
	uint16_t previous_sector[AMD_SAMPLE_WORDS];
	memcpy(first_bank, (const void *) flash->base, sizeof(first_bank));
	memcpy(previous_sector, (const void *) (sector - AMD_SAMPLE_WORDS * 2), sizeof(previous_sector));

	amd_start_erase_sector(flash, sector);
	uint16_t erasing_first = MMIO16(sector);
	uint16_t erasing_second = MMIO16(sector);
	test_eq_memory("other bank reads array during erase", first_bank, (const void *) flash->base, sizeof(first_bank));
	uint32_t toggles;
	enum amd_operation_result result = amd_wait_ready(sector, flash->erase_timeout_ms, &toggles);
	printf("# erase: status %04X %04X, %lu toggles\n", erasing_first, erasing_second, toggles);
	test_eq_u32("sector erase completes", AMD_OPERATION_DONE, result);
	test_eq_u32("data polling reads DQ7 low while erasing", 0, erasing_first & AMD_STATUS_DATA_POLLING);
	test_check("DQ2 toggles inside the erasing sector", ((erasing_first ^ erasing_second) & AMD_STATUS_ERASE_TOGGLE) != 0);
	test_check("erased sector is blank", cfi_range_is_blank(sector, size));
	test_eq_memory(
		"previous sector is preserved",
		previous_sector,
		(const void *) (sector - AMD_SAMPLE_WORDS * 2),
		sizeof(previous_sector)
	);
}

static void test_erase_suspend(const struct amd_flash *flash, uint32_t sector, uint32_t size) {
	uint32_t toggles;
	uint32_t bank = amd_bank_of(flash, sector);
	uint32_t other_sector = sector - AMD_SAMPLE_WORDS * 2;
	uint16_t other_data[AMD_SAMPLE_WORDS];
	memcpy(other_data, (const void *) other_sector, sizeof(other_data));
	amd_start_program_word(flash, sector + ERASE_TEST_OFFSET, TEST_WORD);
	amd_wait_ready(sector + ERASE_TEST_OFFSET, flash->program_timeout_ms, &toggles);

	amd_start_erase_sector(flash, sector);
	stopwatch_t started = stopwatch_get();
	bool erase_started = false;
	while (!erase_started && stopwatch_elapsed_ms(started) <= flash->program_timeout_ms)
		erase_started = (MMIO16(sector) & AMD_STATUS_ERASE_STARTED) != 0;
	test_check("DQ3 reports the started erase", erase_started);
	MMIO16(bank) = AMD_CMD_ERASE_SUSPEND;
	enum amd_operation_result suspend = amd_wait_ready(other_sector, flash->program_timeout_ms, &toggles);
	test_eq_u32("erase suspend stops DQ6", AMD_OPERATION_DONE, suspend);

	uint16_t suspended_first = MMIO16(sector);
	uint16_t suspended_second = MMIO16(sector);
	printf("# erase suspended: status %04X %04X\n", suspended_first, suspended_second);
	test_eq_u32("DQ6 stops toggling in the suspended sector", 0,
		(suspended_first ^ suspended_second) & AMD_STATUS_TOGGLE);
	test_check("DQ2 toggles in the suspended sector",
		((suspended_first ^ suspended_second) & AMD_STATUS_ERASE_TOGGLE) != 0);
	test_eq_memory("other sector of the bank reads array while suspended", other_data,
		(const void *) other_sector, sizeof(other_data));

	MMIO16(bank) = AMD_CMD_ERASE_RESUME;
	enum amd_operation_result result = amd_wait_ready(sector, flash->erase_timeout_ms, &toggles);
	test_eq_u32("resumed erase completes", AMD_OPERATION_DONE, result);
	test_check("resumed erase blanks the sector", cfi_range_is_blank(sector, size));
}

int main(void) {
	test_start("AMD CFI flash program and erase test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct amd_flash flash;
	if (!test_check("AMD CFI flash found", find_flash(&flash)))
		return test_finish();
	printf(
		"# CS%lu: %lu MiB, program timeout %lu ms, erase timeout %lu ms\n",
		flash.cs,
		flash.cfi.size >> 20,
		flash.program_timeout_ms,
		flash.erase_timeout_ms
	);

	uint32_t sector;
	uint32_t size;
	if (!amd_find_blank_sector(&flash, &sector, &size)) {
		test_skip("AMD program and erase", "no blank sector outside the first bank");
		return test_finish();
	}
	printf("# test sector %08lX, %lu KiB\n", sector, size >> 10);

	test_category("DYB protection");
	test_dyb_protection(&flash, sector);
	test_category("Word program");
	test_program_word(&flash, sector);
	test_category("Buffer program");
	test_program_buffer(&flash, sector);
	test_category("Write buffer abort");
	test_buffer_abort(&flash, sector);
	test_category("Sector erase");
	test_erase_sector(&flash, sector, size);
	test_category("Erase suspend");
	test_erase_suspend(&flash, sector, size);

	amd_write_dyb(&flash, sector, AMD_CMD_DYB_SET);
	return test_finish();
}
