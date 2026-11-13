#include <pmb887x.h>

#include <string.h>

#include "intel.h"
#include "test.h"

#define LOCK_COMMAND_BURST_ITERATIONS 64
#define WRITE_TEST_ALIGNMENT (1u << 20)
#define RCR_ASYNCHRONOUS BIT(15)
#define RCR_WAIT_POLARITY BIT(10)
#define RCR_BURST_LENGTH (BIT(2) | BIT(1) | BIT(0))
#define ECR_DRIVER_STRENGTH (BIT(2) | BIT(1) | BIT(0))
#define SPLIT_PROBE_WINDOW_EXPONENT 26
#define UNDECLARED_COMMAND_DATA_SEARCH_WORDS 0x10000
#define UNDECLARED_COMMANDS_MAX 16

enum ecr_driver_strength {
	ECR_DRIVER_STRENGTH_CODE_3 = 3,
	ECR_DRIVER_STRENGTH_CODE_4 = 4,
};

static struct flash_device flashes[FLASH_CHIP_SELECT_COUNT];
static uint32_t flash_count;
static bool write_test_area_found;

static void scan_flashes(void) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		uint32_t cs = cfi_chip_selects[i];
		printf("# probing CS%lu\n", cs);
		if (cfi_probe(cs, &flashes[flash_count])) {
			flash_count++;
		}
	}
	cfi_disable_chip_selects();
}

static uint16_t read_identifier_word(uint32_t address, uint32_t offset) {
	cfi_enter_id(address);
	uint16_t value = cfi_read_word(address, offset);
	cfi_enter_read_array(address);
	return value;
}

static bool flash_is_split_across(const struct flash_device *first, const struct flash_device *second) {
	uint32_t first_base = FLASH_WINDOW_BASE;
	uint32_t second_base = FLASH_WINDOW_BASE + (1u << SPLIT_PROBE_WINDOW_EXPONENT);
	cfi_disable_chip_selects();
	cfi_map(first->cs, first_base, 27 - SPLIT_PROBE_WINDOW_EXPONENT);
	cfi_map(second->cs, second_base, 27 - SPLIT_PROBE_WINDOW_EXPONENT);

	uint16_t rcr = read_identifier_word(first_base, 5);
	uint16_t second_rcr_before = read_identifier_word(second_base, 5);
	uint16_t probe_rcr = rcr ^ RCR_WAIT_POLARITY;
	cfi_program_configuration(first_base, probe_rcr, false);
	uint16_t second_rcr_after = read_identifier_word(second_base, 5);
	cfi_program_configuration(first_base, rcr, false);
	cfi_disable_chip_selects();

	return second_rcr_before == rcr && second_rcr_after == probe_rcr;
}

static void merge_split_flashes(void) {
	for (uint32_t i = 0; i < flash_count; i++) {
		for (uint32_t j = i + 1; j < flash_count; j++) {
			bool same_device = (
				flashes[i].manufacturer == flashes[j].manufacturer &&
				flashes[i].device == flashes[j].device &&
				flashes[i].cfi.size == flashes[j].cfi.size
			);
			if (!same_device || !flash_is_split_across(&flashes[i], &flashes[j])) {
				continue;
			}

			printf(
				"# CS%lu and CS%lu are windows of one flash: RCR written through CS%lu reads back through CS%lu\n",
				flashes[i].cs,
				flashes[j].cs,
				flashes[i].cs,
				flashes[j].cs
			);
			struct flash_device merged = flashes[i].cs < flashes[j].cs ? flashes[i] : flashes[j];
			merged.split = true;
			merged.high_cs = flashes[i].cs < flashes[j].cs ? flashes[j].cs : flashes[i].cs;
			merged.window_size = merged.cfi.size / 2;
			flashes[i] = merged;
			for (uint32_t k = j; k + 1 < flash_count; k++) {
				flashes[k] = flashes[k + 1];
			}
			flash_count--;
			return;
		}
	}
}

