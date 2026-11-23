#include <pmb887x.h>

#include <string.h>

#include "amd.h"
#include "test.h"

#define SECSI_DUMP_WORDS 8
#define DUMP_WORDS 0x100
#define DUMP_LINE_WORDS 32
#define INTEL_READ_ARRAY 0xFF
#define INTEL_READ_IDENTIFIER 0x90
#define NON_COMMAND_WRITE 0x0000
#define WRONG_UNLOCK1_ADDRESS 0x554
#define CONFIGURATION_REGISTER_DEFAULT 0xEFC8
#define CONFIGURATION_REGISTER_TEST 0x9FC2

static struct amd_flash flashes[FLASH_CHIP_SELECT_COUNT];
static uint32_t flash_count;

static void scan_flashes(void) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		uint32_t cs = cfi_chip_selects[i];
		printf("# probing CS%lu\n", cs);
		if (amd_probe(cs, &flashes[flash_count]))
			flash_count++;
	}
	cfi_disable_chip_selects();
}

static void configure_flash_map(void) {
	uint32_t base = FLASH_WINDOW_BASE;
	for (uint32_t i = 0; i < flash_count; i++) {
		struct amd_flash *flash = &flashes[i];
		flash->base = base;
		cfi_map(flash->cs, base, 27 - flash->cfi.size_exponent);
		printf("# CS%lu mapped at %08lX..%08lX\n", flash->cs, base, base + flash->cfi.size - 1);
		base += flash->cfi.size;
	}
}

static void print_flash(const struct amd_flash *flash) {
	printf(
		"# ID %04X:%04X:%04X:%04X, %lu MiB, write buffer %lu bytes, PRI %c.%c, %lu banks, unlock %s\n",
		flash->manufacturer,
		flash->device[0],
		flash->device[1],
		flash->device[2],
		flash->cfi.size >> 20,
		flash->cfi.write_buffer_size,
		(char) flash->pri_major,
		(char) flash->pri_minor,
		flash->banks,
		flash->address_sensitive_unlock ? "address sensitive" : "address insensitive"
	);
	for (uint32_t region = 0; region < flash->cfi.regions; region++) {
		printf(
			"# erase region %lu: %lu x %lu KiB\n",
			region,
			flash->cfi.erase[region].blocks,
			flash->cfi.erase[region].block_size >> 10
		);
	}
}

static void print_dump(const struct amd_flash *flash) {
	uint16_t dump[DUMP_WORDS];
	amd_command(flash->base, AMD_CMD_AUTOSELECT);
	for (uint32_t word = 0; word <= AMD_AUTOSELECT_DEVICE3; word++)
		dump[word] = cfi_read_word(flash->base, word);
	amd_reset(flash->base);
	cfi_enter_query(flash->base);
	for (uint32_t word = AMD_AUTOSELECT_DEVICE3 + 1; word < DUMP_WORDS; word++)
		dump[word] = cfi_read_word(flash->base, word);
	amd_reset(flash->base);

	for (uint32_t line = 0; line < DUMP_WORDS; line += DUMP_LINE_WORDS) {
		printf("# dump:");
		for (uint32_t word = line; word < line + DUMP_LINE_WORDS; word++)
			printf(" %04X", dump[word]);
		printf("\n");
	}
}

static void test_cfi_query(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	memcpy(array, (const void *) flash->base, sizeof(array));
	cfi_enter_query(flash->base);
	amd_reset(flash->base);
	test_eq_memory("reset leaves CFI query mode", array, (const void *) flash->base, sizeof(array));

	uint32_t bank_sectors = 0;
	for (uint32_t bank = 0; bank < flash->banks; bank++)
		bank_sectors += flash->bank_sectors[bank];
	test_eq_u32("PRI banks cover all erase sectors", amd_sector_count(flash), bank_sectors);
}

static void test_autoselect(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	memcpy(array, (const void *) flash->base, sizeof(array));

	amd_command(flash->base, AMD_CMD_AUTOSELECT);
	test_eq_u32("manufacturer stays readable", flash->manufacturer,
		cfi_read_word(flash->base, AMD_AUTOSELECT_MANUFACTURER));
	test_eq_u32("device word 1 stays readable", flash->device[0], cfi_read_word(flash->base, AMD_AUTOSELECT_DEVICE1));
	amd_reset(flash->base);
	test_eq_memory("reset leaves autoselect mode", array, (const void *) flash->base, sizeof(array));

	uint32_t last_bank = amd_bank_base(flash, flash->banks - 1);
	test_eq_u32("last bank reports the same manufacturer", flash->manufacturer,
		amd_read_autoselect(last_bank, AMD_AUTOSELECT_MANUFACTURER));
	test_eq_u32("last bank reports the same device word 3", flash->device[2],
		amd_read_autoselect(last_bank, AMD_AUTOSELECT_DEVICE3));
}

