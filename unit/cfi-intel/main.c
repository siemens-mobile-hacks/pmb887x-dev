#include <pmb887x.h>

#include <string.h>

#include "cfi.h"
#include "test.h"

#define EFA_SIZE 0x8000
#define WRITE_TEST_ALIGNMENT (1u << 20)

static struct flash_device flashes[FLASH_CHIP_SELECT_COUNT];
static uint32_t flash_count;
static bool write_test_area_found;

static void scan_flashes(void) {
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		uint32_t cs = cfi_chip_selects[i];
		printf("# probing CS%u\n", cs);
		if (cfi_probe(cs, &flashes[flash_count])) {
			flash_count++;
		}
	}
	cfi_disable_chip_selects();
}

static bool configure_flash_map(void) {
	cfi_disable_chip_selects();

	uint32_t base = FLASH_WINDOW_BASE;
	for (uint32_t mapped = 0; mapped < flash_count; mapped++) {
		struct flash_device *flash = NULL;
		for (uint32_t i = 0; i < flash_count; i++) {
			if (!flashes[i].mapped && (!flash || flashes[i].size > flash->size)) {
				flash = &flashes[i];
			}
		}

		bool valid_size = (
			flash->size >= 1u << 20 &&
			flash->size <= 1u << 27 &&
			(flash->size & (flash->size - 1)) == 0 &&
			flash->size <= FLASH_WINDOW_END - base
		);
		if (!valid_size) {
			return false;
		}

		flash->base = base;
		flash->mapped = true;
		cfi_map(flash->cs, base, 27 - flash->size_exponent);
		printf("# CS%u mapped at %08X..%08X\n", flash->cs, base, base + flash->size - 1);
		base += flash->size;
	}
	return true;
}

static void test_identification(const struct flash_device *flash) {
	cfi_enter_id(flash->base);
	uint16_t manufacturer = cfi_read_word(flash->base, 0);
	test_check("manufacturer is Intel or ST", manufacturer == 0x0089 || manufacturer == 0x0020);
	test_eq_u32("device ID matches scan", flash->device, cfi_read_word(flash->base, 1));
	cfi_enter_read_array(flash->base);
}

static void test_electronic_signature(const struct flash_device *flash) {
	cfi_enter_id(flash->base);
	uint16_t configuration = cfi_read_word(flash->base, 5);
	uint16_t enhanced_configuration = cfi_read_word(flash->base, 6);
	test_check("configuration register is readable", configuration != 0xFFFF);
	test_check("enhanced configuration register is readable", enhanced_configuration != 0xFFFF);
	cfi_enter_read_array(flash->base);
	cfi_program_configuration(flash->base, configuration, false);
	cfi_program_configuration(flash->base, enhanced_configuration, true);
	cfi_enter_id(flash->base);
	test_eq_u32("configuration register accepts programming command", configuration, cfi_read_word(flash->base, 5));
	uint16_t actual_enhanced_configuration = cfi_read_word(flash->base, 6);
	test_eq_u32("enhanced configuration register is programmed", enhanced_configuration, actual_enhanced_configuration);
	cfi_enter_read_array(flash->base);
	printf("# CR=%04X ECR=%04X\n", configuration, enhanced_configuration);
}

static void test_block_lock_status(const struct flash_device *flash) {
	uint32_t offset = 0;
	bool valid = true;
	uint16_t status_mask = flash->manufacturer == 0x0020 ? 0x33 : 0x03;
	for (uint32_t region = 0; region < flash->regions; region++) {
		uint32_t states[4] = {0};
		for (uint32_t block = 0; block < flash->erase[region].blocks; block++) {
			uint32_t address = flash->base + offset;
			cfi_enter_id(address);
			uint16_t status = cfi_read_word(address, 2);
			cfi_enter_read_array(address);
			valid &= (status & ~status_mask) == 0;
			states[status & 0x3]++;
			offset += flash->erase[region].block_size;
			test_watchdog_serve();
		}
		printf(
			"# lock region %u: unlocked=%u locked=%u unlocked-down=%u locked-down=%u\n",
			region,
			states[0],
			states[1],
			states[2],
			states[3]
		);
	}
	test_check("all erase block lock status values are valid", valid);
	test_eq_u32("lock status covers the complete flash", flash->size, offset);
}