static bool configure_flash_map(void) {
	cfi_disable_chip_selects();

	uint32_t base = FLASH_WINDOW_BASE;
	for (uint32_t mapped = 0; mapped < flash_count; mapped++) {
		struct flash_device *flash = NULL;
		for (uint32_t i = 0; i < flash_count; i++) {
			if (!flashes[i].mapped && (!flash || flashes[i].cfi.size > flash->cfi.size)) {
				flash = &flashes[i];
			}
		}

		bool valid_size = (
			flash->cfi.size >= 1u << 20 &&
			flash->cfi.size <= 1u << 27 &&
			(flash->cfi.size & (flash->cfi.size - 1)) == 0 &&
			flash->cfi.size <= FLASH_WINDOW_END - base
		);
		if (!valid_size) {
			return false;
		}

		flash->base = base;
		flash->mapped = true;
		if (flash->split) {
			uint32_t window_exponent = flash->cfi.size_exponent - 1;
			cfi_map(flash->cs, base, 27 - window_exponent);
			cfi_map(flash->high_cs, base + flash->window_size, 27 - window_exponent);
			cfi_enter_read_array(base);
			cfi_enter_read_array(base + flash->window_size);
			cfi_sample_map(flash);
			printf(
				"# CS%lu+CS%lu mapped at %08lX..%08lX (%lu MiB flash split into two %lu MiB chip select windows)\n",
				flash->cs,
				flash->high_cs,
				base,
				base + flash->cfi.size - 1,
				flash->cfi.size >> 20,
				flash->window_size >> 20
			);
		} else {
			cfi_map(flash->cs, base, 27 - flash->cfi.size_exponent);
			printf("# CS%lu mapped at %08lX..%08lX\n", flash->cs, base, base + flash->cfi.size - 1);
		}
		base += flash->cfi.size;
	}
	return true;
}

static void test_identification(const struct flash_device *flash) {
	cfi_enter_id(flash->base);
	test_eq_u32("device ID matches scan", flash->device, cfi_read_word(flash->base, 1));
	cfi_enter_read_array(flash->base);
}

static void test_configuration_registers(const struct flash_device *flash) {
	uint32_t far_bank = flash->base + flash->cfi.size / 2;
	uint16_t rcr = read_identifier_word(flash->base, 5);
	uint16_t word6 = read_identifier_word(flash->base, 6);
	printf(
		"# RCR: %s read, WAIT active %s, burst length code %u; identifier word 6: %04X\n",
		(rcr & RCR_ASYNCHRONOUS) ? "asynchronous" : "synchronous",
		(rcr & RCR_WAIT_POLARITY) ? "high" : "low",
		rcr & RCR_BURST_LENGTH,
		word6
	);
	printf("# raw RCR=%04X word6=%04X\n", rcr, word6);

	uint16_t new_rcr = rcr ^ RCR_WAIT_POLARITY;
	cfi_program_configuration(flash->base, new_rcr, false);
	test_eq_u32("RCR stores the programmed value", new_rcr, read_identifier_word(flash->base, 5));
	test_eq_u32("RCR is shared by all partitions", new_rcr, read_identifier_word(far_bank, 5));
	cfi_program_configuration(flash->base, rcr, false);
	test_eq_u32("RCR is restored", rcr, read_identifier_word(flash->base, 5));

	if (flash->cfi.command_set == P_ID_INTEL_PERFORMANCE) {
		uint16_t driver_strength = (word6 & ECR_DRIVER_STRENGTH) == ECR_DRIVER_STRENGTH_CODE_4 ?
			ECR_DRIVER_STRENGTH_CODE_3 : ECR_DRIVER_STRENGTH_CODE_4;
		uint16_t new_ecr = (word6 & ~ECR_DRIVER_STRENGTH) | driver_strength;
		cfi_program_configuration(flash->base, new_ecr, true);
		test_eq_u32("ECR stores the programmed value", new_ecr, read_identifier_word(flash->base, 6));
		test_eq_u32("ECR is shared by all partitions", new_ecr, read_identifier_word(far_bank, 6));
		cfi_program_configuration(flash->base, word6, true);
		test_eq_u32("ECR is restored", word6, read_identifier_word(flash->base, 6));
	} else {
		cfi_program_configuration(flash->base, word6 ^ ECR_DRIVER_STRENGTH, true);
		uint16_t status = cfi_clear_status(flash->base);
		printf(
			"# ECR program without ECR: device %s, %s\n",
			(status & STATUS_READY) ? "ready" : "busy",
			(status & STATUS_ERRORS) ? "error bits set" : "no error bits"
		);
		printf("# raw status=%04X\n", status);
		test_eq_u32("identifier word 6 ignores ECR programming", word6, read_identifier_word(flash->base, 6));
	}
}

static void test_block_lock_status(const struct flash_device *flash) {
	cfi_enter_query(flash->base);
	uint16_t status_mask = cfi_read_u16(flash->base, flash->cfi.primary_address + 0xA);
	cfi_enter_read_array(flash->base);

	uint32_t offset = 0;
	bool valid = true;
	for (uint32_t region = 0; region < flash->cfi.regions; region++) {
		uint32_t states[4] = {0};
		for (uint32_t block = 0; block < flash->cfi.erase[region].blocks; block++) {
			uint32_t address = flash->base + offset;
			cfi_enter_id(address);
			uint16_t status = cfi_read_word(address, 2);
			cfi_enter_read_array(address);
			valid &= (status & ~status_mask) == 0;
			states[status & 0x3]++;
			offset += flash->cfi.erase[region].block_size;
			test_watchdog_serve();
		}
		printf(
			"# lock region %lu: unlocked=%lu locked=%lu unlocked-down=%lu locked-down=%lu\n",
			region,
			states[0],
			states[1],
			states[2],
			states[3]
		);
	}
	test_check("all erase block lock status values are valid", valid);
	test_eq_u32("lock status covers the complete flash", flash->cfi.size, offset);
}

