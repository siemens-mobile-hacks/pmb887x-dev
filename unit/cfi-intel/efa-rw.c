#include <pmb887x.h>

#include <string.h>

#include "cfi.h"
#include "test.h"

static bool find_test_flash(struct flash_device *flash) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		printf("# probing CS%u for EFA\n", cfi_chip_selects[i]);
		if (!cfi_probe(cfi_chip_selects[i], flash)) {
			continue;
		}
		bool valid_efa = (
			(flash->features & BIT(10)) != 0 &&
			flash->efa_blocks > 0 &&
			flash->efa_block_size > 0
		);
		if (!valid_efa) {
			continue;
		}
		printf(
			"# CS%u %04X:%04X EFA=%u blocks x %u KiB\n",
			flash->cs,
			flash->manufacturer,
			flash->device,
			flash->efa_blocks,
			flash->efa_block_size >> 10
		);
		return true;
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

static void test_efa_program_and_cleanup(const struct flash_device *flash, uint32_t address) {
	uint16_t main_array[MAP_SAMPLE_WORDS];
	memcpy(main_array, (const void *) flash->base, sizeof(main_array));

	test_eq_u32("EFA block starts locked", 1, cfi_read_efa_lock_status(address));
	test_operation("EFA block unlock completes", cfi_unlock_block(address, true));
	test_eq_u32("EFA block is unlocked", 0, cfi_read_efa_lock_status(address));
	test_operation("EFA word program completes", cfi_program_word(flash, address, 0xA55A, true));
	test_eq_u32("EFA word reads back", 0xA55A, cfi_read_efa_word(address));
	test_eq_memory("main array is unchanged", main_array, (const void *) flash->base, sizeof(main_array));

	test_operation("EFA block lock completes", cfi_lock_block(address, true));
	test_eq_u32("EFA block is locked again", 1, cfi_read_efa_lock_status(address));
	test_operation("EFA block unlock completes", cfi_unlock_block(address, true));
	test_operation("EFA cleanup erase completes", cfi_erase_block(address, true));
	test_watchdog_reset();
	test_check("EFA cleanup erased the complete block", cfi_efa_range_is_blank(address, flash->efa_block_size));
	test_operation("EFA block lock completes", cfi_lock_block(address, true));
	test_eq_u32("clean EFA block is locked", 1, cfi_read_efa_lock_status(address));
	test_eq_memory("cleanup preserves main array", main_array, (const void *) flash->base, sizeof(main_array));
	test_operation("EFA block lock-down completes", cfi_lock_down_block(address, true));
	test_eq_u32("clean EFA block is locked-down", 3, cfi_read_efa_lock_status(address));
	test_operation("locked-down EFA block unlock completes", cfi_unlock_block(address, true));
	uint32_t unlocked_state = cfi_read_efa_lock_status(address);
	test_check("locked-down EFA block unlock state is valid", unlocked_state == 0 || unlocked_state == 2);
	test_operation("EFA block final lock completes", cfi_lock_block(address, true));
	uint32_t final_state = unlocked_state == 0 ? 1 : 3;
	test_eq_u32("clean EFA block final lock state", final_state, cfi_read_efa_lock_status(address));
}

int main(void) {
	test_start("Intel/ST CFI EFA read/write test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct flash_device flash = {0};
	test_category("Discovery");
	bool found = find_test_flash(&flash);
	if (!found) {
		test_skip("EFA flash is found", "no flash with EFA support");
		return test_finish();
	}
	test_check("EFA flash is found", true);
	uint32_t address = 0;
	bool blank = cfi_find_blank_efa_block(&flash, &address);
	if (!blank) {
		test_skip("blank EFA block is found", "all EFA blocks contain data");
		return test_finish();
	}
	test_check("blank EFA block is found", true);
	printf("# selected EFA block: %08X..%08X\n", address, address + flash.efa_block_size - 1);

	test_category("Program and cleanup");
	test_efa_program_and_cleanup(&flash, address);
	return test_finish();
}
