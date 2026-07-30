#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_BRANCH 2
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_EXPECTED_MASK_ID 0x0801
#define DSP_STARTUP_ADDRESS 0x0100
#define DSP_DONE_OFFSET 0x0300
#define DSP_DONE_VALUE 0xA55A
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_SEGMENT_ADDRESS_OFFSET 4
#define DSP1_SEGMENT_SIZE_OFFSET 8
#define DSP1_SEGMENT_MEMORY_TYPE_OFFSET 0x0F
#define DSP1_DATA_MEMORY_TYPE 2

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

#ifndef DSP_INSTRUCTION_CASES_INCLUDE
#define DSP_INSTRUCTION_CASES_INCLUDE "instructions/instructions-cases.inc"
#define DSP_INSTRUCTION_GOLDEN_INCLUDE "instructions/instructions-golden-packed.inc"
#define DSP_INSTRUCTION_IMAGES_INCLUDE "instructions/instructions-images-packed.inc"
#define DSP_INSTRUCTION_TEST_TITLE "DSP TeakLite I instruction test"
#endif

#include DSP_INSTRUCTION_CASES_INCLUDE
#include DSP_INSTRUCTION_GOLDEN_INCLUDE
#include DSP_INSTRUCTION_IMAGES_INCLUDE

#if defined(DSP_INSTRUCTION_IMAGES_PACKED) || defined(DSP_INSTRUCTION_GOLDEN_PACKED)
/* Packed blocks end on token boundaries and retain this history between shards. */
typedef struct {
	uint8_t history[UINT16_MAX + 1];
	uint16_t position;
} lzss_decoder_t;

static void unpack_lzss_block(lzss_decoder_t *decoder, const uint8_t *input, uint8_t *output, size_t output_size) {
	size_t output_position = 0;

	while (output_position < output_size) {
		uint8_t control = *input++;
		if ((control & BIT(7)) == 0) {
			size_t length = control + 1;
			for (size_t i = 0; i < length; i++) {
				uint8_t value = *input++;
				output[output_position++] = value;
				decoder->history[decoder->position++] = value;
			}
		} else {
			size_t length = (control & 0x7F) + 3;
			uint16_t distance = input[0] | (uint16_t) input[1] << 8;
			uint16_t source = decoder->position - distance;
			input += 2;
			for (size_t i = 0; i < length; i++) {
				uint8_t value = decoder->history[source++];
				output[output_position++] = value;
				decoder->history[decoder->position++] = value;
			}
		}
	}
}
#endif

#ifdef DSP_INSTRUCTION_IMAGES_PACKED
static lzss_decoder_t DSP_INSTRUCTION_IMAGE_DECODER;
static uint8_t DSP_INSTRUCTION_IMAGE[DSP_INSTRUCTION_IMAGES_MAX_SIZE];

static const uint8_t *unpack_instruction_image(size_t shard) {
	unpack_lzss_block(&DSP_INSTRUCTION_IMAGE_DECODER, DSP_INSTRUCTION_IMAGES_DATA + DSP_INSTRUCTION_IMAGES_OFFSETS[shard],
		DSP_INSTRUCTION_IMAGE, DSP_INSTRUCTION_IMAGES_SIZES[shard]);
	return DSP_INSTRUCTION_IMAGE;
}
#endif

#ifdef DSP_INSTRUCTION_GOLDEN_PACKED
static lzss_decoder_t DSP_INSTRUCTION_GOLDEN_DECODER;
static uint16_t DSP_INSTRUCTION_GOLDEN_SHARD[DSP_INSTRUCTION_GOLDEN_MAX_SIZE / sizeof(uint16_t)];

static const uint16_t *unpack_instruction_golden(size_t shard) {
	unpack_lzss_block(&DSP_INSTRUCTION_GOLDEN_DECODER, DSP_INSTRUCTION_GOLDEN_DATA + DSP_INSTRUCTION_GOLDEN_OFFSETS[shard],
		(uint8_t *) DSP_INSTRUCTION_GOLDEN_SHARD, DSP_INSTRUCTION_GOLDEN_SIZES[shard]);
	return DSP_INSTRUCTION_GOLDEN_SHARD;
}
#endif

#if DSP_INSTRUCTION_GOLDEN_COUNT != 0 && DSP_INSTRUCTION_GOLDEN_COUNT != DSP_INSTRUCTION_CASE_COUNT
#error Hardware golden does not match the generated instruction corpus
#endif

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool reset_dsp(void) {
	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	uint32_t reset_readback = SCU_RST_REQ;
	SCU_RST_REQ = 0;
	(void) reset_readback;

	return wait_for_boot_ready();
}

static bool submit_boot_command(void) {
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	return wait_for_boot_ready();
}

static bool load_words(uint16_t command, uint16_t destination, const uint8_t *data, size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = destination;
	boot_data[2] = (uint16_t) words;
	for (size_t i = 0; i < words; i++)
		boot_data[3 + i] = data[i * 2] | (uint16_t) data[i * 2 + 1] << 8;

	return submit_boot_command();
}

static uint32_t read_le32(const uint8_t *data) {
	return data[0] | (uint32_t) data[1] << 8 | (uint32_t) data[2] << 16 | (uint32_t) data[3] << 24;
}