static void test_lock_command_burst(const struct flash_device *flash) {
	uint32_t address = flash->base;
	uint16_t initial_lock = cfi_read_lock_status(address) & 0x3;

	if ((initial_lock & BIT(0)) == 0) {
		test_skip("repeated lock commands preserve block state", "first block is unlocked");
		return;
	}

	for (uint32_t iteration = 0; iteration < LOCK_COMMAND_BURST_ITERATIONS; iteration++) {
		MMIO16(address) = CFI_CMD_BLOCK_LOCK_SETUP;
		MMIO16(address) = CFI_CMD_BLOCK_LOCK_CONFIRM;
		MMIO16(address) = CFI_CMD_READ_ARRAY;
	}
	test_eq_u32("repeated lock commands preserve block state", initial_lock, cfi_read_lock_status(address) & 0x3);
}

static void read_otp_words(const struct flash_device *flash, uint32_t offset, uint16_t *words, uint32_t count) {
	cfi_enter_id(flash->base);
	for (uint32_t i = 0; i < count; i++) {
		words[i] = cfi_read_word(flash->base, offset + i);
	}
	cfi_enter_read_array(flash->base);
}

static bool otp_value_is_programmed(const uint16_t *words, uint32_t count) {
	uint16_t bits_set = 0;
	uint16_t bits_clear = 0;
	for (uint32_t i = 0; i < count; i++) {
		bits_set |= words[i];
		bits_clear |= ~words[i];
	}
	return bits_set != 0 && bits_clear != 0;
}

static void print_otp_value(const char *name, const uint16_t *words, uint32_t count) {
	printf("# %s: ", name);
	for (uint32_t i = 0; i < count; i++) {
		printf("%02X%02X", words[i] & 0xFF, words[i] >> 8);
	}
	printf("\n");
}

static void test_otp(const struct flash_device *flash) {
	if (!flash->otp0_address || !flash->otp1_address) {
		test_skip("protection registers", "primary extended query reports no protection registers");
		return;
	}

	uint32_t factory_words = flash->otp0_factory_size / 2;
	uint32_t pr0_user_words = flash->otp0_user_size / 2;
	uint32_t group_words = flash->otp1_group_size / 2;
	uint16_t factory[factory_words];
	uint16_t pr0_user[pr0_user_words];
	uint16_t group[group_words];
	read_otp_words(flash, flash->otp0_address + 1, factory, factory_words);
	read_otp_words(flash, flash->otp0_address + 1 + factory_words, pr0_user, pr0_user_words);
	read_otp_words(flash, flash->otp1_address + 1, group, group_words);
	cfi_enter_id(flash->base);
	uint16_t pr0_lock = cfi_read_word(flash->base, flash->otp0_address);
	uint16_t pr1_lock = cfi_read_word(flash->base, flash->otp1_address);
	cfi_enter_read_array(flash->base);
	printf(
		"# protection registers: PR0 at 0x%02lX (%lu factory + %lu user bytes), PR1 at 0x%02lX (%lu-byte groups)\n",
		flash->otp0_address,
		flash->otp0_factory_size,
		flash->otp0_user_size,
		flash->otp1_address,
		flash->otp1_group_size
	);
	printf("# protection locks: PR0=%04X PR1=%04X\n", pr0_lock, pr1_lock);
	print_otp_value("PR0 factory", factory, factory_words);
	print_otp_value("PR0 user", pr0_user, pr0_user_words);
	print_otp_value("PR1 group 0", group, group_words);
	test_check("factory protection register is programmed", otp_value_is_programmed(factory, factory_words));
}

static void read_efa_samples(const struct flash_device *flash, uint16_t samples[3][MAP_SAMPLE_WORDS]) {
	uint32_t offsets[] = {0, flash->efa_size / 2, flash->efa_size - sizeof(samples[0])};
	MMIO16(flash->base) = CFI_CMD_READ_EFA;
	for (uint32_t sample = 0; sample < ARRAY_SIZE(offsets); sample++) {
		for (uint32_t word = 0; word < MAP_SAMPLE_WORDS; word++) {
			samples[sample][word] = MMIO16(flash->base + offsets[sample] + word * 2);
		}
	}
	cfi_enter_read_array(flash->base);
}

