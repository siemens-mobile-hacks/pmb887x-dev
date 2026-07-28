#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_PREAD 3
#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_RESULT_OFFSET (DSP_BOOT_DATA_OFFSET + 3)
#define DSP_BOOT_MAX_WORDS 507
#define IHEX_RECORD_WORDS 8
#define DSP_BOOT_BLOCK_WORDS (DSP_BOOT_MAX_WORDS / IHEX_RECORD_WORDS * IHEX_RECORD_WORDS)
#define DSP_MASK_ID_FAMILY_MASK 0xFF00
#define DSP_MASK_ID_FAMILY_06XX 0x0600
#define DSP_MASK_ID_FAMILY_08XX 0x0800
#define DSP_MAX_PROGRAM_PAGES 3
#define DSP_MAX_DATA_PAGES 4
#define DSP_WAIT_ITERATIONS 1000000

static volatile uint16_t *const dsp_shared_memory = (volatile uint16_t *) DSP_RAM_BASE;

struct dsp_rom_layout {
	uint16_t page_address;
	uint16_t program_fixed_first;
	uint32_t program_fixed_words;
	uint16_t program_window_first;
	uint32_t program_window_words;
	size_t program_page_shift;
	size_t program_page_count;
	uint16_t data_fixed_first;
	uint32_t data_fixed_words;
	uint16_t data_window_first;
	uint32_t data_window_words;
	size_t data_page_count;
};

static const struct dsp_rom_layout ROM_LAYOUT_06XX = {
	.page_address = 0xE6A3,
	.program_fixed_first = 0x1000,
	.program_fixed_words = 0xA000,
	.program_window_first = 0xB000,
	.program_window_words = 0x5000,
	.program_page_shift = 1,
	.program_page_count = 2,
	.data_fixed_first = 0x6000,
	.data_fixed_words = 0x3000,
	.data_window_first = 0x9000,
	.data_window_words = 0x4800,
	.data_page_count = 2,
};

static const struct dsp_rom_layout ROM_LAYOUT_08XX = {
	.page_address = 0xDEA3,
	.program_fixed_first = 0x2000,
	.program_fixed_words = 0x8000,
	.program_window_first = 0xA000,
	.program_window_words = 0x6000,
	.program_page_shift = 2,
	.program_page_count = 3,
	.data_fixed_first = 0x8000,
	.data_fixed_words = 0x1000,
	.data_window_first = 0x9000,
	.data_window_words = 0x4000,
	.data_page_count = 4,
};

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
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

