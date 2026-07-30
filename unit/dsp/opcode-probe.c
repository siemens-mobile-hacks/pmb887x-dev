#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_BRANCH 2
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_STARTUP_ADDRESS 0x0100
#define DSP_PROBE_PAGE_ADDRESS 0x0154
#define DSP_PROBE_OPCODE_ADDRESS 0x0160
#define DSP_PROBE_PAGE_ZERO_WORD 0x0400
#define DSP_PROBE_PAGE_D6_WORD 0x04D6
#define DSP_EXPANSION_PAGE_FIRST_WORD 0xE100
#define DSP_EXPANSION_PAGE_TARGET 0xD600
#define DSP_EXPANSION_FIXED_SOURCE_ADDRESS 0xD6A0
#define DSP_PROGRAM_BACKING_LAST_ADDRESS 0x1FFF
#define DSP_SHARED_ADDRESS_MASK 0x0FFF
#define DSP_PROBE_TIMEOUT_MS 25
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_SEGMENT_ADDRESS_OFFSET 4
#define DSP1_SEGMENT_SIZE_OFFSET 8
#define DSP1_SEGMENT_MEMORY_TYPE_OFFSET 0x0F
#define DSP1_DATA_MEMORY_TYPE 2
#define DSP_MARKER_DONE_OFFSET 0x0300
#define DSP_MARKER_TRAP_OFFSET 0x0301
#define DSP_MARKER_ENTERED_OFFSET 0x0302
#define DSP_MARKER_POST_OFFSET 0x0303
#define DSP_MARKER_DONE 0x0053
#define DSP_MARKER_TRAP 0x0054
#define DSP_MARKER_ENTERED 0x0051
#define DSP_MARKER_POST 0x0052
#define DSP_EXPANSION_RESULT_OFFSET 0x0310
#define DSP_EXPANSION_DSP_RESULT_WORDS 19
#define DSP_EXPANSION_RESULT_WORDS 20

#ifndef DSP_OPCODE_PROBE_FIRST
#define DSP_OPCODE_PROBE_FIRST 0x0000
#endif

#ifndef DSP_OPCODE_PROBE_LAST
#define DSP_OPCODE_PROBE_LAST 0x00FF
#endif

_Static_assert(DSP_OPCODE_PROBE_FIRST >= 0 && DSP_OPCODE_PROBE_FIRST <= UINT16_MAX, "invalid first probe opcode");
_Static_assert(DSP_OPCODE_PROBE_LAST >= DSP_OPCODE_PROBE_FIRST && DSP_OPCODE_PROBE_LAST <= UINT16_MAX,
	"invalid last probe opcode");

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

#include "opcode-probe/probe-image.inc"

#ifdef DSP_EXPANSION_PROBE
#include "opcode-probe/expansion-probe-image.inc"
#include "opcode-probe/program-backing-image.inc"

enum expansion_address_mode {
	EXPANSION_VALUE,
	EXPANSION_PROGRAM_ADDRESS,
	EXPANSION_FIXED_SOURCE,
	EXPANSION_ABSOLUTE_READ,
	EXPANSION_R7_READ,
	EXPANSION_PAGE_WRITE,
	EXPANSION_ABSOLUTE_WRITE,
	EXPANSION_R7_WRITE,
};

struct expansion_family {
	const char *name;
	uint16_t first_word;
	enum expansion_address_mode address_mode;
};

static const struct expansion_family DSP_EXPANSION_FAMILIES[] = {
	{ "ALU long immediate add a0", 0x86C0, EXPANSION_VALUE },
	{ "ALU r7 long offset add a0", 0xD4DB, EXPANSION_R7_READ },
	{ "ALU long direct add a0", 0xD4FB, EXPANSION_ABSOLUTE_READ },
	{ "ALB indirect set r0", 0x80E0, EXPANSION_VALUE },
	{ "ALB register set r0", 0x81E0, EXPANSION_VALUE },
	{ "ALB direct set page 00", 0xE100, EXPANSION_PAGE_WRITE },
	{ "multiply indirect long immediate mpy a0", 0x8000, EXPANSION_FIXED_SOURCE },
	{ "msu indirect long immediate a0", 0x90C0, EXPANSION_FIXED_SOURCE },
	{ "mov long absolute a0", 0xD4B8, EXPANSION_ABSOLUTE_READ },
	{ "mov a0l long absolute", 0xD4BC, EXPANSION_ABSOLUTE_WRITE },
	{ "mov long immediate r0", 0x5E00, EXPANSION_VALUE },
	{ "mov long immediate b0", 0x5E20, EXPANSION_VALUE },
	{ "mov r7 long offset a0", 0xD498, EXPANSION_R7_READ },
	{ "mov a0l r7 long offset", 0xD49C, EXPANSION_R7_WRITE },
	{ "push long immediate", 0x5F40, EXPANSION_VALUE },
	{ "bkrep short", 0x5C00, EXPANSION_VALUE },
	{ "bkrep register r0", 0x5D00, EXPANSION_VALUE },
	{ "branch absolute always", 0x4180, EXPANSION_PROGRAM_ADDRESS },
	{ "call absolute always", 0x41C0, EXPANSION_PROGRAM_ADDRESS },
};

