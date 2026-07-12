#include <pmb887x.h>

#include "cfi.h"
#include "test.h"

static const uint16_t test_imei[] = {0x9400, 0x1099, 0x0460, 0xFF00};

static void test_operation(const char *name, uint16_t status) {
	test_check(name, (status & STATUS_READY) != 0 && (status & STATUS_ERRORS) == 0);
	if ((status & STATUS_READY) == 0 || (status & STATUS_ERRORS) != 0) {
		printf("# status=%04X\n", status);
	}
}

static bool find_flash(struct flash_device *flash) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		if (cfi_probe(cfi_chip_selects[i], flash)) {
			printf("# CS%u %04X:%04X\n", flash->cs, flash->manufacturer, flash->device);
			return true;
		}
	}
	return false;
}

static bool program_otp(const struct flash_device *flash) {
	uint16_t esn[4];
	uint32_t address = flash->base + 0x8A * 2;
	cfi_enter_id(flash->base);
	for (uint32_t word = 0; word < ARRAY_SIZE(esn); word++) {
		esn[word] = cfi_read_word(flash->base, 0x81 + word);
	}
	bool blank = true;
	for (uint32_t word = 0; word < ARRAY_SIZE(test_imei); word++) {
		blank &= MMIO16(address + word * 2) == 0xFFFF;
	}
	cfi_enter_read_array(flash->base);
	test_check("IMEI OTP slot is blank", blank);
	if (!blank) {
		return false;
	}

	for (uint32_t word = 0; word < ARRAY_SIZE(test_imei); word++) {
		uint32_t word_address = address + word * 2;
		test_operation("IMEI OTP word program completes", cfi_program_otp_word(word_address, test_imei[word]));
	}
	cfi_enter_id(flash->base);
	test_eq_memory("IMEI OTP reads back", test_imei, (const void *) address, sizeof(test_imei));
	test_eq_memory("factory ESN is unchanged", esn, (const void *) (flash->base + 0x81 * 2), sizeof(esn));
	cfi_enter_read_array(flash->base);
	printf("# programmed IMEI: 004999010640000\n");
	return true;
}

static void freeze_otp(const struct flash_device *flash) {
	uint32_t lock_address = flash->base + 0x89 * 2;
	uint32_t imei_address = flash->base + 0x8A * 2;
	cfi_enter_id(flash->base);
	uint16_t lock = MMIO16(lock_address);
	cfi_enter_read_array(flash->base);
	if (!(lock & BIT(0))) {
		test_skip("IMEI OTP freeze", "OTP region is already frozen");
		return;
	}

	test_operation("IMEI OTP freeze completes", cfi_program_otp_word(lock_address, lock & ~BIT(0)));
	cfi_enter_id(flash->base);
	test_eq_u32("IMEI OTP is frozen", 0, MMIO16(lock_address) & BIT(0));
	test_eq_memory("freeze preserves IMEI", test_imei, (const void *) imei_address, sizeof(test_imei));
	cfi_enter_read_array(flash->base);

	uint16_t status = cfi_program_otp_word(imei_address, test_imei[0] & 0x0F0F);
	test_check("frozen OTP rejects programming", (status & STATUS_READY) != 0 && (status & STATUS_ERRORS) != 0);
	cfi_enter_id(flash->base);
	test_eq_memory("rejected program preserves IMEI", test_imei, (const void *) imei_address, sizeof(test_imei));
	cfi_enter_read_array(flash->base);
	cfi_clear_status(imei_address);
}

int main(void) {
	test_start("Intel/ST CFI OTP read/write test");
	printf("# WARNING: THIS TEST IRREVERSIBLY PROGRAMS AND FREEZES OTP\n");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct flash_device flash = {0};
	test_category("Discovery");
	bool found = find_flash(&flash);
	test_check("CFI flash is found", found);
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