static void test_efa(const struct flash_device *flash) {
	if (!flash->efa_size) {
		test_skip("extended flash array", "primary extended query reports no extended flash array");
		return;
	}

	uint16_t first[3][MAP_SAMPLE_WORDS];
	uint16_t second[3][MAP_SAMPLE_WORDS];
	uint16_t array_data[MAP_SAMPLE_WORDS];
	memcpy(array_data, (const void *) flash->base, sizeof(array_data));
	read_efa_samples(flash, first);
	read_efa_samples(flash, second);
	test_eq_memory("EFA start is stable", first[0], second[0], sizeof(first[0]));
	test_eq_memory("EFA middle is stable", first[1], second[1], sizeof(first[1]));
	test_eq_memory("EFA end is stable", first[2], second[2], sizeof(first[2]));
	test_eq_memory("read array restored after EFA mode", array_data, (const void *) flash->base, sizeof(array_data));
}

static uint32_t undeclared_commands(const struct flash_device *flash, uint16_t commands[UNDECLARED_COMMANDS_MAX]) {
	static const uint16_t UNDEFINED_COMMANDS[] = { 0x00, 0xAA, 0x55, 0xF0 };
	uint32_t count = 0;
	for (uint32_t i = 0; i < ARRAY_SIZE(UNDEFINED_COMMANDS); i++)
		commands[count++] = UNDEFINED_COMMANDS[i];

	if (flash->cfi.command_set == P_ID_INTEL_PERFORMANCE) {
		commands[count++] = CFI_CMD_WORD_PROGRAM_SETUP;
		commands[count++] = CFI_CMD_WORD_PROGRAM_SETUP_ALTERNATE;
		commands[count++] = CFI_CMD_BUFFERED_PROGRAM;
	} else {
		commands[count++] = CFI_CMD_PROGRAM_SETUP;
		commands[count++] = CFI_CMD_BUFFER_PROGRAM;
		commands[count++] = CFI_CMD_BLANK_CHECK_SETUP;
	}
	if (!flash->efa_size) {
		commands[count++] = CFI_CMD_READ_EFA;
		commands[count++] = CFI_CMD_EFA_PROGRAM_SETUP;
		commands[count++] = CFI_CMD_EFA_BLOCK_ERASE_SETUP;
		commands[count++] = CFI_CMD_EFA_BLOCK_LOCK_SETUP;
	}
	if (!(flash->features & BIT(6)))
		commands[count++] = CFI_CMD_PROTECTION_REGISTER_PROGRAM;
	return count;
}

static void test_undeclared_commands(const struct flash_device *flash) {
	uint16_t commands[UNDECLARED_COMMANDS_MAX];
	uint32_t commands_n = undeclared_commands(flash, commands);
	uint32_t data_offset = 0;
	while (cfi_read_word(flash->base, data_offset) == 0xFFFF && data_offset < UNDECLARED_COMMAND_DATA_SEARCH_WORDS)
		data_offset++;
	uint16_t array_word = cfi_read_word(flash->base, data_offset);

	bool keeps_read_array = true;
	bool keeps_identifier_mode = true;
	bool sets_no_error = true;
	printf("# undeclared commands:");
	for (uint32_t i = 0; i < commands_n; i++) {
		printf(" %02X", commands[i]);
		MMIO16(flash->base) = commands[i];
		keeps_read_array &= cfi_read_word(flash->base, data_offset) == array_word;
		MMIO16(flash->base) = CFI_CMD_READ_ARRAY;
		sets_no_error &= (cfi_clear_status(flash->base) & STATUS_ERRORS) == 0;

		cfi_enter_id(flash->base);
		MMIO16(flash->base) = commands[i];
		keeps_identifier_mode &= cfi_read_word(flash->base, 0) == flash->manufacturer;
		MMIO16(flash->base) = CFI_CMD_READ_ARRAY;
		sets_no_error &= (cfi_clear_status(flash->base) & STATUS_ERRORS) == 0;
	}
	printf("; array word at offset %lX: %04X\n", data_offset, array_word);
	if (array_word == 0xFFFF) {
		test_skip("undeclared commands keep read array mode", "no programmed word near the flash base");
	} else {
		test_check("undeclared commands keep read array mode", keeps_read_array);
	}
	test_check("undeclared commands keep identifier mode", keeps_identifier_mode);
	test_check("undeclared commands set no status error", sets_no_error);
}