#ifndef DSP_EXPANSION_PROBE_FAMILY_FIRST
#define DSP_EXPANSION_PROBE_FAMILY_FIRST 0
#endif

#ifndef DSP_EXPANSION_PROBE_FAMILY_LAST
#define DSP_EXPANSION_PROBE_FAMILY_LAST 0
#endif

#ifndef DSP_EXPANSION_PROBE_WORD_FIRST
#define DSP_EXPANSION_PROBE_WORD_FIRST 0x0000
#endif

#ifndef DSP_EXPANSION_PROBE_WORD_LAST
#define DSP_EXPANSION_PROBE_WORD_LAST 0x00FF
#endif

#define DSP_EXPANSION_FAMILY_COUNT (sizeof(DSP_EXPANSION_FAMILIES) / sizeof(DSP_EXPANSION_FAMILIES[0]))

_Static_assert(DSP_EXPANSION_PROBE_FAMILY_FIRST >= 0 && DSP_EXPANSION_PROBE_FAMILY_FIRST < DSP_EXPANSION_FAMILY_COUNT,
	"invalid first expansion family");
_Static_assert(DSP_EXPANSION_PROBE_FAMILY_LAST >= DSP_EXPANSION_PROBE_FAMILY_FIRST &&
	DSP_EXPANSION_PROBE_FAMILY_LAST < DSP_EXPANSION_FAMILY_COUNT, "invalid last expansion family");
_Static_assert(DSP_EXPANSION_PROBE_WORD_FIRST >= 0 && DSP_EXPANSION_PROBE_WORD_FIRST <= UINT16_MAX,
	"invalid first expansion word");
_Static_assert(DSP_EXPANSION_PROBE_WORD_LAST >= DSP_EXPANSION_PROBE_WORD_FIRST &&
	DSP_EXPANSION_PROBE_WORD_LAST <= UINT16_MAX, "invalid last expansion word");
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

static uint32_t read_le32(const uint8_t *data) {
	return data[0] | (uint32_t) data[1] << 8 | (uint32_t) data[2] << 16 | (uint32_t) data[3] << 24;
}

static bool load_words(uint16_t command, uint16_t destination, const uint8_t *data, size_t words, uint16_t first_word,
	uint16_t second_word, uint16_t page_word, bool patch_words)
{
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = destination;
	boot_data[2] = (uint16_t) words;
	for (size_t i = 0; i < words; i++) {
		uint16_t word = data[i * 2] | (uint16_t) data[i * 2 + 1] << 8;
		if (patch_words && command == DSP_BOOT_PLOAD && destination + i == DSP_PROBE_OPCODE_ADDRESS) {
			word = first_word;
		} else if (patch_words && command == DSP_BOOT_PLOAD && destination + i == DSP_PROBE_OPCODE_ADDRESS + 1) {
			word = second_word;
		} else if (patch_words && command == DSP_BOOT_PLOAD && destination + i == DSP_PROBE_PAGE_ADDRESS) {
			word = page_word;
		}
		boot_data[3 + i] = word;
	}

	return submit_boot_command();
}

static bool load_dsp1_image(const uint8_t *image, uint16_t first_word, uint16_t second_word, uint16_t page_word,
	bool patch_words)
{
	size_t segments = image[DSP1_SEGMENT_COUNT_OFFSET];

	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = image + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint16_t address = (uint16_t) read_le32(entry + DSP1_SEGMENT_ADDRESS_OFFSET);
		size_t words = read_le32(entry + DSP1_SEGMENT_SIZE_OFFSET) / sizeof(uint16_t);
		uint16_t command = entry[DSP1_SEGMENT_MEMORY_TYPE_OFFSET] == DSP1_DATA_MEMORY_TYPE ? DSP_BOOT_DLOAD : DSP_BOOT_PLOAD;

		if (!load_words(command, address, image + offset, words, first_word, second_word, page_word, patch_words))
			return false;
	}

	return true;
}

