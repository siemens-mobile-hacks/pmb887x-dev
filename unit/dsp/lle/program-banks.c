#include <pmb887x.h>
#include <gen/dsp.h>

#ifdef PMB8875
#include "program-banks-8875.inc"
#endif
#ifdef PMB8876
#include "program-banks-8876.inc"
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x01FF
#define RESULT_OFFSET 0x0200
#define MAX_PROBE_WORDS 5
#define MAX_RESULT_WORDS 3

struct program_bank_config {
	uint16_t mask_id;
	uint16_t probe_address;
	uint16_t runner_address;
	size_t page_count;
	size_t probe_words;
	size_t result_words;
	const uint8_t *image;
	size_t image_size;
	const uint16_t (*expected_words)[MAX_PROBE_WORDS];
	const uint16_t (*expected_results)[MAX_RESULT_WORDS];
	const uint16_t *page_order;
	size_t run_count;
};

#ifdef PMB8875
static const uint16_t PAGE_ORDER_8875[] = { 0, 1, 0, 1, 1, 0 };

static const uint16_t PROGRAM_WORDS_0602[][MAX_PROBE_WORDS] = {
	{ 0x4580, 0x5E1A, 0xEA04, 0x1B44 }, // ret; mov 0xEA04,a0l; mov a0l,[r4]
	{ 0x8C88, 0x4580, 0x3F03, 0x2E3A }, // cmp [r0++],a0; ret; mov +3,y0; mov y0,[page:3A]
};

static const uint16_t PROGRAM_RESULTS_0602[][MAX_RESULT_WORDS] = {
	{ TEAK_SHARED_RAM_BASE + 0x0100 },
	{ TEAK_SHARED_RAM_BASE + 0x0101 },
};

static const uint16_t PROGRAM_WORDS_0604[][MAX_PROBE_WORDS] = {
	{ 0x4580, 0xD4B8, 0x58D2, 0x5021 }, // ret; mov [0x58D2],a0; brr +2,eq
	{ 0x6760, 0x4580, 0x5E1D, 0x7FFF }, // clr a0; ret; mov 0x7FFF,a1h
};

static const uint16_t PROGRAM_RESULTS_0604[][MAX_RESULT_WORDS] = {
	{ 0xEEEE },
	{ 0x0000 },
};
#endif

#ifdef PMB8876
static const uint16_t PAGE_ORDER_8876[] = { 0, 1, 2, 0, 2, 1, 0, 1, 2 };

static const uint16_t PROGRAM_WORDS_0801[][MAX_PROBE_WORDS] = {
	// ret; mov 0xC5FC,r1; cmp [r1],a0; ret eq
	{ 0x4580, 0x5E01, 0xC5FC, 0x8C81, 0x4581 },
	// mov a0l,[r7+0x38]; ret; push st1; mov [0x7801],a0
	{ 0xDCB8, 0x4580, 0x5E49, 0xD4B8, 0x7801 },
	// mov [r1++],a0l; mov a0l,[r4++]; mov [r2++],a0l; mov a0l,[r5++]; ret
	{ 0x1F49, 0x1B4C, 0x1F4A, 0x1B4D, 0x4580 },
};

static const uint16_t PROGRAM_RESULTS_0801[][MAX_RESULT_WORDS] = {
	{ 0xEEEE, 0xEEEE, 0xEEEE },
	{ 0x0004, 0xEEEE, 0xEEEE },
	{ 0xEEEE, 0x1111, 0x2222 },
};
#endif