static void test_status_register(const struct flash_device *flash) {
	uint16_t array_data = cfi_read_word(flash->base, 0);
	MMIO16(flash->base) = CFI_CMD_CLEAR_STATUS_REGISTER;
	test_eq_u32("clear status preserves read-array mode", array_data, cfi_read_word(flash->base, 0));

	cfi_enter_status(flash->base);
	uint16_t status = cfi_read_word(flash->base, 0);
	test_check("flash is ready", (status & STATUS_READY) != 0);
	test_eq_u32("status error bits are clear", 0, status & STATUS_ERRORS);
	test_eq_u32("status is independent of read address", status, cfi_read_word(flash->base, 1));
	cfi_enter_read_array(flash->base);
	test_eq_u32("read array restored after status mode", array_data, cfi_read_word(flash->base, 0));
}

static void test_blank_check(const struct flash_device *flash) {
	if (flash->cfi.command_set != P_ID_INTEL_PERFORMANCE) {
		test_skip("blank check command", "command set has no blank check");
		return;
	}

	uint32_t block_size = flash->cfi.erase[0].block_size;
	uint16_t samples[3][MAP_SAMPLE_WORDS];
	memcpy(samples[0], (const void *) flash->base, sizeof(samples[0]));
	memcpy(samples[1], (const void *) (flash->base + block_size / 2), sizeof(samples[1]));
	memcpy(samples[2], (const void *) (flash->base + block_size - sizeof(samples[2])), sizeof(samples[2]));
	MMIO16(flash->base) = CFI_CMD_BLANK_CHECK_SETUP;
	MMIO16(flash->base) = CFI_CMD_CONFIRM;
	uint16_t status = cfi_wait_ready(flash->base);
	printf("# blank check status=%04X\n", status);
	test_check("blank check completes", (status & STATUS_READY) != 0);
	test_check("blank check status is valid", (status & ~(STATUS_READY | BIT(5))) == 0);
	printf("# first block is %s\n", status & BIT(5) ? "not blank" : "blank");
	cfi_enter_read_array(flash->base);
	test_eq_memory("blank check preserves block start", samples[0], (const void *) flash->base, sizeof(samples[0]));
	test_eq_memory(
		"blank check preserves block middle",
		samples[1],
		(const void *) (flash->base + block_size / 2),
		sizeof(samples[1])
	);
	test_eq_memory(
		"blank check preserves block end",
		samples[2],
		(const void *) (flash->base + block_size - sizeof(samples[2])),
		sizeof(samples[2])
	);
	MMIO16(flash->base) = CFI_CMD_CLEAR_STATUS_REGISTER;
}

static void test_blank_space(struct flash_device *flash) {
	uint32_t range_base = 0;
	uint32_t range_size = 0;
	for (uint32_t offset = 0; offset < flash->cfi.size; offset += WRITE_TEST_ALIGNMENT) {
		test_watchdog_reset();
		if (cfi_range_is_blank(flash->base + offset, WRITE_TEST_ALIGNMENT)) {
			if (!range_size) {
				range_base = flash->base + offset;
			}
			range_size += WRITE_TEST_ALIGNMENT;
			continue;
		}

		if (range_size) {
			printf(
				"# blank range: %08lX..%08lX (%lu MiB)\n",
				range_base,
				range_base + range_size - 1,
				range_size >> 20
			);
			if (range_size > flash->blank_size) {
				flash->blank_base = range_base;
				flash->blank_size = range_size;
			}
			range_size = 0;
		}
	}

	if (range_size) {
		printf(
			"# blank range: %08lX..%08lX (%lu MiB)\n",
			range_base,
			range_base + range_size - 1,
			range_size >> 20
		);
		if (range_size > flash->blank_size) {
			flash->blank_base = range_base;
			flash->blank_size = range_size;
		}
	}
	if (flash->blank_size) {
		write_test_area_found = true;
		test_check("aligned blank area for write tests is found", true);
		printf(
			"# selected write test area: %08lX..%08lX (%lu MiB)\n",
			flash->blank_base,
			flash->blank_base + flash->blank_size - 1,
			flash->blank_size >> 20
		);
		if (flash->cfi.command_set == P_ID_INTEL_PERFORMANCE) {
			MMIO16(flash->blank_base) = CFI_CMD_BLANK_CHECK_SETUP;
			MMIO16(flash->blank_base) = CFI_CMD_CONFIRM;
			uint16_t status = cfi_wait_ready(flash->blank_base);
			test_check("blank check accepts erased block", (status & (STATUS_READY | BIT(5))) == STATUS_READY);
			cfi_enter_read_array(flash->blank_base);
			MMIO16(flash->blank_base) = CFI_CMD_CLEAR_STATUS_REGISTER;
		}
	} else {
		test_skip("aligned blank area for write tests", "none on this flash device");
	}
}

static uint32_t query_power_of_two(uint32_t value) {
	return value < 32 ? 1u << value : 0;
}