static bool load_probe_image(uint16_t first_word, uint16_t second_word) {
#ifdef DSP_EXPANSION_PROBE
	uint16_t page_word = first_word == DSP_EXPANSION_PAGE_FIRST_WORD ? DSP_PROBE_PAGE_D6_WORD : DSP_PROBE_PAGE_ZERO_WORD;
	return load_dsp1_image(DSP_EXPANSION_PROBE_IMAGE, first_word, second_word, page_word, true);
#else
	return load_dsp1_image(DSP_OPCODE_PROBE_IMAGE, first_word, second_word, DSP_PROBE_PAGE_ZERO_WORD, true);
#endif
}

static bool branch_to_probe(void) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_BRANCH;
	boot_data[1] = DSP_STARTUP_ADDRESS;

	return submit_boot_command();
}

static void clear_markers(void) {
	DSP_SHARED_MEMORY[DSP_MARKER_DONE_OFFSET] = 0;
	DSP_SHARED_MEMORY[DSP_MARKER_TRAP_OFFSET] = 0;
	DSP_SHARED_MEMORY[DSP_MARKER_ENTERED_OFFSET] = 0;
	DSP_SHARED_MEMORY[DSP_MARKER_POST_OFFSET] = 0;
}

static bool wait_for_completion(void) {
	stopwatch_t start = stopwatch_get();
	while (DSP_SHARED_MEMORY[DSP_MARKER_DONE_OFFSET] != DSP_MARKER_DONE &&
		stopwatch_elapsed_ms(start) < DSP_PROBE_TIMEOUT_MS)
	{
		test_watchdog_serve();
	}

	return DSP_SHARED_MEMORY[DSP_MARKER_DONE_OFFSET] == DSP_MARKER_DONE;
}

#ifndef DSP_EXPANSION_PROBE
static int fail_transport(const char *operation, uint16_t opcode) {
	printf("# DSPPROBE-ERROR %04X %s\n", (uint32_t) opcode, operation);
	test_check(operation, false);
	return test_finish();
}

int main(void) {
	test_start("DSP TeakLite I raw opcode probe");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	uint32_t completed = 0;
	uint32_t trapped = 0;
	uint32_t timed_out = 0;

	for (uint32_t opcode = DSP_OPCODE_PROBE_FIRST; opcode <= DSP_OPCODE_PROBE_LAST; opcode++) {
		test_watchdog_reset();
		if (!reset_dsp())
			return fail_transport("DSP reset failed during opcode probe", (uint16_t) opcode);
		clear_markers();
		if (!load_probe_image((uint16_t) opcode, 0))
			return fail_transport("DSP1 load failed during opcode probe", (uint16_t) opcode);
		if (!branch_to_probe())
			return fail_transport("DSP branch failed during opcode probe", (uint16_t) opcode);

		bool done = wait_for_completion();
		uint16_t entered = DSP_SHARED_MEMORY[DSP_MARKER_ENTERED_OFFSET];
		uint16_t trap = DSP_SHARED_MEMORY[DSP_MARKER_TRAP_OFFSET];
		uint16_t post = DSP_SHARED_MEMORY[DSP_MARKER_POST_OFFSET];
		uint16_t done_marker = DSP_SHARED_MEMORY[DSP_MARKER_DONE_OFFSET];
		const char *outcome;
		if (done && trap == DSP_MARKER_TRAP) {
			outcome = "trap-complete";
			trapped++;
			completed++;
		} else if (done) {
			outcome = "complete";
			completed++;
		} else if (trap == DSP_MARKER_TRAP) {
			outcome = "trap-timeout";
			trapped++;
			timed_out++;
		} else {
			outcome = "timeout";
			timed_out++;
		}

		if (!done) {
			printf("# DSPPROBE-TRANSIENT %04X %04X %04X %04X %04X\n", opcode, (uint32_t) entered,
				(uint32_t) trap, (uint32_t) post, (uint32_t) done_marker);
			entered = UINT16_MAX;
			trap = UINT16_MAX;
			post = UINT16_MAX;
			done_marker = UINT16_MAX;
		}

		printf("# DSPPROBE %04X %s %04X %04X %04X %04X\n", opcode, outcome, (uint32_t) entered,
			(uint32_t) trap, (uint32_t) post, (uint32_t) done_marker);
		test_watchdog_serve();
		if ((opcode & 0xFF) == 0xFF)
			printf("# DSPPROBE-PROGRESS %04X\n", opcode);
	}

	printf("# DSPPROBE-SUMMARY %04X %04X complete=%u trap=%u timeout=%u\n", DSP_OPCODE_PROBE_FIRST,
		DSP_OPCODE_PROBE_LAST, completed, trapped, timed_out);
	test_check("opcode probe range completed", true);
	return test_finish();
}
#else
static uint16_t expansion_address(const struct expansion_family *family, uint16_t expansion) {
	if (family->address_mode == EXPANSION_R7_READ || family->address_mode == EXPANSION_R7_WRITE)
		return 0xD6C0 + expansion;
	return expansion;
}

