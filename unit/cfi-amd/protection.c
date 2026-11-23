#include <pmb887x.h>

#include <stopwatch.h>

#include "amd.h"
#include "test.h"

#define PROTECTED_TEST_WORD 0x1234
#define PASSWORD_WORDS 4
#define PASSWORD_UNLOCK_US 2

static void print_result(const char *name, enum amd_operation_result result) {
	static const char *const RESULT_NAMES[] = {
		[AMD_OPERATION_DONE] = "done",
		[AMD_OPERATION_FAILED] = "failed",
		[AMD_OPERATION_TIMEOUT] = "timeout",
	};
	printf("# %s: %s\n", name, RESULT_NAMES[result]);
}

static void exit_secsi(uint32_t base) {
	amd_command(base, AMD_CMD_COMMAND_SET_EXIT);
	MMIO16(base) = AMD_CMD_COMMAND_SET_EXIT_CONFIRM;
}

static enum amd_operation_result program_secsi_word(const struct amd_flash *flash, uint32_t word, uint16_t value) {
	uint32_t address = flash->base + word * 2;
	uint32_t toggles;
	amd_command(flash->base, AMD_CMD_SECSI_SECTOR_ENTRY);
	amd_command(flash->base, AMD_CMD_PROGRAM);
	MMIO16(address) = value;
	enum amd_operation_result result = amd_wait_ready(address, flash->program_timeout_ms, &toggles);
	exit_secsi(flash->base);
	return result;
}

static uint16_t read_secsi_word(const struct amd_flash *flash, uint32_t word) {
	amd_command(flash->base, AMD_CMD_SECSI_SECTOR_ENTRY);
	uint16_t value = cfi_read_word(flash->base, word);
	exit_secsi(flash->base);
	return value;
}

static void test_customer_secsi_lock(const struct amd_flash *flash) {
	uint16_t lock = amd_read_lock_register(flash->base);
	if ((lock & AMD_LOCK_CUSTOMER_SECSI_PROTECTION) == 0) {
		test_skip("customer SecSi program and lock", "customer SecSi region is already locked");
		return;
	}
	uint32_t word = AMD_SECSI_CUSTOMER_WORD;
	while (word + 1 < AMD_SECSI_WORDS && (read_secsi_word(flash, word) & read_secsi_word(flash, word + 1)) != 0xFFFF)
		word++;
	if (word + 1 == AMD_SECSI_WORDS) {
		test_skip("customer SecSi program and lock", "customer SecSi region has no two blank words");
		return;
	}
	printf("# customer SecSi test words %02lX, %02lX\n", word, word + 1);

	test_eq_u32("customer SecSi program completes", AMD_OPERATION_DONE,
		program_secsi_word(flash, word, PROTECTED_TEST_WORD));
	test_eq_u32("customer SecSi word reads back", PROTECTED_TEST_WORD, read_secsi_word(flash, word));

	uint32_t toggles;
	uint32_t lock_address = flash->base + AMD_LOCK_REGISTER_WORD * 2;
	amd_command(flash->base, AMD_CMD_LOCK_REGISTER_ENTRY);
	MMIO16(flash->base) = AMD_CMD_PROGRAM;
	MMIO16(lock_address) = lock & ~AMD_LOCK_CUSTOMER_SECSI_PROTECTION;
	enum amd_operation_result lock_result = amd_wait_ready(lock_address, flash->program_timeout_ms, &toggles);
	amd_exit_command_set(flash->base);
	test_eq_u32("customer SecSi protection bit program completes", AMD_OPERATION_DONE, lock_result);
	test_eq_u32("lock register reports the customer SecSi protection", 0,
		amd_read_lock_register(flash->base) & AMD_LOCK_CUSTOMER_SECSI_PROTECTION);
	test_check("customer indicator bit reports the locked region",
		(amd_read_autoselect(flash->base, AMD_AUTOSELECT_INDICATOR_BITS) & AMD_INDICATOR_CUSTOMER_SECSI_LOCKED) != 0);

	print_result("locked customer SecSi program", program_secsi_word(flash, word + 1, PROTECTED_TEST_WORD));
	test_eq_u32("locked customer SecSi word is not programmed", 0xFFFF, read_secsi_word(flash, word + 1));
}

static void test_lock_register_program(const struct amd_flash *flash) {
	uint16_t lock = amd_read_lock_register(flash->base);
	if (lock & AMD_LOCK_PERSISTENT_PROTECTION_MODE) {
		test_skip("lock register reprogram", "persistent protection mode is not selected");
		return;
	}

	uint32_t address = flash->base + AMD_LOCK_REGISTER_WORD * 2;
	uint32_t toggles;
	amd_command(flash->base, AMD_CMD_LOCK_REGISTER_ENTRY);
	MMIO16(flash->base) = AMD_CMD_PROGRAM;
	MMIO16(address) = lock;
	enum amd_operation_result result = amd_wait_ready(address, flash->program_timeout_ms, &toggles);
	amd_exit_command_set(flash->base);

	print_result("repeated lock register program", result);
	test_eq_u32("repeated lock register program keeps the value", lock, amd_read_lock_register(flash->base));
}

