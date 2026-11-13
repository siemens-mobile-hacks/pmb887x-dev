#include <pmb887x.h>

#include "intel.h"
#include "test.h"

static const uint16_t user_otp_value[] = {0x9400, 0x1099, 0x0460, 0xFF00};

static void test_operation(const char *name, uint16_t status) {
	test_check(name, (status & STATUS_READY) != 0 && (status & STATUS_ERRORS) == 0);
	if ((status & STATUS_READY) == 0 || (status & STATUS_ERRORS) != 0) {
		printf("# status=%04X\n", status);
	}
}

static bool find_flash(struct flash_device *flash) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		if (cfi_probe(cfi_chip_selects[i], flash) && flash->otp0_address && flash->otp1_address) {
			printf("# CS%lu %04lX:%04lX\n", flash->cs, flash->manufacturer, flash->device);
			return true;
		}
	}
	return false;
}

static bool program_otp(const struct flash_device *flash) {
	if (sizeof(user_otp_value) > flash->otp1_group_size) {
		test_skip("user OTP program", "PR1 group is smaller than the test value");
		return false;
	}

	uint16_t factory[flash->otp0_factory_size / 2];
	uint32_t factory_address = flash->base + (flash->otp0_address + 1) * 2;
	uint32_t user_address = flash->base + (flash->otp1_address + 1) * 2;
	cfi_enter_id(flash->base);
	uint16_t lock = cfi_read_word(flash->base, flash->otp1_address);
	for (uint32_t word = 0; word < ARRAY_SIZE(factory); word++) {
		factory[word] = cfi_read_word(flash->base, flash->otp0_address + 1 + word);
	}
	cfi_enter_read_array(flash->base);
	if (!(lock & BIT(0))) {
		test_skip("user OTP program", "PR1 group 0 is frozen");
		return false;
	}

	for (uint32_t word = 0; word < ARRAY_SIZE(user_otp_value); word++) {
		uint32_t word_address = user_address + word * 2;
		test_operation("user OTP word program completes", cfi_program_otp_word(word_address, user_otp_value[word]));
	}
	cfi_enter_id(flash->base);
	test_eq_memory("user OTP reads back", user_otp_value, (const void *) user_address, sizeof(user_otp_value));
	test_eq_memory("factory OTP is unchanged", factory, (const void *) factory_address, sizeof(factory));
	cfi_enter_read_array(flash->base);
	printf(
		"# programmed PR1 group 0: %04X %04X %04X %04X\n",
		user_otp_value[0],
		user_otp_value[1],
		user_otp_value[2],
		user_otp_value[3]
	);
	return true;
}

static void freeze_otp(const struct flash_device *flash) {
	uint32_t lock_address = flash->base + flash->otp1_address * 2;
	uint32_t user_address = flash->base + (flash->otp1_address + 1) * 2;
	cfi_enter_id(flash->base);
	uint16_t lock = MMIO16(lock_address);
	cfi_enter_read_array(flash->base);
	if (!(lock & BIT(0))) {
		test_skip("user OTP freeze", "PR1 group 0 is already frozen");
		return;
	}

	test_operation("user OTP freeze completes", cfi_program_otp_word(lock_address, lock & ~BIT(0)));
	cfi_enter_id(flash->base);
	test_eq_u32("user OTP is frozen", 0, MMIO16(lock_address) & BIT(0));
	test_eq_memory("freeze preserves user OTP", user_otp_value, (const void *) user_address, sizeof(user_otp_value));
	cfi_enter_read_array(flash->base);

	uint16_t status = cfi_program_otp_word(user_address, user_otp_value[0] & 0x0F0F);
	test_check("frozen OTP rejects programming", (status & STATUS_READY) != 0 && (status & STATUS_ERRORS) != 0);
	cfi_enter_id(flash->base);
	test_eq_memory(
		"rejected program preserves user OTP",
		user_otp_value,
		(const void *) user_address,
		sizeof(user_otp_value)
	);
	cfi_enter_read_array(flash->base);
	cfi_clear_status(user_address);
}

int main(void) {
	test_start("Intel/ST CFI OTP read/write test");
	printf("# WARNING: THIS TEST IRREVERSIBLY PROGRAMS AND FREEZES OTP\n");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct flash_device flash = {0};
	test_category("Discovery");
	bool found = find_flash(&flash);
	test_check("CFI flash with protection registers is found", found);
	if (!found) {
		return test_finish();
	}

	test_category("Irreversible OTP program");
	if (program_otp(&flash)) {
		test_category("Irreversible OTP freeze");
		freeze_otp(&flash);
	}
	return test_finish();
}