static bool expansion_is_safe(const struct expansion_family *family, uint16_t expansion) {
	uint16_t address = expansion_address(family, expansion);

	switch (family->address_mode) {
	case EXPANSION_VALUE:
	case EXPANSION_FIXED_SOURCE:
	case EXPANSION_PAGE_WRITE:
		return true;
	case EXPANSION_PROGRAM_ADDRESS:
		return expansion <= DSP_PROGRAM_BACKING_LAST_ADDRESS;
	case EXPANSION_ABSOLUTE_READ:
	case EXPANSION_R7_READ:
		return address >= 0xD005 && address <= 0xD7FF;
	case EXPANSION_ABSOLUTE_WRITE:
	case EXPANSION_R7_WRITE:
		return address >= 0xD600 && address <= 0xD7FF;
	}

	return false;
}

static uint16_t expansion_observation_address(const struct expansion_family *family, uint16_t expansion) {
	bool uses_fixed_observation = family->address_mode == EXPANSION_VALUE ||
		family->address_mode == EXPANSION_PROGRAM_ADDRESS || family->address_mode == EXPANSION_FIXED_SOURCE;
	if (uses_fixed_observation)
		return DSP_EXPANSION_FIXED_SOURCE_ADDRESS;
	if (family->address_mode == EXPANSION_PAGE_WRITE)
		return DSP_EXPANSION_PAGE_TARGET;
	return expansion_address(family, expansion);
}

static void prepare_expansion_operand(const struct expansion_family *family, uint16_t expansion) {
	if (family->address_mode == EXPANSION_FIXED_SOURCE) {
		DSP_SHARED_MEMORY[DSP_EXPANSION_FIXED_SOURCE_ADDRESS & DSP_SHARED_ADDRESS_MASK] = 0x8001;
		return;
	}
	if (family->address_mode == EXPANSION_PAGE_WRITE) {
		DSP_SHARED_MEMORY[DSP_EXPANSION_PAGE_TARGET & DSP_SHARED_ADDRESS_MASK] = 0;
		return;
	}
	if (family->address_mode != EXPANSION_ABSOLUTE_READ && family->address_mode != EXPANSION_R7_READ)
		return;

	uint16_t address = expansion_address(family, expansion);
	uint16_t value = expansion ^ 0xA55A;
	if (address >= 0xD300 && address <= 0xD303)
		value = 0;
	DSP_SHARED_MEMORY[address & DSP_SHARED_ADDRESS_MASK] = value;
}

static int fail_expansion_transport(const char *operation, uint32_t family, uint16_t expansion) {
	printf("# DSPEXP-ERROR %u %04X %s\n", family, (uint32_t) expansion, operation);
	test_check(operation, false);
	return test_finish();
}