static void read_otp_words(const struct flash_device *flash, uint32_t offset, uint16_t words[4]) {
	cfi_enter_id(flash->base);
	for (uint32_t i = 0; i < 4; i++) {
		words[i] = cfi_read_word(flash->base, offset + i);
	}
	cfi_enter_read_array(flash->base);
}

static bool otp_value_is_programmed(const uint16_t words[4]) {
	uint16_t bits_set = 0;
	uint16_t bits_clear = 0;
	for (uint32_t i = 0; i < 4; i++) {
		bits_set |= words[i];
		bits_clear |= ~words[i];
	}
	return bits_set != 0 && bits_clear != 0;
}

static void print_otp_value(const char *name, const uint16_t words[4]) {
	printf("# %s: ", name);
	for (uint32_t i = 0; i < 4; i++) {
		printf("%02X%02X", words[i] & 0xFF, words[i] >> 8);
	}
	printf("\n");
}

static void test_otp(const struct flash_device *flash) {
	uint16_t esn[4];
	uint16_t imei[4];
	read_otp_words(flash, 0x81, esn);
	read_otp_words(flash, 0x8A, imei);
	cfi_enter_id(flash->base);
	uint16_t esn_lock = cfi_read_word(flash->base, 0x80);
	uint16_t imei_lock = cfi_read_word(flash->base, 0x89);
	cfi_enter_read_array(flash->base);
	print_otp_value("ESN", esn);
	print_otp_value("IMEI", imei);
	printf(
		"# OTP locks: ESN=%s IMEI=%s (PR0=%04X PR1-16=%04X)\n",
		esn_lock & BIT(0) ? "open" : "closed",
		imei_lock & BIT(0) ? "open" : "closed",
		esn_lock,
		imei_lock
	);
	test_check("ESN is programmed", otp_value_is_programmed(esn));
	if (otp_value_is_programmed(imei)) {
		test_check("IMEI is programmed", true);
	} else {
		test_skip("IMEI is programmed", "OTP slot is erased");
	}
}

static void read_efa_samples(const struct flash_device *flash, uint16_t samples[3][MAP_SAMPLE_WORDS]) {
	uint32_t offsets[] = {0, EFA_SIZE / 2, EFA_SIZE - sizeof(samples[0])};
	MMIO16(flash->base) = 0x94;
	for (uint32_t sample = 0; sample < ARRAY_SIZE(offsets); sample++) {
		for (uint32_t word = 0; word < MAP_SAMPLE_WORDS; word++) {
			samples[sample][word] = MMIO16(flash->base + offsets[sample] + word * 2);
		}
	}
	cfi_enter_read_array(flash->base);
}