static void print_pri_regions(uint32_t base, uint32_t *cursor, bool st, const char *name) {
	uint32_t regions = cfi_read_word(base, (*cursor)++);
	printf("# PRI %s bank regions: %lu\n", name, regions);
	for (uint32_t region = 0; region < regions; region++) {
		uint32_t section_size = st ? cfi_read_u16(base, *cursor) : 0;
		*cursor += st ? 2 : 0;
		uint32_t banks = cfi_read_u16(base, *cursor);
		*cursor += 2;
		uint32_t simultaneous = cfi_read_word(base, (*cursor)++);
		uint32_t while_program = cfi_read_word(base, (*cursor)++);
		uint32_t while_erase = cfi_read_word(base, (*cursor)++);
		uint32_t erase_regions = cfi_read_word(base, (*cursor)++);
		printf(
			"# PRI %s region %lu: banks=%lu erase-regions=%lu ops=%02lX/%02lX/%02lX section=%lu\n",
			name,
			region,
			banks,
			erase_regions,
			simultaneous,
			while_program,
			while_erase,
			section_size
		);
		for (uint32_t erase = 0; erase < erase_regions; erase++) {
			uint32_t blocks = cfi_read_u16(base, *cursor) + 1;
			*cursor += 2;
			uint32_t block_size = cfi_read_u16(base, *cursor) * 256;
			*cursor += 2;
			uint32_t cycles = cfi_read_u16(base, *cursor) * 1000;
			*cursor += 2;
			uint32_t cell = cfi_read_word(base, (*cursor)++);
			uint32_t reads = cfi_read_word(base, (*cursor)++);
			printf(
				"# PRI %s region %lu erase %lu: %lu x %lu KiB, cycles=%lu, bits/cell=%lu ECC=%u reads=%02lX\n",
				name,
				region,
				erase,
				blocks,
				block_size >> 10,
				cycles,
				cell & 0xF,
				(cell & BIT(4)) != 0,
				reads
			);
			*cursor += st ? 6 : 0;
		}
	}
}

static void print_pri_tables(const struct flash_device *flash, uint32_t features) {
	uint32_t cursor = flash->cfi.primary_address + 0xE;
	if (features & BIT(6)) {
		uint32_t fields = cfi_read_word(flash->base, cursor++);
		printf("# PRI OTP fields: %lu\n", fields);
		for (uint32_t field = 0; field < fields; field++) {
			if (field == 0) {
				uint32_t address = cfi_read_u16(flash->base, cursor);
				cursor += 2;
				uint32_t factory = query_power_of_two(cfi_read_word(flash->base, cursor++));
				uint32_t user = query_power_of_two(cfi_read_word(flash->base, cursor++));
				printf("# PRI OTP%lu: address=%04lX factory=%lu bytes user=%lu bytes\n", field, address, factory, user);
			} else {
				uint32_t address = cfi_read_u32(flash->base, cursor);
				cursor += 4;
				uint32_t factory_groups = cfi_read_u16(flash->base, cursor);
				cursor += 2;
				uint32_t factory_size = query_power_of_two(cfi_read_word(flash->base, cursor++));
				uint32_t user_groups = cfi_read_u16(flash->base, cursor);
				cursor += 2;
				uint32_t user_size = query_power_of_two(cfi_read_word(flash->base, cursor++));
				printf(
					"# PRI OTP%lu: address=%08lX factory=%lu x %lu bytes user=%lu x %lu bytes\n",
					field,
					address,
					factory_groups,
					factory_size,
					user_groups,
					user_size
				);
			}
		}
	}

	uint32_t page_size = query_power_of_two(cfi_read_word(flash->base, cursor++));
	uint32_t synchronous_modes = cfi_read_word(flash->base, cursor++);
	printf("# PRI read capability: page=%lu bytes synchronous-modes=%lu", page_size, synchronous_modes);
	for (uint32_t mode = 0; mode < synchronous_modes; mode++) {
		printf(" mode%lu=%lu", mode, query_power_of_two(cfi_read_word(flash->base, cursor++)));
	}
	printf("\n");

	if (features & BIT(9)) {
		bool st = flash->cfi.command_set == P_ID_INTEL_PERFORMANCE;
		print_pri_regions(flash->base, &cursor, st, "array");
		if (st && (features & BIT(10))) {
			print_pri_regions(flash->base, &cursor, true, "EFA");
		}
	}
}

