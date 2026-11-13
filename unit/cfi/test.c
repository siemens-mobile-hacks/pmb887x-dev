#include <pmb887x.h>

#include <string.h>

#include "cfi/cfi.h"
#include "test.h"

#define QUERY_EXIT_WRITE 0x00FF
#define ARRAY_SAMPLE_WORDS 4
#define CFI_VCC_MIN 0x1B
#define CFI_TYPICAL_TIMEOUTS 0x1F
#define CFI_TIMEOUT_FIELDS 8
#define CFI_INTERFACE 0x28
#define PRI_SIGNATURE_WORDS 3

static void print_bcd_voltage(const char *name, uint32_t base, uint32_t offset) {
	printf(
		" %s=%X.%X..%X.%X V",
		name,
		cfi_read_word(base, offset) >> 4,
		cfi_read_word(base, offset) & 0xF,
		cfi_read_word(base, offset + 1) >> 4,
		cfi_read_word(base, offset + 1) & 0xF
	);
}

static void print_query(uint32_t base, const struct cfi_geometry *geometry) {
	printf(
		"# CFI: command set %04lX, PRI at %04lX, interface %04lX, %lu MiB, write buffer %lu bytes\n",
		geometry->command_set,
		geometry->primary_address,
		cfi_read_u16(base, CFI_INTERFACE),
		geometry->size >> 20,
		geometry->write_buffer_size
	);
	printf("# voltage:");
	print_bcd_voltage("VCC", base, CFI_VCC_MIN);
	print_bcd_voltage("VPP", base, CFI_VCC_MIN + 2);
	printf("\n# timeout exponents (typical word/buffer/erase/chip, max factors):");
	for (uint32_t field = 0; field < CFI_TIMEOUT_FIELDS; field++)
		printf(" %u", cfi_read_word(base, CFI_TYPICAL_TIMEOUTS + field));
	printf("\n");

	uint32_t region_start = 0;
	for (uint32_t region = 0; region < geometry->regions && region < CFI_MAX_ERASE_REGIONS; region++) {
		const struct erase_region *erase = &geometry->erase[region];
		uint32_t region_size = erase->blocks * erase->block_size;
		printf(
			"# erase region %lu: %08lX..%08lX, %lu x %lu KiB\n",
			region,
			region_start,
			region_start + region_size - 1,
			erase->blocks,
			erase->block_size >> 10
		);
		region_start += region_size;
	}
}

static void test_geometry(uint32_t base, const struct cfi_geometry *geometry) {
	test_check("primary command set is present", geometry->command_set != 0);
	test_check("primary table address is valid", geometry->primary_address >= 0x31 && geometry->primary_address < 0x1000);
	char signature[PRI_SIGNATURE_WORDS];
	for (uint32_t i = 0; i < PRI_SIGNATURE_WORDS; i++)
		signature[i] = cfi_read_word(base, geometry->primary_address + i);
	test_check("primary table signature is PRI", memcmp(signature, "PRI", PRI_SIGNATURE_WORDS) == 0);
	test_check("device size is valid", geometry->size_exponent >= 20 && geometry->size_exponent <= 27);
	test_check("erase region count is valid", geometry->regions > 0 && geometry->regions <= CFI_MAX_ERASE_REGIONS);

	uint32_t covered = 0;
	for (uint32_t region = 0; region < geometry->regions && region < CFI_MAX_ERASE_REGIONS; region++) {
		const struct erase_region *erase = &geometry->erase[region];
		bool valid_block_size = erase->block_size >= 256 && (erase->block_size & (erase->block_size - 1)) == 0;
		test_check("erase region block size is valid", erase->blocks > 0 && valid_block_size);
		covered += erase->blocks * erase->block_size;
	}
	test_eq_u32("erase regions cover device", geometry->size, covered);
}

static bool test_chip_select(uint32_t cs) {
	uint32_t base = FLASH_WINDOW_BASE;
	cfi_disable_chip_selects();
	cfi_map(cs, base, 0);
	MMIO16(base) = QUERY_EXIT_WRITE;
	uint16_t array[ARRAY_SAMPLE_WORDS];
	memcpy(array, (const void *) base, sizeof(array));

	cfi_enter_query(base);
	if (!cfi_has_query_signature(base)) {
		MMIO16(base) = QUERY_EXIT_WRITE;
		printf("# CS%lu: no CFI flash\n", cs);
		return false;
	}

	printf("# testing CS%lu\n", cs);
	struct cfi_geometry geometry = { 0 };
	cfi_read_geometry(base, &geometry);
	print_query(base, &geometry);
	test_geometry(base, &geometry);
	MMIO16(base) = QUERY_EXIT_WRITE;
	test_eq_memory("FFh leaves CFI query mode", array, (const void *) base, sizeof(array));
	return true;
}

int main(void) {
	test_start("CFI query test");
	EBU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	uint32_t flash_count = 0;
	for (uint32_t i = 0; i < FLASH_CHIP_SELECT_COUNT; i++) {
		if (test_chip_select(cfi_chip_selects[i]))
			flash_count++;
	}
	cfi_disable_chip_selects();
	test_check("at least one CFI flash found", flash_count > 0);

	return test_finish();
}