static void print_ihex_data(uint32_t address, const volatile uint16_t *words, size_t word_count,
	uint16_t *current_upper_address) {
	uint16_t upper_address = address >> 16;
	uint16_t lower_address = address;
	uint32_t byte_count = word_count * sizeof(uint16_t);
	uint32_t sum = byte_count + (lower_address >> 8) + (lower_address & 0xFF);

	if (upper_address != *current_upper_address) {
		print_ihex_extended_address(upper_address);
		*current_upper_address = upper_address;
	}

	printf(":%02X%04X00", byte_count, lower_address);
	for (size_t i = 0; i < word_count; i++) {
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

		for (size_t i = 0; i < block_words; i += IHEX_RECORD_WORDS) {
			size_t record_words = block_words - i;

			if (record_words > IHEX_RECORD_WORDS)
				record_words = IHEX_RECORD_WORDS;
			print_ihex_data((output_first + source + i - first) * sizeof(uint16_t), result + i, record_words,
				current_upper_address);
			for (size_t j = 0; j < record_words; j++)
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

	uint16_t mask_id = dsp_shared_memory[0];
	uint16_t mask_family = mask_id & DSP_MASK_ID_FAMILY_MASK;
	printf("# DSP mask ID: %04X\n", (uint32_t) mask_id);
	if (!test_check("DSP Mask ROM ID is supported",
		mask_family == DSP_MASK_ID_FAMILY_06XX || mask_family == DSP_MASK_ID_FAMILY_08XX))
		return test_finish();
	const struct dsp_rom_layout *layout = mask_family == DSP_MASK_ID_FAMILY_06XX ?
		&ROM_LAYOUT_06XX : &ROM_LAYOUT_08XX;

#ifdef DSP_DUMP_DATA_ROM
	printf("# Convert: rg -a '^:' /tmp/dsp-%04X-data-rom.log > /tmp/dsp-%04X-data-rom.hex && "
		"objcopy -I ihex -O binary /tmp/dsp-%04X-data-rom.hex ../rom/dsp/%04X-data-rom.bin\n",
		(uint32_t) mask_id, (uint32_t) mask_id, (uint32_t) mask_id, (uint32_t) mask_id);
	test_category("Data space dump");
	uint16_t current_upper_address = UINT16_MAX;
	uint32_t fixed_hash;
	uint32_t page_hashes[DSP_MAX_DATA_PAGES];

	if (!test_check("DLOAD selects data page 0", dsp_dload_word(layout->page_address, 0)))
		return test_finish();
	bool fixed_dumped = dump_region(DSP_BOOT_DREAD, layout->data_fixed_first, layout->data_fixed_words, 0,
		&fixed_hash, &current_upper_address);
	if (!test_check("DREAD dumps fixed data ROM", fixed_dumped)) {
		printf(":00000001FF\n");
		return test_finish();
	}
	printf("# DSP fixed data ROM word-wise FNV-1a: %08X\n", fixed_hash);

	for (size_t page = 0; page < layout->data_page_count; page++) {
		if (!test_check("DLOAD selects data ROM bank", dsp_dload_word(layout->page_address, page))) {
			printf(":00000001FF\n");
			return test_finish();
		}
		uint32_t output_first = layout->data_fixed_words + page * layout->data_window_words;
		bool page_dumped = dump_region(DSP_BOOT_DREAD, layout->data_window_first, layout->data_window_words,
			output_first, &page_hashes[page], &current_upper_address);
		if (!test_check("DREAD dumps banked data ROM", page_dumped)) {
			printf(":00000001FF\n");
			return test_finish();
		}
		printf("# DSP data ROM bank %u word-wise FNV-1a: %08X\n", (uint32_t) page, page_hashes[page]);
	}
#else
	printf("# Convert: rg -a '^:' /tmp/dsp-%04X-program-rom.log > /tmp/dsp-%04X-program-rom.hex && "
		"objcopy -I ihex -O binary /tmp/dsp-%04X-program-rom.hex ../rom/dsp/%04X-program-rom.bin\n",
		(uint32_t) mask_id, (uint32_t) mask_id, (uint32_t) mask_id, (uint32_t) mask_id);
	test_category("Program space dump");
	uint16_t current_upper_address = UINT16_MAX;
	uint32_t fixed_hash;
	uint32_t page_hashes[DSP_MAX_PROGRAM_PAGES];

	if (!test_check("DLOAD selects program page 0", dsp_dload_word(layout->page_address, 0)))
		return test_finish();
	bool fixed_dumped = dump_region(DSP_BOOT_PREAD, layout->program_fixed_first, layout->program_fixed_words,
		0, &fixed_hash, &current_upper_address);
	if (!test_check("PREAD dumps fixed program ROM", fixed_dumped)) {
		printf(":00000001FF\n");
		return test_finish();
	}
	printf("# DSP fixed program ROM word-wise FNV-1a: %08X\n", fixed_hash);

	for (size_t page = 0; page < layout->program_page_count; page++) {
		if (!test_check("DLOAD selects program ROM bank", dsp_dload_word(layout->page_address,
			page << layout->program_page_shift))) {
			printf(":00000001FF\n");
			return test_finish();
		}
		uint32_t output_first = layout->program_fixed_words + page * layout->program_window_words;
		bool page_dumped = dump_region(DSP_BOOT_PREAD, layout->program_window_first,
			layout->program_window_words, output_first, &page_hashes[page], &current_upper_address);
		if (!test_check("PREAD dumps banked program ROM", page_dumped)) {
			printf(":00000001FF\n");
			return test_finish();
		}
		printf("# DSP program ROM bank %u word-wise FNV-1a: %08X\n", (uint32_t) page, page_hashes[page]);
	}
	bool banks_distinct = page_hashes[0] != page_hashes[1];
	if (layout->program_page_count == 3)
		banks_distinct = banks_distinct && page_hashes[0] != page_hashes[2] && page_hashes[1] != page_hashes[2];
	test_check("program ROM banks are distinct", banks_distinct);
#endif
	printf(":00000001FF\n");

	return test_finish();
}
