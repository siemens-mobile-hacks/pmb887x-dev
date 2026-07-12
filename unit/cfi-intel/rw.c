#include <pmb887x.h>

#include <string.h>

#include "cfi.h"
#include "test.h"

#define SEARCH_ALIGNMENT (1u << 20)

static bool find_test_flash(struct flash_device *flash) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		printf("# probing CS%u for a blank block\n", cfi_chip_selects[i]);
		if (!cfi_probe(cfi_chip_selects[i], flash)) {
			continue;
		}
		if (cfi_find_blank_block(flash, SEARCH_ALIGNMENT)) {
			printf(
				"# CS%u %04X:%04X block=%08X..%08X buffer=%u bytes\n",
				flash->cs,
				flash->manufacturer,
				flash->device,
				flash->test_block,
				flash->test_block + flash->test_block_size - 1,
				flash->write_buffer_size
			);
			return true;
		}
	}
	cfi_disable_chip_selects();
	return false;
}

static void test_operation(const char *name, uint16_t status) {
	test_check(name, (status & STATUS_READY) != 0 && (status & STATUS_ERRORS) == 0);
	if ((status & STATUS_READY) == 0 || (status & STATUS_ERRORS) != 0) {
		printf("# status=%04X\n", status);
	}
}

static void clear_status(uint32_t address) {
	test_eq_u32("status errors are cleared", 0, cfi_clear_status(address) & STATUS_ERRORS);
}

static void test_locked_operation(const struct flash_device *flash, bool erase, const char *name) {
	uint16_t status = erase ? cfi_erase_block(flash->test_block, false) :
		cfi_program_word(flash, flash->test_block, 0x5AA5, false);
	test_check(name, (status & (STATUS_READY | BIT(1))) == (STATUS_READY | BIT(1)));
	test_eq_u32("locked operation preserves data", 0xFFFF, MMIO16(flash->test_block));
	clear_status(flash->test_block);
}

static void program_buffer(const struct flash_device *flash, uint32_t address, const uint16_t *data, uint32_t words) {
	uint16_t buffer_status;
	uint16_t status = cfi_program_buffer(flash, address, data, words, &buffer_status);
	test_check("write buffer is ready", (buffer_status & STATUS_READY) != 0);
	test_operation("buffer program completes", status);
}

static uint16_t program_buffer_with_suspend(const struct flash_device *flash, uint32_t address, const uint16_t *data,
	uint32_t words, const uint16_t *read_sample, uint32_t read_address) {
	MMIO16(address) = 0x50;
	MMIO16(address) = flash->manufacturer == 0x0020 ? 0xE9 : 0xE8;
	uint16_t buffer_status = cfi_wait_ready(address);
	test_check("suspended write buffer is ready", (buffer_status & STATUS_READY) != 0);
	MMIO16(address) = words - 1;
	for (uint32_t word = 0; word < words; word++) {
		MMIO16(address + word * 2) = data[word];
	}
	MMIO16(address) = 0xD0;
	MMIO16(address) = 0xB0;
	MMIO16(address) = 0x70;
	uint16_t status = cfi_wait_ready(address);
	if (!(status & STATUS_PROGRAM_SUSPEND)) {
		test_skip("buffer program enters suspend", "program completed before suspend");
		cfi_enter_read_array(address);
		return status;
	}

	test_check("buffer program enters suspend", (status & STATUS_ERRORS) == 0);
	cfi_enter_read_array(address);
	test_eq_memory(
		"program suspend allows reading another block",
		read_sample,
		(const void *) read_address,
		MAP_SAMPLE_WORDS * sizeof(uint16_t)
	);
	MMIO16(address) = 0xD0;
	MMIO16(address) = 0x70;
	status = cfi_wait_ready(address);
	test_eq_u32("program suspend clears after resume", 0, status & STATUS_PROGRAM_SUSPEND);
	cfi_enter_read_array(address);
	return status;
}

static uint16_t erase_with_suspend(uint32_t address, const uint16_t *read_sample, uint32_t read_address) {
	MMIO16(address) = 0x50;
	MMIO16(address) = 0x20;
	MMIO16(address) = 0xD0;
	MMIO16(address) = 0xB0;
	MMIO16(address) = 0x70;
	uint16_t status = cfi_wait_ready(address);
	if (!(status & STATUS_ERASE_SUSPEND)) {
		test_skip("erase enters suspend", "erase completed before suspend");
		cfi_enter_read_array(address);
		return status;
	}

	test_check("erase enters suspend", (status & STATUS_ERRORS) == 0);
	cfi_enter_read_array(address);
	test_eq_memory(
		"erase suspend allows reading another block",
		read_sample,
		(const void *) read_address,
		MAP_SAMPLE_WORDS * sizeof(uint16_t)
	);
	MMIO16(address) = 0xD0;
	MMIO16(address) = 0x70;
	status = cfi_wait_ready(address);
	test_eq_u32("erase suspend clears after resume", 0, status & STATUS_ERASE_SUSPEND);
	cfi_enter_read_array(address);
	return status;
}

