#include <pmb887x.h>

#include "cfi.h"
#include "test.h"

#define TRACE_ABORT_X32_WRITES 112
#define OPERATION_TIMEOUT_MS 30000
#define STATUS_ABORT_ERRORS (BIT(7) | BIT(5) | BIT(4))
#define STATUS_LOW_ERRORS (BIT(5) | BIT(4) | BIT(3) | BIT(1))

typedef struct {
	uint32_t base;
	uint32_t size;
} flash_block_t;

static uint32_t crc32_table[256];
static bool crc32_table_ready;

static int skip_destructive_test(const char *reason) {
	test_skip("CFI buffered program abort", reason);
	return test_finish();
}

static void print_status(const char *name, uint16_t status) {
	uint8_t raw = status;
	printf(
		"# %s: raw=%02X SR7..SR0=%u%u%u%u%u%u%u%u\n",
		name,
		raw,
		(raw >> 7) & 1,
		(raw >> 6) & 1,
		(raw >> 5) & 1,
		(raw >> 4) & 1,
		(raw >> 3) & 1,
		(raw >> 2) & 1,
		(raw >> 1) & 1,
		raw & 1
	);
}

static uint32_t crc32_block(uint32_t address, uint32_t size) {
	if (!crc32_table_ready) {
		for (uint32_t index = 0; index < ARRAY_SIZE(crc32_table); index++) {
			uint32_t value = index;
			for (uint32_t bit = 0; bit < 8; bit++)
				value = (value >> 1) ^ (0xEDB88320 & (0U - (value & 1)));
			crc32_table[index] = value;
		}
		crc32_table_ready = true;
	}
	wdt_set_max_execution_time(OPERATION_TIMEOUT_MS);
	uint32_t crc = UINT32_MAX;

	for (uint32_t offset = 0; offset < size; offset += sizeof(uint32_t)) {
		uint32_t value = MMIO32(address + offset);
		for (uint32_t byte = 0; byte < sizeof(value); byte++) {
			crc = crc32_table[(crc ^ value) & UINT8_MAX] ^ (crc >> 8);
			value >>= 8;
		}
		if ((offset & 0xFF) == 0)
			test_watchdog_serve();
	}

	test_watchdog_reset();
	return ~crc;
}

static bool range_is_blank(uint32_t address, uint32_t size) {
	wdt_set_max_execution_time(0);
	for (uint32_t offset = 0; offset < size; offset += sizeof(uint32_t)) {
		if (MMIO32(address + offset) != UINT32_MAX) {
			test_watchdog_reset();
			return false;
		}
		test_watchdog_serve();
	}
	test_watchdog_reset();
	return true;
}

static bool find_block(const struct flash_device *flash, uint32_t address, flash_block_t *result) {
	if (address < flash->base || address - flash->base >= flash->size)
		return false;

	uint32_t region_base = flash->base;
	for (uint32_t region = 0; region < flash->regions; region++) {
		uint32_t block_size = flash->erase[region].block_size;
		uint32_t region_size = flash->erase[region].blocks * block_size;
		if (block_size == 0 || region_size / block_size != flash->erase[region].blocks)
			return false;
		if (address - flash->base < region_base - flash->base + region_size) {
			result->base = region_base + (address - region_base) / block_size * block_size;
			result->size = block_size;
			return true;
		}
		region_base += region_size;
	}

	return false;
}

static uint32_t select_setup_offset(const struct flash_device *flash, const flash_block_t *block) {
	return (block->size / 2) & ~(flash->write_buffer_size - 1);
}

static bool block_is_usable(const struct flash_device *flash, const flash_block_t *block,
	flash_block_t *neighbor) {
	if (block->size < flash->write_buffer_size || (block->size & (block->size - 1)) != 0)
		return false;

	uint32_t setup_offset = select_setup_offset(flash, block);
	if (setup_offset + flash->write_buffer_size > block->size)
		return false;
	uint32_t setup_address = block->base + setup_offset;
	if ((setup_address & (flash->write_buffer_size - 1)) != 0)
		return false;

	uint32_t abort_address = setup_address ^ block->size;
	if (!find_block(flash, abort_address, neighbor))
		return false;
	if (neighbor->base == block->base || neighbor->size != block->size ||
		abort_address - neighbor->base != setup_offset)
		return false;
	if ((cfi_read_lock_status(block->base) & BIT(1)) != 0)
		return false;

	cfi_enter_read_array(block->base);
	return cfi_range_is_blank(block->base, block->size);
}