static void test_efa(const struct flash_device *flash) {
	if (flash->manufacturer != 0x0020) {
		test_skip("extended flash array", "not an ST M58PR flash");
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

static void test_status_register(const struct flash_device *flash) {
	uint16_t array_data = cfi_read_word(flash->base, 0);
	MMIO16(flash->base) = 0x50;
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
	if (flash->manufacturer != 0x0020) {
		test_skip("blank check command", "not an ST M58PR flash");
		return;
	}

	uint32_t block_size = flash->erase[0].block_size;
	uint16_t samples[3][MAP_SAMPLE_WORDS];
	memcpy(samples[0], (const void *) flash->base, sizeof(samples[0]));
	memcpy(samples[1], (const void *) (flash->base + block_size / 2), sizeof(samples[1]));
	memcpy(samples[2], (const void *) (flash->base + block_size - sizeof(samples[2])), sizeof(samples[2]));
	MMIO16(flash->base) = 0xBC;
	MMIO16(flash->base) = 0xD0;
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
	MMIO16(flash->base) = 0x50;
}

static void test_blank_space(struct flash_device *flash) {
	uint32_t range_base = 0;
	uint32_t range_size = 0;
	for (uint32_t offset = 0; offset < flash->size; offset += WRITE_TEST_ALIGNMENT) {
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
				"# blank range: %08X..%08X (%u MiB)\n",
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
			"# blank range: %08X..%08X (%u MiB)\n",
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
			"# selected write test area: %08X..%08X (%u MiB)\n",
			flash->blank_base,
			flash->blank_base + flash->blank_size - 1,
			flash->blank_size >> 20
		);
		if (flash->manufacturer == 0x0020) {
			MMIO16(flash->blank_base) = 0xBC;
			MMIO16(flash->blank_base) = 0xD0;
			uint16_t status = cfi_wait_ready(flash->blank_base);
			test_check("blank check accepts erased block", (status & (STATUS_READY | BIT(5))) == STATUS_READY);
			cfi_enter_read_array(flash->blank_base);
			MMIO16(flash->blank_base) = 0x50;
		}
	} else {
		test_skip("aligned blank area for write tests", "none on this flash device");
	}
}

static void test_cfi_query(const struct flash_device *flash) {
	cfi_enter_query(flash->base);
	test_eq_u32("CFI Q signature", 'Q', cfi_read_word(flash->base, 0x10));
	test_eq_u32("CFI R signature", 'R', cfi_read_word(flash->base, 0x11));
	test_eq_u32("CFI Y signature", 'Y', cfi_read_word(flash->base, 0x12));
	test_check("primary command set is present", cfi_read_u16(flash->base, 0x13) != 0);
	test_check("primary table address is valid", flash->primary_address >= 0x31 && flash->primary_address < 0x1000);
	test_eq_u32("primary P signature", 'P', cfi_read_word(flash->base, flash->primary_address));
	test_eq_u32("primary R signature", 'R', cfi_read_word(flash->base, flash->primary_address + 1));
	test_eq_u32("primary I signature", 'I', cfi_read_word(flash->base, flash->primary_address + 2));
	test_check("device size is valid", (
		flash->size_exponent >= 20 &&
		flash->size_exponent <= 27 &&
		(flash->size & (flash->size - 1)) == 0
	));
	test_check("erase region count is valid", flash->regions > 0 && flash->regions <= CFI_MAX_ERASE_REGIONS);

	uint32_t erase_size = 0;
	for (uint32_t region = 0; region < flash->regions && region < CFI_MAX_ERASE_REGIONS; region++) {
		test_check("erase region has blocks", flash->erase[region].blocks > 0);
		uint32_t block_size = flash->erase[region].block_size;
		test_check("erase block size is valid", (
			block_size >= 256 &&
			(block_size & (block_size - 1)) == 0
		));
		erase_size += flash->erase[region].blocks * flash->erase[region].block_size;
	}
	test_eq_u32("erase regions cover device", flash->size, erase_size);
	cfi_enter_read_array(flash->base);
}

static uint32_t query_power_of_two(uint32_t value) {
	return value < 32 ? 1u << value : 0;
}

static void print_pri_regions(uint32_t base, uint32_t *cursor, bool st, const char *name) {
	uint32_t regions = cfi_read_word(base, (*cursor)++);
	printf("# PRI %s bank regions: %u\n", name, regions);
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
			"# PRI %s region %u: banks=%u erase-regions=%u ops=%02X/%02X/%02X section=%u\n",
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
				"# PRI %s region %u erase %u: %u x %u KiB, cycles=%u, bits/cell=%u ECC=%u reads=%02X\n",
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

static void print_pri_information(const struct flash_device *flash, uint32_t features) {
	uint32_t cursor = flash->primary_address + 0xE;
	if (features & BIT(6)) {
		uint32_t fields = cfi_read_word(flash->base, cursor++);
		printf("# PRI OTP fields: %u\n", fields);
		for (uint32_t field = 0; field < fields; field++) {
			if (field == 0) {
				uint32_t address = cfi_read_u16(flash->base, cursor);
				cursor += 2;
				uint32_t factory = query_power_of_two(cfi_read_word(flash->base, cursor++));
				uint32_t user = query_power_of_two(cfi_read_word(flash->base, cursor++));
				printf("# PRI OTP%u: address=%04X factory=%u bytes user=%u bytes\n", field, address, factory, user);
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
					"# PRI OTP%u: address=%08X factory=%u x %u bytes user=%u x %u bytes\n",
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
	printf("# PRI read capability: page=%u bytes synchronous-modes=%u", page_size, synchronous_modes);
	for (uint32_t mode = 0; mode < synchronous_modes; mode++) {
		printf(" mode%u=%u", mode, query_power_of_two(cfi_read_word(flash->base, cursor++)));
	}
	printf("\n");

	if (features & BIT(9)) {
		bool st = cfi_read_u16(flash->base, 0x13) == 0x0200;
		print_pri_regions(flash->base, &cursor, st, "array");
		if (st && (features & BIT(10))) {
			print_pri_regions(flash->base, &cursor, true, "EFA");
		}
	}
}

static void print_cfi_information(const struct flash_device *flash) {
	cfi_enter_query(flash->base);
	uint32_t primary_set = cfi_read_u16(flash->base, 0x13);
	uint32_t alternate_set = cfi_read_u16(flash->base, 0x17);
	uint32_t alternate_address = cfi_read_u16(flash->base, 0x19);
	uint32_t interface = cfi_read_u16(flash->base, 0x28);
	uint32_t write_buffer_exponent = cfi_read_u16(flash->base, 0x2A);
	uint32_t features = cfi_read_u32(flash->base, flash->primary_address + 5);

	printf(
		"# CFI: primary=%04X at %04X alternate=%04X at %04X interface=%04X\n",
		primary_set,
		flash->primary_address,
		alternate_set,
		alternate_address,
		interface
	);
	printf(
		"# voltage: VCC=%X.%X..%X.%X V VPP=%X.%X..%X.%X V\n",
		cfi_read_word(flash->base, 0x1B) >> 4,
		cfi_read_word(flash->base, 0x1B) & 0xF,
		cfi_read_word(flash->base, 0x1C) >> 4,
		cfi_read_word(flash->base, 0x1C) & 0xF,
		cfi_read_word(flash->base, 0x1D) >> 4,
		cfi_read_word(flash->base, 0x1D) & 0xF,
		cfi_read_word(flash->base, 0x1E) >> 4,
		cfi_read_word(flash->base, 0x1E) & 0xF
	);
	printf(
		"# timeout exponents: word=%u buffer=%u erase=%u chip=%u max=%u/%u/%u/%u\n",
		cfi_read_word(flash->base, 0x1F),
		cfi_read_word(flash->base, 0x20),
		cfi_read_word(flash->base, 0x21),
		cfi_read_word(flash->base, 0x22),
		cfi_read_word(flash->base, 0x23),
		cfi_read_word(flash->base, 0x24),
		cfi_read_word(flash->base, 0x25),
		cfi_read_word(flash->base, 0x26)
	);
	printf(
		"# geometry: size=%u MiB write-buffer=%u bytes erase-regions=%u\n",
		flash->size >> 20,
		write_buffer_exponent < 32 ? 1u << write_buffer_exponent : 0,
		flash->regions
	);

	uint32_t region_start = 0;
	for (uint32_t region = 0; region < flash->regions && region < CFI_MAX_ERASE_REGIONS; region++) {
		uint32_t region_size = flash->erase[region].blocks * flash->erase[region].block_size;
		printf(
			"# erase region %u: %08X..%08X, %u blocks x %u KiB = %u KiB\n",
			region,
			region_start,
			region_start + region_size - 1,
			flash->erase[region].blocks,
			flash->erase[region].block_size >> 10,
			region_size >> 10
		);
		region_start += region_size;
	}

	printf(
		"# PRI: version=%c.%c features=%08X suspend=%02X block-status=%04X\n",
		cfi_read_word(flash->base, flash->primary_address + 3),
		cfi_read_word(flash->base, flash->primary_address + 4),
		features,
		cfi_read_word(flash->base, flash->primary_address + 9),
		cfi_read_u16(flash->base, flash->primary_address + 0xA)
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
		cfi_read_word(flash->base, flash->primary_address + 0xC) >> 4,
		cfi_read_word(flash->base, flash->primary_address + 0xC) & 0xF,
		cfi_read_word(flash->base, flash->primary_address + 0xD) >> 4,
		cfi_read_word(flash->base, flash->primary_address + 0xD) & 0xF
	);
	print_pri_information(flash, features);
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
	for (uint32_t region = 0; region < flash->regions; region++) {
		for (uint32_t block = 0; block < flash->erase[region].blocks; block++) {
			offset += flash->erase[region].block_size;
			if (offset >= flash->size) {
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
	bool simultaneous_operations = (cfi_read_u32(flash->base, flash->primary_address + 5) & BIT(9)) != 0;
	cfi_enter_read_array(flash->base);
	uint32_t partition_size = find_command_partition_size(flash);
	test_eq_u32("command partitions match PRI simultaneous operations", simultaneous_operations, partition_size != 0);
	if (!partition_size || partition_size * 2 > flash->size) {
		return;
	}

	uint32_t first = flash->base;
	uint32_t second = flash->base + partition_size;
	uint16_t first_array[MAP_SAMPLE_WORDS];
	uint16_t second_array[MAP_SAMPLE_WORDS];
	memcpy(first_array, (const void *) first, sizeof(first_array));
	memcpy(second_array, (const void *) second, sizeof(second_array));
	printf("# command partitions: first=%08X second=%08X size=%u KiB\n", first, second, partition_size >> 10);

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
	cfi_enter_query(flash->base);
	cfi_enter_read_array(flash->base);
	test_eq_memory("read array restored after query mode", array_data, (const void *) flash->base, sizeof(array_data));
}

static void test_flash_mapping(const struct flash_device *flash) {
	uint32_t expected_addrsel = (
		EBU_ADDRSEL_REGENAB |
		((27 - flash->size_exponent) << EBU_ADDRSEL_MASK_SHIFT) |
		flash->base
	);

	test_eq_u32("EBU address window", expected_addrsel, EBU_ADDRSEL(flash->cs));
	test_eq_memory(
		"flash start is mapped",
		flash->map_samples[0],
		(const void *) flash->base,
		sizeof(flash->map_samples[0])
	);
	test_eq_memory(
		"flash middle is mapped",
		flash->map_samples[1],
		(const void *) (flash->base + flash->size / 2),
		sizeof(flash->map_samples[1])
	);
	test_eq_memory(
		"flash end is mapped",
		flash->map_samples[2],
		(const void *) (flash->base + flash->size - sizeof(flash->map_samples[2])),
		sizeof(flash->map_samples[2])
	);
}

int main(void) {
	test_start("Intel/ST CFI flash test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Scan");
	scan_flashes();
	test_check("at least one CFI flash found", flash_count > 0);
	bool flash_map_valid = configure_flash_map();
	test_check("all flash windows fit EBU address space", flash_map_valid);
	if (!flash_map_valid) {
		return test_finish();
	}
	test_category("Flash device command modes");
	test_flash_device_modes();

	for (uint32_t i = 0; i < flash_count; i++) {
		printf("# testing CS%u at %08X\n", flashes[i].cs, flashes[i].base);
		test_category("EBU mapping");
		test_flash_mapping(&flashes[i]);
		test_category("Identification");
		test_identification(&flashes[i]);
		test_category("Electronic signature");
		test_electronic_signature(&flashes[i]);
		test_category("Erase block locks");
		test_block_lock_status(&flashes[i]);
		test_category("OTP");
		test_otp(&flashes[i]);
		test_category("Extended flash array");
		test_efa(&flashes[i]);
		test_category("Status register");
		test_status_register(&flashes[i]);
		test_category("Blank check");
		test_blank_check(&flashes[i]);
		test_category("Blank space");
		test_blank_space(&flashes[i]);
		test_category("CFI query and geometry");
		test_cfi_query(&flashes[i]);
		print_cfi_information(&flashes[i]);
		test_category("Command partitions");
		test_partition_modes(&flashes[i]);
		test_category("Read array recovery");
		test_read_array_recovery(&flashes[i]);
	}
	test_category("Write test area");
	test_check("at least one aligned blank area is available", write_test_area_found);

	return test_finish();
}