static void print_pri(const struct flash_device *flash) {
	cfi_enter_query(flash->base);
	uint32_t features = cfi_read_u32(flash->base, flash->cfi.primary_address + 5);
	printf(
		"# PRI: version=%c.%c features=%08lX suspend=%02X block-status=%04lX\n",
		cfi_read_word(flash->base, flash->cfi.primary_address + 3),
		cfi_read_word(flash->base, flash->cfi.primary_address + 4),
		features,
		cfi_read_word(flash->base, flash->cfi.primary_address + 9),
		cfi_read_u16(flash->base, flash->cfi.primary_address + 0xA)
	);
	printf(
		"# PRI features 0-4: chip-erase=%u erase-suspend=%u program-suspend=%u legacy-lock=%u queued-erase=%u\n",
		(features & BIT(0)) != 0,
		(features & BIT(1)) != 0,
		(features & BIT(2)) != 0,
		(features & BIT(3)) != 0,
		(features & BIT(4)) != 0
	);
	printf(
		"# PRI features 5-10: instant-lock=%u protection=%u page=%u synchronous=%u simultaneous=%u EFA=%u\n",
		(features & BIT(5)) != 0,
		(features & BIT(6)) != 0,
		(features & BIT(7)) != 0,
		(features & BIT(8)) != 0,
		(features & BIT(9)) != 0,
		(features & BIT(10)) != 0
	);
	printf(
		"# PRI features 30-31: CFI-links=%u optional-features=%u\n",
		(features & BIT(30)) != 0,
		(features & BIT(31)) != 0
	);
	printf(
		"# PRI optimum voltage: VCC=%X.%X V VPP=%X.%X V\n",
		cfi_read_word(flash->base, flash->cfi.primary_address + 0xC) >> 4,
		cfi_read_word(flash->base, flash->cfi.primary_address + 0xC) & 0xF,
		cfi_read_word(flash->base, flash->cfi.primary_address + 0xD) >> 4,
		cfi_read_word(flash->base, flash->cfi.primary_address + 0xD) & 0xF
	);
	print_pri_tables(flash, features);
	cfi_enter_read_array(flash->base);
}

static bool flash_words_match(uint32_t address, const uint16_t expected[MAP_SAMPLE_WORDS]) {
	for (uint32_t word = 0; word < MAP_SAMPLE_WORDS; word++) {
		if (MMIO16(address + word * 2) != expected[word]) {
			return false;
		}
	}
	return true;
}

static uint32_t find_command_partition_size(const struct flash_device *flash) {
	uint32_t offset = 0;
	for (uint32_t region = 0; region < flash->cfi.regions; region++) {
		for (uint32_t block = 0; block < flash->cfi.erase[region].blocks; block++) {
			offset += flash->cfi.erase[region].block_size;
			if (offset >= flash->cfi.size) {
				return 0;
			}

			uint32_t address = flash->base + offset + 0x10 * 2;
			uint16_t array_data[MAP_SAMPLE_WORDS];
			memcpy(array_data, (const void *) address, sizeof(array_data));
			cfi_enter_query(flash->base);
			bool independent = flash_words_match(address, array_data);
			cfi_enter_read_array(flash->base);
			if (independent) {
				return offset;
			}
			test_watchdog_serve();
		}
	}
	return 0;
}

static void test_partition_modes(const struct flash_device *flash) {
	cfi_enter_query(flash->base);
	bool simultaneous_operations = (cfi_read_u32(flash->base, flash->cfi.primary_address + 5) & BIT(9)) != 0;
	cfi_enter_read_array(flash->base);
	uint32_t partition_size = find_command_partition_size(flash);
	test_eq_u32("command partitions match PRI simultaneous operations", simultaneous_operations, partition_size != 0);
	if (!partition_size || partition_size * 2 > flash->cfi.size) {
		return;
	}

	uint32_t first = flash->base;
	uint32_t second = flash->base + partition_size;
	uint16_t first_array[MAP_SAMPLE_WORDS];
	uint16_t second_array[MAP_SAMPLE_WORDS];
	memcpy(first_array, (const void *) first, sizeof(first_array));
	memcpy(second_array, (const void *) second, sizeof(second_array));
	printf("# command partitions: first=%08lX second=%08lX size=%lu KiB\n", first, second, partition_size >> 10);

	cfi_enter_query(first);
	test_eq_memory("second partition remains in array mode", second_array, (const void *) second, sizeof(second_array));
	cfi_enter_status(second);
	test_eq_u32("first partition remains in CFI mode", 'Q', cfi_read_word(first, 0x10));
	test_check("second partition enters status mode", (cfi_read_word(second, 0) & STATUS_READY) != 0);

	cfi_enter_read_array(first);
	test_eq_memory("first partition reset is local", first_array, (const void *) first, sizeof(first_array));
	test_check("second partition remains in status mode", (cfi_read_word(second, 0) & STATUS_READY) != 0);

	cfi_enter_read_array(second);
	test_eq_memory("second partition returns to array mode", second_array, (const void *) second, sizeof(second_array));
}