static bool find_blank_test_block(struct flash_device *flash, flash_block_t *block,
	flash_block_t *neighbor) {
	for (uint32_t chip = 0; chip < FLASH_CHIP_SELECT_COUNT; chip++) {
		struct flash_device candidate = {0};
		if (!cfi_probe(cfi_chip_selects[chip], &candidate))
			continue;
		if (candidate.size_exponent < 20 || candidate.size_exponent > 27 ||
			candidate.write_buffer_size < 2 * sizeof(uint32_t) ||
			(candidate.write_buffer_size & (candidate.write_buffer_size - 1)) != 0)
			continue;
		cfi_map(candidate.cs, candidate.base, 27 - candidate.size_exponent);

		printf(
			"# probing CS%u vendor/device=%04X:%04X buffer=%u bytes\n",
			(unsigned int) candidate.cs,
			(unsigned int) candidate.manufacturer,
			(unsigned int) candidate.device,
			(unsigned int) candidate.write_buffer_size
		);
		uint32_t address = candidate.base;
		for (uint32_t region = 0; region < candidate.regions; region++) {
			uint32_t block_size = candidate.erase[region].block_size;
			for (uint32_t index = 0; index < candidate.erase[region].blocks; index++) {
				flash_block_t current = {
					.base = address,
					.size = block_size,
				};
				test_watchdog_reset();
				if (block_is_usable(&candidate, &current, neighbor)) {
					*flash = candidate;
					*block = current;
					return true;
				}
				address += block_size;
				test_watchdog_serve();
			}
		}
	}

	return false;
}

static uint32_t program_value(uint32_t index) {
	return 0x13570000 + index;
}

static bool buffered_program_command(const struct flash_device *flash, uint16_t *command) {
	cfi_enter_query(flash->base);
	uint16_t primary_command_set = cfi_read_u16(flash->base, 0x13);
	cfi_enter_read_array(flash->base);
	if (primary_command_set == 0x0001)
		*command = 0xE8;
	else if (primary_command_set == 0x0200)
		*command = 0xE9;
	else
		return false;
	return true;
}

static uint32_t block_erase_max_ms(const struct flash_device *flash) {
	cfi_enter_query(flash->base);
	uint32_t typical_exponent = cfi_read_word(flash->base, 0x21);
	uint32_t maximum_exponent = cfi_read_word(flash->base, 0x25);
	cfi_enter_read_array(flash->base);
	uint32_t total_exponent = typical_exponent + maximum_exponent;
	return typical_exponent != 0 && total_exponent < 15 ? 1u << total_exponent : OPERATION_TIMEOUT_MS;
}

static bool emergency_cleanup(const struct flash_device *flash, const flash_block_t *block) {
	uint32_t cfi_max_ms = block_erase_max_ms(flash);
	uint32_t delay_ms = cfi_max_ms <= (OPERATION_TIMEOUT_MS - 1000) / 4 ?
		cfi_max_ms * 4 + 1000 : OPERATION_TIMEOUT_MS;
	printf("# emergency cleanup delay: %u ms (CFI max %u ms)\n", (unsigned int) delay_ms,
		(unsigned int) cfi_max_ms);
	wdt_set_max_execution_time(OPERATION_TIMEOUT_MS);
	MMIO16(block->base) = 0x50;
	MMIO16(block->base) = 0x20;
	MMIO16(block->base) = 0xD0;
	stopwatch_t started = stopwatch_get();
	while (stopwatch_elapsed_ms(started) < delay_ms)
		test_watchdog_serve();
	MMIO16(block->base) = 0x70;
	uint16_t status = MMIO16(block->base);
	if ((status & STATUS_READY) != 0)
		cfi_enter_read_array(block->base);
	test_watchdog_reset();
	print_status("emergency cleanup status", status);
	if ((status & (STATUS_READY | STATUS_LOW_ERRORS)) != STATUS_READY)
		return false;
	return range_is_blank(block->base, block->size);
}