static void test_bank_local_modes(const struct amd_flash *flash) {
	if (flash->banks < 2) {
		test_skip("bank-local command modes", "PRI declares one bank");
		return;
	}

	uint32_t last_bank = amd_bank_base(flash, flash->banks - 1);
	printf("# last bank %lu at %08lX\n", flash->banks - 1, last_bank);
	uint16_t first_array[AMD_SAMPLE_WORDS];
	uint16_t last_array[AMD_SAMPLE_WORDS];
	memcpy(first_array, (const void *) flash->base, sizeof(first_array));
	memcpy(last_array, (const void *) last_bank, sizeof(last_array));

	cfi_enter_query(flash->base);
	test_eq_memory("other bank reads array during CFI query", last_array, (const void *) last_bank, sizeof(last_array));
	test_check("first bank stays in CFI query", cfi_has_query_signature(flash->base));
	amd_reset(flash->base);

	amd_command(last_bank, AMD_CMD_AUTOSELECT);
	test_eq_memory("other bank reads array during autoselect", first_array, (const void *) flash->base,
		sizeof(first_array));
	test_eq_u32("last bank stays in autoselect", flash->manufacturer,
		cfi_read_word(last_bank, AMD_AUTOSELECT_MANUFACTURER));
	amd_reset(last_bank);
	test_eq_memory("last bank returns to array", last_array, (const void *) last_bank, sizeof(last_array));
}

static void test_ignored_writes(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	memcpy(array, (const void *) flash->base, sizeof(array));

	MMIO16(flash->base) = INTEL_READ_IDENTIFIER;
	test_eq_memory("autoselect needs the unlock cycles", array, (const void *) flash->base, sizeof(array));
	amd_reset(flash->base);
	MMIO16(flash->base) = INTEL_READ_ARRAY;
	test_eq_memory("single-cycle FFh keeps array mode", array, (const void *) flash->base, sizeof(array));
	amd_reset(flash->base);

	if (!flash->address_sensitive_unlock) {
		test_skip("unlock address check", "PRI declares address-insensitive unlock");
		return;
	}
	MMIO16(flash->base + WRONG_UNLOCK1_ADDRESS * 2) = AMD_CMD_UNLOCK1;
	MMIO16(flash->base + AMD_UNLOCK2_ADDRESS * 2) = AMD_CMD_UNLOCK2;
	MMIO16(flash->base + AMD_UNLOCK1_ADDRESS * 2) = AMD_CMD_AUTOSELECT;
	test_eq_memory("unlock at a wrong address is ignored", array, (const void *) flash->base, sizeof(array));
	amd_reset(flash->base);
}

static void test_single_cycle_exits(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	memcpy(array, (const void *) flash->base, sizeof(array));

	uint16_t writes[] = { INTEL_READ_ARRAY, NON_COMMAND_WRITE };
	for (uint32_t i = 0; i < ARRAY_SIZE(writes); i++) {
		printf("# write %04X\n", writes[i]);
		cfi_enter_query(flash->base);
		MMIO16(flash->base) = writes[i];
		test_eq_memory("write leaves CFI query mode", array, (const void *) flash->base, sizeof(array));
		amd_reset(flash->base);

		amd_command(flash->base, AMD_CMD_AUTOSELECT);
		MMIO16(flash->base) = writes[i];
		test_eq_memory("write leaves autoselect mode", array, (const void *) flash->base, sizeof(array));
		amd_reset(flash->base);
	}
}

static void test_configuration_register(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	memcpy(array, (const void *) flash->base, sizeof(array));
	uint32_t last_bank = amd_bank_base(flash, flash->banks - 1);

	test_eq_u32("configuration register has the power-up value", CONFIGURATION_REGISTER_DEFAULT,
		amd_read_configuration_register(flash->base));
	amd_set_configuration_register(flash->base, CONFIGURATION_REGISTER_TEST);
	test_eq_u32("set configuration register stores the value", CONFIGURATION_REGISTER_TEST,
		amd_read_configuration_register(flash->base));
	test_eq_u32("configuration register is shared by all banks", CONFIGURATION_REGISTER_TEST,
		amd_read_configuration_register(last_bank));
	test_eq_memory("asynchronous array reads are unchanged", array, (const void *) flash->base, sizeof(array));
	amd_set_configuration_register(flash->base, CONFIGURATION_REGISTER_DEFAULT);
	test_eq_u32("configuration register returns to the power-up value", CONFIGURATION_REGISTER_DEFAULT,
		amd_read_configuration_register(flash->base));
}