static bool load_dsp1_image(const uint8_t *image) {
	size_t segments = image[DSP1_SEGMENT_COUNT_OFFSET];

	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = image + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint16_t address = (uint16_t) read_le32(entry + DSP1_SEGMENT_ADDRESS_OFFSET);
		size_t words = read_le32(entry + DSP1_SEGMENT_SIZE_OFFSET) / sizeof(uint16_t);
		uint16_t command = entry[DSP1_SEGMENT_MEMORY_TYPE_OFFSET] == DSP1_DATA_MEMORY_TYPE ? DSP_BOOT_DLOAD : DSP_BOOT_PLOAD;

		if (!load_words(command, address, image + offset, words))
			return false;
	}

	return true;
}

static bool branch_to_test(void) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_BRANCH;
	boot_data[1] = DSP_STARTUP_ADDRESS;

	return submit_boot_command();
}

static bool wait_for_test_done(void) {
	stopwatch_t start = stopwatch_get();

	while (DSP_SHARED_MEMORY[DSP_DONE_OFFSET] != DSP_DONE_VALUE && stopwatch_elapsed_ms(start) < 1000)
		test_watchdog_serve();

	return DSP_SHARED_MEMORY[DSP_DONE_OFFSET] == DSP_DONE_VALUE;
}

static void print_capture(size_t first, size_t count) {
	for (size_t i = 0; i < count; i++) {
		volatile uint16_t *record = DSP_SHARED_MEMORY + DSP_INSTRUCTION_RESULT_OFFSET + i * DSP_INSTRUCTION_RECORD_WORDS;
		size_t case_index = first + i;

		printf("# DSPCASE %u \"%s\"", (uint32_t) case_index, DSP_INSTRUCTION_CASE_NAMES[case_index]);
		for (size_t j = 0; j < DSP_INSTRUCTION_RECORD_WORDS; j++)
			printf(" %04X", (uint32_t) record[j]);
		printf("\n");
	}
}

static void compare_golden(size_t first, size_t count, const uint16_t *golden) {
	for (size_t i = 0; i < count; i++) {
		volatile uint16_t *record = DSP_SHARED_MEMORY + DSP_INSTRUCTION_RESULT_OFFSET + i * DSP_INSTRUCTION_RECORD_WORDS;
		size_t case_index = first + i;

		test_eq_memory(DSP_INSTRUCTION_CASE_NAMES[case_index], golden + i * DSP_INSTRUCTION_RECORD_WORDS, record,
			DSP_INSTRUCTION_RECORD_WORDS * sizeof(uint16_t));
	}
}

int main(void) {
	test_start(DSP_INSTRUCTION_TEST_TITLE);

	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("DSP Mask ROM boot dispatcher becomes ready", reset_dsp()))
		return test_finish();
	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	printf("# DSP mask ID: %04X\n", (uint32_t) mask_id);
	if (!test_eq_u32("EL71 DSP Mask ROM is 0801", DSP_EXPECTED_MASK_ID, mask_id))
		return test_finish();

	bool compare_results = DSP_INSTRUCTION_GOLDEN_COUNT != 0;
	for (size_t shard = 0; shard < DSP_INSTRUCTION_SHARD_COUNT; shard++) {
		char name[80];
		size_t first = DSP_INSTRUCTION_SHARD_FIRST[shard];
		size_t count = DSP_INSTRUCTION_SHARD_CASES[shard];

		if (!test_check("DSP reset returns to Mask ROM boot dispatcher", reset_dsp()))
			return test_finish();
		DSP_SHARED_MEMORY[DSP_DONE_OFFSET] = 0;
		for (size_t i = 0; i < count * DSP_INSTRUCTION_RECORD_WORDS; i++)
			DSP_SHARED_MEMORY[DSP_INSTRUCTION_RESULT_OFFSET + i] = 0;

		tfp_sprintf(name, "boot commands load instruction corpus shard %u", (uint32_t) shard);
#ifdef DSP_INSTRUCTION_IMAGES_PACKED
		const uint8_t *image = unpack_instruction_image(shard);
#else
		const uint8_t *image = DSP_INSTRUCTION_IMAGES[shard];
#endif
		if (!test_check(name, load_dsp1_image(image)))
			return test_finish();
		tfp_sprintf(name, "BRANCH starts instruction corpus shard %u", (uint32_t) shard);
		if (!test_check(name, branch_to_test()))
			return test_finish();
		tfp_sprintf(name, "instruction corpus shard %u reaches completion marker", (uint32_t) shard);
		if (!test_check(name, wait_for_test_done())) {
			print_capture(first, count);
			return test_finish();
		}

		print_capture(first, count);
		if (compare_results) {
#ifdef DSP_INSTRUCTION_GOLDEN_PACKED
			const uint16_t *golden = unpack_instruction_golden(shard);
#else
			const uint16_t *golden = DSP_INSTRUCTION_GOLDEN[first];
#endif
			compare_golden(first, count, golden);
		}
	}

	test_category("Hardware golden comparison");
	if (!compare_results)
		test_skip("instruction results match EL71", "hardware capture has not been imported");
	return test_finish();
}
