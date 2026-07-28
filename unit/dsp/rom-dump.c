#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_PREAD 3
#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_RESULT_OFFSET (DSP_BOOT_DATA_OFFSET + 3)
#define DSP_BOOT_MAX_WORDS 509
#define IHEX_RECORD_WORDS 8
#define DSP_BOOT_BLOCK_WORDS (DSP_BOOT_MAX_WORDS / IHEX_RECORD_WORDS * IHEX_RECORD_WORDS)
#define DSP_PAGE_ADDRESS 0xDEA3
#define DSP_PROGRAM_ROM_FIXED_FIRST 0x2000
#define DSP_PROGRAM_ROM_FIXED_WORDS 0x8000
#define DSP_PROGRAM_ROM_WINDOW_FIRST 0xA000
#define DSP_PROGRAM_ROM_WINDOW_WORDS 0x6000
#define DSP_PROGRAM_PAGE_SHIFT 2
#define DSP_PROGRAM_PAGE_COUNT 3
#define DSP_DATA_ROM_FIXED_FIRST 0x8000
#define DSP_DATA_ROM_FIXED_WORDS 0x1000
#define DSP_DATA_ROM_WINDOW_FIRST 0x9000
#define DSP_DATA_ROM_WINDOW_WORDS 0x4000
#define DSP_DATA_PAGE_COUNT 4
#define DSP_WAIT_ITERATIONS 1000000

static volatile uint16_t *const dsp_shared_memory = (volatile uint16_t *) DSP_RAM_BASE;