static void test_ppb_protection(const struct amd_flash *flash, uint32_t sector, uint32_t size) {
	uint32_t toggles;
	amd_write_dyb(flash, sector, AMD_CMD_DYB_CLEAR);
	test_eq_u32("PPB of the test sector is clear", AMD_PROTECTION_CLEAR,
		amd_read_ppb(flash, sector) & AMD_PROTECTION_CLEAR);

	test_eq_u32("PPB program completes", AMD_OPERATION_DONE, amd_program_ppb(flash, sector));
	test_eq_u32("PPB program protects the sector", 0, amd_read_ppb(flash, sector) & AMD_PROTECTION_CLEAR);
	test_eq_u32("PPB program keeps the next sector clear", AMD_PROTECTION_CLEAR,
		amd_read_ppb(flash, sector + size) & AMD_PROTECTION_CLEAR);
	amd_start_program_word(flash, sector, PROTECTED_TEST_WORD);
	enum amd_operation_result protected_program = amd_wait_ready(sector, flash->program_timeout_ms, &toggles);
	amd_reset(sector);
	test_eq_u32("program of a PPB-protected sector completes", AMD_OPERATION_DONE, protected_program);
	test_eq_u32("PPB-protected sector keeps erased data", 0xFFFF, MMIO16(sector));

	test_eq_u32("all PPB erase completes", AMD_OPERATION_DONE, amd_erase_all_ppb(flash));
	test_eq_u32("all PPB erase clears the PPB", AMD_PROTECTION_CLEAR,
		amd_read_ppb(flash, sector) & AMD_PROTECTION_CLEAR);
	amd_start_program_word(flash, sector, PROTECTED_TEST_WORD);
	amd_wait_ready(sector, flash->program_timeout_ms, &toggles);
	test_eq_u32("sector is programmable after all PPB erase", PROTECTED_TEST_WORD, MMIO16(sector));

	amd_start_erase_sector(flash, sector);
	enum amd_operation_result erase = amd_wait_ready(sector, flash->erase_timeout_ms, &toggles);
	test_eq_u32("cleanup erase completes", AMD_OPERATION_DONE, erase);
	test_check("cleanup erase blanks the sector", cfi_range_is_blank(sector, size));
	amd_write_dyb(flash, sector, AMD_CMD_DYB_SET);
}

static void test_ppb_lock(const struct amd_flash *flash, uint32_t sector) {
	uint32_t last_bank = amd_bank_base(flash, flash->banks - 1);
	amd_set_ppb_lock(flash->base);
	test_eq_u32("PPB lock bit set", 0, amd_read_ppb_lock(flash->base) & AMD_PROTECTION_CLEAR);
	test_eq_u32("PPB lock bit is global", 0, amd_read_ppb_lock(last_bank) & AMD_PROTECTION_CLEAR);

	print_result("PPB program with the PPB lock bit set", amd_program_ppb(flash, sector));
	test_eq_u32("PPB lock bit prevents PPB program", AMD_PROTECTION_CLEAR,
		amd_read_ppb(flash, sector) & AMD_PROTECTION_CLEAR);
}

static void test_password_unlock(const struct amd_flash *flash) {
	uint16_t lock = amd_read_lock_register(flash->base);
	if (lock & AMD_LOCK_PERSISTENT_PROTECTION_MODE) {
		test_skip("password unlock in persistent mode", "persistent protection mode is not selected");
		return;
	}

	uint16_t password[PASSWORD_WORDS];
	amd_command(flash->base, AMD_CMD_PASSWORD_PROTECTION_ENTRY);
	for (uint32_t word = 0; word < PASSWORD_WORDS; word++)
		password[word] = cfi_read_word(flash->base, word);
	amd_exit_command_set(flash->base);
	printf("# password: %04X %04X %04X %04X\n", password[0], password[1], password[2], password[3]);

	amd_command(flash->base, AMD_CMD_PASSWORD_PROTECTION_ENTRY);
	MMIO16(flash->base) = AMD_CMD_PASSWORD_UNLOCK;
	MMIO16(flash->base) = AMD_CMD_PASSWORD_UNLOCK_WORD_COUNT;
	for (uint32_t word = 0; word < PASSWORD_WORDS; word++)
		MMIO16(flash->base + word * 2) = password[word];
	MMIO16(flash->base) = AMD_CMD_PASSWORD_UNLOCK_CONFIRM;
	stopwatch_usleep(PASSWORD_UNLOCK_US);
	amd_exit_command_set(flash->base);

	test_eq_u32("password unlock in persistent mode keeps the PPB lock bit", 0,
		amd_read_ppb_lock(flash->base) & AMD_PROTECTION_CLEAR);
}

int main(void) {
	test_start("AMD CFI flash protection test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct amd_flash flash;
	if (!amd_find_flash(&flash)) {
		test_skip("AMD CFI flash", "no AMD CFI flash found");
		return test_finish();
	}
	uint32_t sector;
	uint32_t size;
	if (!amd_find_blank_sector(&flash, flash.base, &sector, &size)) {
		test_skip("AMD sector protection", "no blank sector outside the first bank");
		return test_finish();
	}
	printf("# test sector %08lX, %lu KiB, lock register %04X\n", sector, size >> 10,
		amd_read_lock_register(flash.base));

	test_category("Customer SecSi lock");
	test_customer_secsi_lock(&flash);
	test_category("Lock register program");
	test_lock_register_program(&flash);
	test_category("PPB protection");
	test_ppb_protection(&flash, sector, size);
	test_category("PPB lock bit");
	test_ppb_lock(&flash, sector);
	test_category("Password unlock");
	test_password_unlock(&flash);
	return test_finish();
}