static void test_program_and_cleanup(struct flash_device *flash) {
	uint16_t buffer[512];
	uint32_t words = flash->write_buffer_size / 2;
	for (uint32_t word = 0; word < words; word++) {
		buffer[word] = 0xA500 ^ word;
	}

	uint16_t previous_block[MAP_SAMPLE_WORDS];
	uint16_t next_block[MAP_SAMPLE_WORDS];
	memcpy(previous_block, (const void *) (flash->test_block - sizeof(previous_block)), sizeof(previous_block));
	memcpy(next_block, (const void *) (flash->test_block + flash->test_block_size), sizeof(next_block));

	test_eq_u32("test block starts locked", 1, cfi_read_lock_status(flash->test_block) & 0x3);
	test_locked_operation(flash, false, "locked word program is rejected");
	test_locked_operation(flash, true, "locked erase is rejected");
	if (flash->manufacturer == 0x0020) {
		uint16_t blank_status = cfi_blank_check(flash->test_block);
		test_check("blank check reports erased block", (blank_status & (STATUS_READY | BIT(5))) == STATUS_READY);
	}
	test_operation("block unlock completes", cfi_unlock_block(flash->test_block, false));
	test_eq_u32("test block is unlocked", 0, cfi_read_lock_status(flash->test_block) & 0x3);

	test_operation("single-word program completes", cfi_program_word(flash, flash->test_block, 0x5AA5, false));
	test_eq_u32("single word reads back", 0x5AA5, MMIO16(flash->test_block));
	uint32_t short_buffer = flash->test_block + flash->write_buffer_size;
	program_buffer(flash, short_buffer, buffer, 1);
	test_eq_memory("one-word buffer reads back", buffer, (const void *) short_buffer, sizeof(buffer[0]));
	uint32_t odd_buffer = flash->test_block + flash->write_buffer_size * 2;
	program_buffer(flash, odd_buffer, buffer, 7);
	test_eq_memory("seven-word buffer reads back", buffer, (const void *) odd_buffer, sizeof(buffer[0]) * 7);
	uint32_t full_buffer = flash->test_block + flash->test_block_size - flash->write_buffer_size;
	uint16_t full_status;
	if (flash->features & BIT(2)) {
		full_status = program_buffer_with_suspend(
			flash,
			full_buffer,
			buffer,
			words,
			previous_block,
			flash->test_block - sizeof(previous_block)
		);
		test_operation("resumed buffer program completes", full_status);
	} else {
		test_skip("buffer program suspend", "not supported");
		program_buffer(flash, full_buffer, buffer, words);
	}
	test_eq_memory("full buffer reads back", buffer, (const void *) full_buffer, flash->write_buffer_size);
	if (flash->manufacturer == 0x0020) {
		test_check("blank check reports programmed block", (cfi_blank_check(flash->test_block) & BIT(5)) != 0);
	}

	test_operation("block lock completes", cfi_lock_block(flash->test_block, false));
	test_eq_u32("test block is locked again", 1, cfi_read_lock_status(flash->test_block) & 0x3);
	test_operation("block unlock completes", cfi_unlock_block(flash->test_block, false));
	uint16_t erase_status;
	if (flash->features & BIT(1)) {
		erase_status = erase_with_suspend(
			flash->test_block,
			previous_block,
			flash->test_block - sizeof(previous_block)
		);
	} else {
		test_skip("erase suspend", "not supported");
		erase_status = cfi_erase_block(flash->test_block, false);
	}
	test_operation("cleanup erase completes", erase_status);
	test_watchdog_reset();
	test_check("cleanup erased the complete block", cfi_range_is_blank(flash->test_block, flash->test_block_size));
	if (flash->manufacturer == 0x0020) {
		uint16_t blank_status = cfi_blank_check(flash->test_block);
		test_check("blank check accepts cleaned block", (blank_status & (STATUS_READY | BIT(5))) == STATUS_READY);
	}
	test_eq_memory(
		"previous block is unchanged",
		previous_block,
		(const void *) (flash->test_block - sizeof(previous_block)),
		sizeof(previous_block)
	);
	test_eq_memory(
		"next block is unchanged",
		next_block,
		(const void *) (flash->test_block + flash->test_block_size),
		sizeof(next_block)
	);
	test_operation("block lock completes", cfi_lock_block(flash->test_block, false));
	test_eq_u32("clean block is locked", 1, cfi_read_lock_status(flash->test_block) & 0x3);
	test_operation("block lock-down completes", cfi_lock_down_block(flash->test_block, false));
	test_eq_u32("clean block is locked-down", 3, cfi_read_lock_status(flash->test_block) & 0x3);
	test_operation("locked-down block unlock completes", cfi_unlock_block(flash->test_block, false));
	uint32_t unlocked_state = cfi_read_lock_status(flash->test_block) & 0x3;
	test_check("locked-down block unlock state is valid", unlocked_state == 0 || unlocked_state == 2);
	test_operation("block final lock completes", cfi_lock_block(flash->test_block, false));
	uint32_t final_state = unlocked_state == 0 ? 1 : 3;
	test_eq_u32("clean block final lock state", final_state, cfi_read_lock_status(flash->test_block) & 0x3);
}

int main(void) {
	test_start("Intel/ST CFI flash read/write test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct flash_device flash = {0};
	test_category("Discovery");
	bool found = find_test_flash(&flash);
	test_check("blank erase block is found", found);
	if (!found) {
		return test_finish();
	}

	test_category("Program and cleanup");
	test_program_and_cleanup(&flash);
	return test_finish();
}