static void test_abort_trace(const struct flash_device *flash, uint16_t command, uint32_t abort_writes,
	uint32_t setup_address, const flash_block_t *block) {
	uint32_t abort_address = setup_address ^ block->size;
	MMIO16(setup_address) = 0x50;
	MMIO16(setup_address) = 0xFF;
	MMIO16(setup_address) = command;
	uint16_t status_before_abort = cfi_wait_ready(setup_address);
	print_status("status before abort", status_before_abort);
	bool accepted = (status_before_abort & (STATUS_READY | STATUS_LOW_ERRORS)) == STATUS_READY;
	test_check("abort trace accepts the buffer setup command", accepted);
	if (!accepted) {
		MMIO16(setup_address) = 0x50;
		MMIO16(setup_address) = 0xFF;
		return;
	}

	MMIO16(setup_address) = flash->write_buffer_size / sizeof(uint16_t) - 1;
	for (uint32_t index = 0; index < abort_writes; index++)
		MMIO32(setup_address + index * sizeof(uint32_t)) = program_value(index);
	MMIO16(abort_address) = 0xFFFF;
	uint16_t status_after_abort = MMIO16(setup_address);
	MMIO16(setup_address) = 0x50;
	uint16_t status_after_clear = MMIO16(setup_address);

	MMIO16(setup_address) = 0x50;
	MMIO16(setup_address) = 0x70;
	MMIO16(setup_address) = 0xFF;
	uint32_t array_after_ff = MMIO32(setup_address);
	MMIO16(setup_address) = 0x70;
	uint16_t status_after_ff = MMIO16(setup_address);
	MMIO16(setup_address) = 0x70;
	MMIO16(setup_address) = 0xFF;

	print_status("status immediately after abort", status_after_abort);
	print_status("status after 50", status_after_clear);
	print_status("status after FF then 70", status_after_ff);
	test_check(
		"out-of-block write reports SR7, SR5 and SR4",
		(status_after_abort & STATUS_ABORT_ERRORS) == STATUS_ABORT_ERRORS
	);
	test_eq_u32("50 clears buffered-program error bits", 0, status_after_clear & STATUS_LOW_ERRORS);
	test_eq_u32("FF returns the device to read-array mode", UINT32_MAX, array_after_ff);
	test_check(
		"status is clean after FF and 70",
		(status_after_ff & (STATUS_READY | STATUS_LOW_ERRORS)) == STATUS_READY
	);

	MMIO16(setup_address) = command;
	uint16_t retry_status = cfi_wait_ready(setup_address);
	print_status("retry buffered-program setup", retry_status);
	bool retry_accepted = (retry_status & (STATUS_READY | STATUS_LOW_ERRORS)) == STATUS_READY;
	test_check("flash accepts a buffered-program command after abort recovery", retry_accepted);
	if (retry_accepted) {
		MMIO16(setup_address) = flash->write_buffer_size / sizeof(uint16_t) - 1;
		for (uint32_t index = 0; index < abort_writes; index++)
			MMIO32(setup_address + index * sizeof(uint32_t)) = program_value(index);
		MMIO16(abort_address) = 0xFFFF;
		uint16_t retry_abort_status = MMIO16(setup_address);
		print_status("retry command abort", retry_abort_status);
		test_check(
			"retry command can be aborted without programming",
			(retry_abort_status & STATUS_ABORT_ERRORS) == STATUS_ABORT_ERRORS
		);
	}
	MMIO16(setup_address) = 0x50;
	MMIO16(setup_address) = 0xFF;
	MMIO16(abort_address) = 0x50;
	MMIO16(abort_address) = 0xFF;
}