static bool wait_for_boot_ready(void) {
	for (unsigned int i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool submit_boot_command(void) {
	DSP_COM_SET = DSP_COM_SET | BIT(0);
	SCU_DSP_INT = SCU_DSP_INT | BIT(0);
	SCU_DSP_INT = SCU_DSP_INT & ~BIT(0);

	return wait_for_boot_ready();
}

static bool dsp_read(uint16_t command, uint16_t source, uint16_t words) {
	volatile uint16_t *boot_data = dsp_shared_memory + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = source;
	boot_data[2] = words;

	return submit_boot_command();
}

static bool dsp_dload_word(uint16_t destination, uint16_t value) {
	volatile uint16_t *boot_data = dsp_shared_memory + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_DLOAD;
	boot_data[1] = destination;
	boot_data[2] = 1;
	boot_data[3] = value;

	return submit_boot_command();
}

static uint8_t ihex_checksum(uint32_t sum) {
	return (uint8_t) -sum;
}

static void print_ihex_extended_address(uint16_t upper_address) {
	uint32_t sum = 2 + 4 + (upper_address >> 8) + (upper_address & 0xFF);

	printf(":02000004%04X%02X\n", upper_address, ihex_checksum(sum));
}

static void print_ihex_data(uint32_t address, const volatile uint16_t *words, unsigned int word_count,
	uint16_t *current_upper_address) {
	uint16_t upper_address = address >> 16;
	uint16_t lower_address = address;
	unsigned int byte_count = word_count * sizeof(uint16_t);
	uint32_t sum = byte_count + (lower_address >> 8) + (lower_address & 0xFF);

	if (upper_address != *current_upper_address) {
		print_ihex_extended_address(upper_address);
		*current_upper_address = upper_address;
	}

	printf(":%02X%04X00", byte_count, lower_address);
	for (unsigned int i = 0; i < word_count; i++) {
		uint8_t low = words[i];
		uint8_t high = words[i] >> 8;

		printf("%02X%02X", low, high);
		sum += low + high;
	}
	printf("%02X\n", ihex_checksum(sum));
}

static bool dump_region(uint16_t command, uint16_t first, uint32_t words, uint32_t output_first,
	uint32_t *hash, uint16_t *current_upper_address) {
	volatile uint16_t *result = dsp_shared_memory + DSP_BOOT_RESULT_OFFSET;
	uint32_t value_hash = 0x811C9DC5U;

	for (uint32_t source = first; source < first + words;) {
		uint32_t remaining = first + words - source;
		uint16_t block_words = remaining < DSP_BOOT_BLOCK_WORDS ? remaining : DSP_BOOT_BLOCK_WORDS;

		if (!dsp_read(command, source, block_words))
			return false;
		test_watchdog_reset();

		for (unsigned int i = 0; i < block_words; i += IHEX_RECORD_WORDS) {
			unsigned int record_words = block_words - i;

			if (record_words > IHEX_RECORD_WORDS)
				record_words = IHEX_RECORD_WORDS;
			print_ihex_data((output_first + source + i - first) * sizeof(uint16_t), result + i, record_words,
				current_upper_address);
			for (unsigned int j = 0; j < record_words; j++)
				value_hash = (value_hash ^ result[i + j]) * 0x01000193U;
			test_watchdog_serve();
		}
		source += block_words;
	}

	*hash = value_hash;
	return true;
}

int main(void) {
#ifdef DSP_DUMP_DATA_ROM
	test_start("DSP data mask ROM dump");
#else
	test_start("DSP program mask ROM dump");
#endif
	if (test_is_qemu()) {
		test_skip("mask ROM dump", "QEMU does not execute DSP firmware");
		return test_finish();
	}

	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	SCU_RST_REQ = 0;
	if (!test_check("DSP boot firmware becomes ready", wait_for_boot_ready()))
		return test_finish();

	printf("# DSP mask ID: %04X\n", dsp_shared_memory[0]);

#ifdef DSP_DUMP_DATA_ROM
	printf("# Convert: rg -a '^:' /tmp/pmb887x-dsp-data-mask-rom.log > /tmp/pmb887x-dsp-data-mask-rom.hex && "
		"objcopy -I ihex -O binary /tmp/pmb887x-dsp-data-mask-rom.hex /tmp/pmb887x-dsp-data-mask-rom.bin\n");
	test_category("Data space dump");
	uint16_t current_upper_address = UINT16_MAX;
	uint32_t fixed_hash;
	uint32_t page_hashes[DSP_DATA_PAGE_COUNT];

	if (!test_check("DLOAD selects data page 0", dsp_dload_word(DSP_PAGE_ADDRESS, 0)))
		return test_finish();
	bool fixed_dumped = dump_region(DSP_BOOT_DREAD, DSP_DATA_ROM_FIXED_FIRST, DSP_DATA_ROM_FIXED_WORDS, 0,
		&fixed_hash, &current_upper_address);
	if (!test_check("DREAD dumps fixed data ROM", fixed_dumped)) {
		printf(":00000001FF\n");
		return test_finish();
	}
	printf("# DSP fixed data ROM word-wise FNV-1a: %08X\n", fixed_hash);

	for (unsigned int page = 0; page < DSP_DATA_PAGE_COUNT; page++) {
		if (!test_check("DLOAD selects data ROM bank", dsp_dload_word(DSP_PAGE_ADDRESS, page))) {
			printf(":00000001FF\n");
			return test_finish();
		}
		uint32_t output_first = DSP_DATA_ROM_FIXED_WORDS + page * DSP_DATA_ROM_WINDOW_WORDS;
		bool page_dumped = dump_region(DSP_BOOT_DREAD, DSP_DATA_ROM_WINDOW_FIRST, DSP_DATA_ROM_WINDOW_WORDS,
			output_first, &page_hashes[page], &current_upper_address);
		if (!test_check("DREAD dumps banked data ROM", page_dumped)) {
			printf(":00000001FF\n");
			return test_finish();
		}
		printf("# DSP data ROM bank %u word-wise FNV-1a: %08X\n", page, page_hashes[page]);
	}
#else
	printf("# Convert: rg -a '^:' /tmp/pmb887x-dsp-program-mask-rom.log > /tmp/pmb887x-dsp-program-mask-rom.hex && "
		"objcopy -I ihex -O binary /tmp/pmb887x-dsp-program-mask-rom.hex "
		"/tmp/pmb887x-dsp-program-mask-rom.bin\n");
	test_category("Program space dump");
	uint16_t current_upper_address = UINT16_MAX;
	uint32_t fixed_hash;
	uint32_t page_hashes[DSP_PROGRAM_PAGE_COUNT];

	if (!test_check("DLOAD selects program page 0", dsp_dload_word(DSP_PAGE_ADDRESS, 0)))
		return test_finish();
	bool fixed_dumped = dump_region(DSP_BOOT_PREAD, DSP_PROGRAM_ROM_FIXED_FIRST, DSP_PROGRAM_ROM_FIXED_WORDS,
		0, &fixed_hash, &current_upper_address);
	if (!test_check("PREAD dumps fixed program ROM", fixed_dumped)) {
		printf(":00000001FF\n");
		return test_finish();
	}
	printf("# DSP fixed program ROM word-wise FNV-1a: %08X\n", fixed_hash);

	for (unsigned int page = 0; page < DSP_PROGRAM_PAGE_COUNT; page++) {
		if (!test_check("DLOAD selects program ROM bank", dsp_dload_word(DSP_PAGE_ADDRESS,
			page << DSP_PROGRAM_PAGE_SHIFT))) {
			printf(":00000001FF\n");
			return test_finish();
		}
		uint32_t output_first = DSP_PROGRAM_ROM_FIXED_WORDS + page * DSP_PROGRAM_ROM_WINDOW_WORDS;
		bool page_dumped = dump_region(DSP_BOOT_PREAD, DSP_PROGRAM_ROM_WINDOW_FIRST,
			DSP_PROGRAM_ROM_WINDOW_WORDS, output_first, &page_hashes[page], &current_upper_address);
		if (!test_check("PREAD dumps banked program ROM", page_dumped)) {
			printf(":00000001FF\n");
			return test_finish();
		}
		printf("# DSP program ROM bank %u word-wise FNV-1a: %08X\n", page, page_hashes[page]);
	}
	bool banks_distinct = page_hashes[0] != page_hashes[1] && page_hashes[0] != page_hashes[2] &&
		page_hashes[1] != page_hashes[2];
	test_check("program ROM banks are distinct", banks_distinct);
#endif
	printf(":00000001FF\n");

	return test_finish();
}