static void test_flash_device_modes(void) {
	if (flash_count < 2) {
		test_skip("independent flash command modes", "only one flash device found");
		return;
	}

	for (uint32_t i = 0; i + 1 < flash_count; i++) {
		const struct flash_device *first = &flashes[i];
		const struct flash_device *second = &flashes[i + 1];
		uint16_t second_array[MAP_SAMPLE_WORDS];
		memcpy(second_array, (const void *) second->base, sizeof(second_array));

		cfi_enter_query(first->base);
		test_eq_memory(
			"other flash remains in array mode",
			second_array,
			(const void *) second->base,
			sizeof(second_array)
		);
		cfi_enter_id(second->base);
		test_eq_u32("first flash remains in CFI mode", 'Q', cfi_read_word(first->base, 0x10));
		test_eq_u32("second flash enters ID mode", second->manufacturer, cfi_read_word(second->base, 0));

		cfi_enter_read_array(first->base);
		test_eq_u32("second flash remains in ID mode", second->manufacturer, cfi_read_word(second->base, 0));
		cfi_enter_read_array(second->base);
		test_eq_memory(
			"second flash returns to array mode",
			second_array,
			(const void *) second->base,
			sizeof(second_array)
		);
	}
}

static void test_read_array_recovery(const struct flash_device *flash) {
	uint16_t array_data[] = {
		cfi_read_word(flash->base, 0),
		cfi_read_word(flash->base, 1),
		cfi_read_word(flash->base, 2),
		cfi_read_word(flash->base, 3),
	};

	cfi_enter_id(flash->base);
	cfi_enter_read_array(flash->base);
	test_eq_memory("read array restored after ID mode", array_data, (const void *) flash->base, sizeof(array_data));
}

static void test_flash_mapping(const struct flash_device *flash) {
	if (flash->split) {
		uint32_t window = (
			EBU_ADDRSEL_REGENAB |
			((28 - flash->cfi.size_exponent) << EBU_ADDRSEL_MASK_SHIFT)
		);
		test_eq_u32("EBU low window", window | flash->base, EBU_ADDRSEL(flash->cs));
		test_eq_u32(
			"EBU high window",
			window | (flash->base + flash->window_size),
			EBU_ADDRSEL(flash->high_cs)
		);
	} else {
		uint32_t expected_addrsel = (
			EBU_ADDRSEL_REGENAB |
			((27 - flash->cfi.size_exponent) << EBU_ADDRSEL_MASK_SHIFT) |
			flash->base
		);
		test_eq_u32("EBU address window", expected_addrsel, EBU_ADDRSEL(flash->cs));
	}
	test_eq_memory(
		"flash start is mapped",
		flash->map_samples[0],
		(const void *) flash->base,
		sizeof(flash->map_samples[0])
	);
	test_eq_memory(
		"flash middle is mapped",
		flash->map_samples[1],
		(const void *) (flash->base + flash->cfi.size / 2),
		sizeof(flash->map_samples[1])
	);
	test_eq_memory(
		"flash end is mapped",
		flash->map_samples[2],
		(const void *) (flash->base + flash->cfi.size - sizeof(flash->map_samples[2])),
		sizeof(flash->map_samples[2])
	);
}

int main(void) {
	test_start("Intel/ST CFI flash test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Scan");
	scan_flashes();
	merge_split_flashes();
	test_check("at least one CFI flash found", flash_count > 0);
	bool flash_map_valid = configure_flash_map();
	test_check("all flash windows fit EBU address space", flash_map_valid);
	if (!flash_map_valid) {
		return test_finish();
	}
	test_category("Flash device command modes");
	test_flash_device_modes();

	for (uint32_t i = 0; i < flash_count; i++) {
		printf("# testing CS%lu at %08lX\n", flashes[i].cs, flashes[i].base);
		test_category("EBU mapping");
		test_flash_mapping(&flashes[i]);
		test_category("Identification");
		test_identification(&flashes[i]);
		test_category("Configuration registers");
		test_configuration_registers(&flashes[i]);
		test_category("Erase block locks");
		test_block_lock_status(&flashes[i]);
		test_category("Block lock command burst");
		test_lock_command_burst(&flashes[i]);
		test_category("OTP");
		test_otp(&flashes[i]);
		test_category("Extended flash array");
		test_efa(&flashes[i]);
		test_category("Status register");
		test_status_register(&flashes[i]);
		test_category("Undeclared commands");
		test_undeclared_commands(&flashes[i]);
		test_category("Blank check");
		test_blank_check(&flashes[i]);
		test_category("Blank space");
		test_blank_space(&flashes[i]);
		print_pri(&flashes[i]);
		test_category("Command partitions");
		test_partition_modes(&flashes[i]);
		test_category("Read array recovery");
		test_read_array_recovery(&flashes[i]);
	}
	test_category("Write test area");
	test_check("at least one aligned blank area is available", write_test_area_found);

	return test_finish();
}