int main(void) {
	test_start("CFI buffered program abort test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	struct flash_device flash = {0};
	flash_block_t test_block;
	flash_block_t neighbor_block;
	if (!find_blank_test_block(&flash, &test_block, &neighbor_block))
		return skip_destructive_test("no suitable blank non-lock-down erase block was found");

	uint32_t setup_address = test_block.base + select_setup_offset(&flash, &test_block);
	uint32_t abort_address = setup_address ^ test_block.size;
	uint32_t requested_words = flash.write_buffer_size / sizeof(uint16_t);
	uint32_t full_writes = flash.write_buffer_size / sizeof(uint32_t);
	uint32_t abort_writes = full_writes > TRACE_ABORT_X32_WRITES ?
		TRACE_ABORT_X32_WRITES : full_writes - 1;
	uint16_t command;
	if (!buffered_program_command(&flash, &command))
		return skip_destructive_test("unsupported CFI primary command set");
	printf("# vendor/device ID: %04X:%04X\n", flash.manufacturer, flash.device);
	printf("# CFI buffer size: %u bytes\n", (unsigned int) flash.write_buffer_size);
	printf("# erase block size: %u bytes\n", (unsigned int) test_block.size);
	printf("# buffered-program command: %02X\n", command);
	printf("# EBU: CLC=%08X ADDRSEL=%08X BUSCON=%08X\n", (unsigned int) EBU_CLC,
		(unsigned int) EBU_ADDRSEL(flash.cs), (unsigned int) EBU_BUSCON(flash.cs));
	printf("# setup address: %08X\n", (unsigned int) setup_address);
	printf("# abort address: %08X\n", (unsigned int) abort_address);
	printf("# x16 words requested: %u\n", (unsigned int) requested_words);
	printf("# abort x32 writes performed: %u\n", (unsigned int) abort_writes);

	test_category("Safety checks");
	uint16_t initial_lock = cfi_read_lock_status(test_block.base) & 0x3;
	uint16_t neighbor_lock = cfi_read_lock_status(neighbor_block.base) & 0x3;
	cfi_enter_read_array(test_block.base);
	uint32_t test_crc = crc32_block(test_block.base, test_block.size);
	uint32_t neighbor_crc = crc32_block(neighbor_block.base, neighbor_block.size);
	printf("# CRC before: test=%08X neighbor=%08X\n", test_crc, neighbor_crc);
	test_check("setup buffer is blank before abort trace",
		range_is_blank(setup_address, flash.write_buffer_size));
	test_eq_u32("abort word is erased before abort trace", UINT16_MAX, MMIO16(abort_address));

	uint16_t unlock_status = cfi_unlock_block(test_block.base, false);
	print_status("block unlock status", unlock_status);
	bool unlocked = (unlock_status & (STATUS_READY | STATUS_LOW_ERRORS)) == STATUS_READY &&
		(cfi_read_lock_status(test_block.base) & 0x3) == 0;
	test_check("test block is unlocked before abort trace", unlocked);
	if (!unlocked) {
		if ((initial_lock & BIT(0)) != 0)
			cfi_lock_block(test_block.base, false);
		return skip_destructive_test("test block did not pass pre-abort safety checks");
	}

	test_category("Out-of-block buffered program abort");
	test_abort_trace(&flash, command, abort_writes, setup_address, &test_block);

	cfi_enter_read_array(test_block.base);
	cfi_enter_read_array(neighbor_block.base);
	bool test_preserved = test_check(
		"unconfirmed buffered data is not programmed",
		range_is_blank(setup_address, flash.write_buffer_size)
	);
	test_eq_u32("abort write preserves its target word", UINT16_MAX, MMIO16(abort_address));
	if (!test_preserved) {
		test_category("Emergency cleanup");
		test_check("emergency erase leaves the test block blank",
			emergency_cleanup(&flash, &test_block));
	}

	MMIO16(setup_address) = 0x50;
	MMIO16(setup_address) = 0xFF;
	uint16_t restore_status = initial_lock ? cfi_lock_block(test_block.base, false) :
		cfi_unlock_block(test_block.base, false);
	print_status("lock restore status", restore_status);
	test_check(
		"lock-state restore completes without errors",
		(restore_status & (STATUS_READY | STATUS_LOW_ERRORS)) == STATUS_READY
	);
	test_eq_u32("test block lock state is restored", initial_lock,
		cfi_read_lock_status(test_block.base) & 0x3);
	test_eq_u32("neighbor block lock state is preserved", neighbor_lock,
		cfi_read_lock_status(neighbor_block.base) & 0x3);
	cfi_enter_read_array(test_block.base);
	cfi_enter_read_array(neighbor_block.base);
	uint32_t test_crc_after = crc32_block(test_block.base, test_block.size);
	uint32_t neighbor_crc_after = crc32_block(neighbor_block.base, neighbor_block.size);
	printf("# CRC after: test=%08X neighbor=%08X\n", test_crc_after, neighbor_crc_after);
	test_eq_u32("test block CRC is preserved", test_crc, test_crc_after);
	test_eq_u32("neighbor block CRC is preserved", neighbor_crc, neighbor_crc_after);
	test_check("cleanup leaves the setup buffer blank",
		range_is_blank(setup_address, flash.write_buffer_size));
	test_eq_u32("cleanup leaves the abort word erased", UINT16_MAX, MMIO16(abort_address));

	return test_finish();
}