static const struct program_bank_config PROGRAM_BANK_CONFIGS[] = {
#ifdef PMB8875
	{
		.mask_id = 0x0602,
		.probe_address = 0xE9F4,
		.runner_address = 0x0100,
		.page_count = ARRAY_SIZE(PROGRAM_WORDS_0602),
		.probe_words = 4,
		.result_words = 1,
		.image = DSP_PROGRAM_BANKS_8875,
		.image_size = sizeof(DSP_PROGRAM_BANKS_8875),
		.expected_words = PROGRAM_WORDS_0602,
		.expected_results = PROGRAM_RESULTS_0602,
		.page_order = PAGE_ORDER_8875,
		.run_count = ARRAY_SIZE(PAGE_ORDER_8875),
	},
	{
		.mask_id = 0x0604,
		.probe_address = 0xC037,
		.runner_address = 0x0200,
		.page_count = ARRAY_SIZE(PROGRAM_WORDS_0604),
		.probe_words = 4,
		.result_words = 1,
		.image = DSP_PROGRAM_BANKS_8875,
		.image_size = sizeof(DSP_PROGRAM_BANKS_8875),
		.expected_words = PROGRAM_WORDS_0604,
		.expected_results = PROGRAM_RESULTS_0604,
		.page_order = PAGE_ORDER_8875,
		.run_count = ARRAY_SIZE(PAGE_ORDER_8875),
	},
#endif
#ifdef PMB8876
	{
		.mask_id = 0x0801,
		.probe_address = 0xD99C,
		.runner_address = 0x0100,
		.page_count = ARRAY_SIZE(PROGRAM_WORDS_0801),
		.probe_words = 5,
		.result_words = 3,
		.image = DSP_PROGRAM_BANKS_8876,
		.image_size = sizeof(DSP_PROGRAM_BANKS_8876),
		.expected_words = PROGRAM_WORDS_0801,
		.expected_results = PROGRAM_RESULTS_0801,
		.page_order = PAGE_ORDER_8876,
		.run_count = ARRAY_SIZE(PAGE_ORDER_8876),
	},
#endif
};

static const struct program_bank_config *find_program_bank_config(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(PROGRAM_BANK_CONFIGS); i++) {
		if (PROGRAM_BANK_CONFIGS[i].mask_id == mask_id)
			return &PROGRAM_BANK_CONFIGS[i];
	}

	return NULL;
}

static bool test_program_words(const struct program_bank_config *config) {
	uint16_t words[MAX_PROBE_WORDS];

	for (size_t page = 0; page < config->page_count; page++) {
		uint16_t page_value = (uint16_t) page << TEAK_DSP_PAGE_PROG_PAGE_SHIFT;

		if (!test_check("DLOAD selects Program ROM page for PREAD",
			dsp_hw_write_reg(TEAK_DSP_PAGE, page_value)))
			return false;
		if (!test_check("PREAD reads the common banked execution address",
			dsp_hw_read_program(config->probe_address, words, config->probe_words)))
			return false;
		char name[64];

		sprintf(name, "Program ROM page %lu words at %04X", (uint32_t) page, config->probe_address);
		test_eq_memory(name, config->expected_words[page], words, config->probe_words * sizeof(words[0]));
	}

	return test_check("DLOAD restores Program ROM page zero", dsp_hw_write_reg(TEAK_DSP_PAGE, 0));
}

static void test_execution_results(const struct program_bank_config *config) {
	for (size_t run = 0; run < config->run_count; run++) {
		size_t page = config->page_order[run];
		volatile uint16_t *result = dsp_hw_shared_memory + RESULT_OFFSET + run * config->result_words;
		char name[64];

		sprintf(name, "run %lu executes Program ROM page %lu", (uint32_t) run, (uint32_t) page);
		test_eq_memory(name, config->expected_results[page], result,
			config->result_words * sizeof(result[0]));
	}
}

int main(void) {
	test_start("DSP Program ROM bank execution test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	uint16_t mask_id = dsp_hw_shared_memory[0];
	printf("# DSP mask ID: %04lX\n", (uint32_t) mask_id);
	const struct program_bank_config *config = find_program_bank_config(mask_id);
	if (!test_check("DSP Mask ROM ID is supported", config != NULL))
		return test_finish();

	test_category("independent Program ROM words");
	if (!test_program_words(config))
		return test_finish();

	test_category("same-PC execution across Program ROM pages");
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t i = COMPLETE_OFFSET; i < RESULT_OFFSET + config->run_count * config->result_words; i++)
		dsp_hw_shared_memory[i] = 0;
	if (!test_check("boot commands load the Program ROM bank runner",
		dsp_hw_load_image(config->image, config->image_size)))
		return test_finish();
	if (!test_check("BRANCH starts the Program ROM bank runner", dsp_hw_branch(config->runner_address)))
		return test_finish();
	if (!test_check("Program ROM bank runner completes",
		dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 500)))
		return test_finish();
	test_execution_results(config);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