int main(void) {
	test_start("DSP TeakLite I expansion-word probe");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_watchdog_reset();
	if (!reset_dsp())
		return fail_expansion_transport("DSP reset failed before program backing load",
			DSP_EXPANSION_PROBE_FAMILY_FIRST, DSP_EXPANSION_PROBE_WORD_FIRST);
	if (!load_dsp1_image(DSP_PROGRAM_BACKING_IMAGE, 0, 0, 0, false))
		return fail_expansion_transport("DSP program backing load failed", DSP_EXPANSION_PROBE_FAMILY_FIRST,
			DSP_EXPANSION_PROBE_WORD_FIRST);

	uint32_t executed = 0;
	uint32_t skipped = 0;
	uint32_t completed = 0;
	uint32_t trapped = 0;
	uint32_t timed_out = 0;

	for (uint32_t family_index = DSP_EXPANSION_PROBE_FAMILY_FIRST;
		family_index <= DSP_EXPANSION_PROBE_FAMILY_LAST; family_index++)
	{
		const struct expansion_family *family = &DSP_EXPANSION_FAMILIES[family_index];

		printf("# DSPEXP-FAMILY %u %04X \"%s\"\n", family_index, (uint32_t) family->first_word, family->name);
		for (uint32_t expansion = DSP_EXPANSION_PROBE_WORD_FIRST;
			expansion <= DSP_EXPANSION_PROBE_WORD_LAST; expansion++)
		{
			if (!expansion_is_safe(family, (uint16_t) expansion)) {
				printf("# DSPEXP %u %04X unsafe-skip", family_index, expansion);
				for (size_t i = 0; i < 4 + DSP_EXPANSION_RESULT_WORDS; i++)
					printf(" FFFF");
				printf("\n");
				skipped++;
				test_watchdog_reset();
				if ((expansion & 0xFF) == 0xFF)
					printf("# DSPEXP-PROGRESS %u %04X\n", family_index, expansion);
				continue;
			}

			test_watchdog_reset();
			if (!reset_dsp())
				return fail_expansion_transport("DSP reset failed during expansion probe", family_index,
					(uint16_t) expansion);
			clear_markers();
			if (!load_probe_image(family->first_word, (uint16_t) expansion))
				return fail_expansion_transport("DSP1 load failed during expansion probe", family_index,
					(uint16_t) expansion);
			prepare_expansion_operand(family, (uint16_t) expansion);
			if (!branch_to_probe())
				return fail_expansion_transport("DSP branch failed during expansion probe", family_index,
					(uint16_t) expansion);

			bool done = wait_for_completion();
			uint16_t entered = DSP_SHARED_MEMORY[DSP_MARKER_ENTERED_OFFSET];
			uint16_t trap = DSP_SHARED_MEMORY[DSP_MARKER_TRAP_OFFSET];
			uint16_t post = DSP_SHARED_MEMORY[DSP_MARKER_POST_OFFSET];
			uint16_t done_marker = DSP_SHARED_MEMORY[DSP_MARKER_DONE_OFFSET];
			uint16_t results[DSP_EXPANSION_RESULT_WORDS];
			const char *outcome;
			if (done && trap == DSP_MARKER_TRAP) {
				outcome = "trap-complete";
				trapped++;
				completed++;
			} else if (done) {
				outcome = "complete";
				completed++;
			} else if (trap == DSP_MARKER_TRAP) {
				outcome = "trap-timeout";
				trapped++;
				timed_out++;
			} else {
				outcome = "timeout";
				timed_out++;
			}

			if (!done) {
				printf("# DSPEXP-TRANSIENT %u %04X %04X %04X %04X %04X\n", family_index, expansion,
					(uint32_t) entered, (uint32_t) trap, (uint32_t) post, (uint32_t) done_marker);
				entered = UINT16_MAX;
				trap = UINT16_MAX;
				post = UINT16_MAX;
				done_marker = UINT16_MAX;
			}
			for (size_t i = 0; i < DSP_EXPANSION_DSP_RESULT_WORDS; i++)
				results[i] = done ? DSP_SHARED_MEMORY[DSP_EXPANSION_RESULT_OFFSET + i] : UINT16_MAX;
			uint16_t observation_address = expansion_observation_address(family, (uint16_t) expansion);
			uint16_t observed_memory = UINT16_MAX;
			if (done)
				observed_memory = DSP_SHARED_MEMORY[observation_address & DSP_SHARED_ADDRESS_MASK];
			results[DSP_EXPANSION_DSP_RESULT_WORDS] = observed_memory;

			printf("# DSPEXP %u %04X %s %04X %04X %04X %04X", family_index, expansion, outcome,
				(uint32_t) entered, (uint32_t) trap, (uint32_t) post, (uint32_t) done_marker);
			for (size_t i = 0; i < DSP_EXPANSION_RESULT_WORDS; i++)
				printf(" %04X", (uint32_t) results[i]);
			printf("\n");
			executed++;
			test_watchdog_serve();
			if ((expansion & 0xFF) == 0xFF)
				printf("# DSPEXP-PROGRESS %u %04X\n", family_index, expansion);
		}
	}

	printf("# DSPEXP-SUMMARY families=%u-%u words=%04X-%04X executed=%u skipped=%u complete=%u trap=%u timeout=%u\n",
		DSP_EXPANSION_PROBE_FAMILY_FIRST, DSP_EXPANSION_PROBE_FAMILY_LAST, DSP_EXPANSION_PROBE_WORD_FIRST,
		DSP_EXPANSION_PROBE_WORD_LAST, executed, skipped, completed, trapped, timed_out);
	test_check("expansion probe range completed", true);
	return test_finish();
}
#endif