static void test_lock_register(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	uint16_t last_array[AMD_SAMPLE_WORDS];
	uint32_t last_bank = amd_bank_base(flash, flash->banks - 1);
	memcpy(array, (const void *) flash->base, sizeof(array));
	memcpy(last_array, (const void *) last_bank, sizeof(last_array));

	amd_command(flash->base, AMD_CMD_LOCK_REGISTER_ENTRY);
	uint16_t lock = cfi_read_word(flash->base, AMD_LOCK_REGISTER_WORD);
	test_eq_memory("other bank reads array in lock register mode", last_array, (const void *) last_bank,
		sizeof(last_array));
	amd_exit_command_set(flash->base);
	printf("# lock register: %04X\n", lock);
	test_eq_memory("lock register exit returns to array", array, (const void *) flash->base, sizeof(array));

	uint16_t indicator = amd_read_autoselect(flash->base, AMD_AUTOSELECT_INDICATOR_BITS);
	bool indicator_locked = (indicator & AMD_INDICATOR_CUSTOMER_SECSI_LOCKED) != 0;
	bool lock_register_locked = (lock & AMD_LOCK_CUSTOMER_SECSI_PROTECTION) == 0;
	test_eq_u32("lock register and indicator agree on the customer SecSi lock", indicator_locked,
		lock_register_locked);
}

static void test_protection_state(const struct amd_flash *flash) {
	uint16_t array[AMD_SAMPLE_WORDS];
	memcpy(array, (const void *) flash->base, sizeof(array));
	uint32_t last_sector = flash->base + flash->cfi.size - flash->cfi.erase[flash->cfi.regions - 1].block_size;

	printf(
		"# PPB status: first sector %04X, last sector %04X\n",
		amd_read_ppb(flash, flash->base),
		amd_read_ppb(flash, last_sector)
	);
	printf(
		"# DYB status: first sector %04X, last sector %04X\n",
		amd_read_dyb(flash, flash->base),
		amd_read_dyb(flash, last_sector)
	);
	test_eq_u32("PPB lock bit is clear after power-up", AMD_PROTECTION_CLEAR,
		amd_read_ppb_lock(flash->base) & AMD_PROTECTION_CLEAR);

	amd_command(flash->base, AMD_CMD_SECSI_SECTOR_ENTRY);
	printf("# SecSi sector:");
	for (uint32_t word = 0; word < SECSI_DUMP_WORDS; word++)
		printf(" %04X", cfi_read_word(flash->base, word));
	printf("\n");
	amd_command(flash->base, AMD_CMD_COMMAND_SET_EXIT);
	MMIO16(flash->base) = AMD_CMD_COMMAND_SET_EXIT_CONFIRM;

	test_eq_memory("command set exits return to array", array, (const void *) flash->base, sizeof(array));
}

int main(void) {
	test_start("AMD CFI flash test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Scan");
	scan_flashes();
	if (flash_count == 0) {
		test_skip("AMD CFI flash", "no AMD CFI flash found");
		return test_finish();
	}
	configure_flash_map();

	for (uint32_t i = 0; i < flash_count; i++) {
		printf("# testing CS%lu at %08lX\n", flashes[i].cs, flashes[i].base);
		print_flash(&flashes[i]);
		print_dump(&flashes[i]);
		test_category("CFI query and banks");
		test_cfi_query(&flashes[i]);
		test_category("Autoselect");
		test_autoselect(&flashes[i]);
		test_category("Bank-local modes");
		test_bank_local_modes(&flashes[i]);
		test_category("Ignored writes");
		test_ignored_writes(&flashes[i]);
		test_category("Single-cycle exits");
		test_single_cycle_exits(&flashes[i]);
		test_category("Configuration register");
		test_configuration_register(&flashes[i]);
		test_category("Lock register");
		test_lock_register(&flashes[i]);
		test_category("Protection state");
		test_protection_state(&flashes[i]);
	}

	return test_finish();
}
